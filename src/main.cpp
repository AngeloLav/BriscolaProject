#include "detector.hpp"
#include "recognizer.hpp"
#include "../model/gameModels.h"
#include "analyzer.hpp"

// Standard include
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

// OpenCV
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

// Model libraries
#include <opencv2/dnn/dnn.hpp>

#include "JsonReader.h"
#include "GameEngine.h"
#include "BriscolaRules.h"
#include "ErrorResolver.h"
#include "OutputWriter.h"
#include "MetricsEvaluator.h"

namespace {

// Keep these ids in sync with Detector::drawDetections().
constexpr int BRISCOLA_CLASS_ID = 0;
constexpr int PLAYED_CARD_CLASS_ID = 1;

// Returns the round number from the video path
int getRoundNumberFromVideoPath(const cv::String& videoPath) {
    const std::string stem = std::filesystem::path(videoPath).stem().string();
    const std::string marker = "round";
    const std::size_t markerPosition = stem.rfind(marker);

    if (markerPosition == std::string::npos) {
        return std::numeric_limits<int>::max();
    }

    try {
        return std::stoi(stem.substr(markerPosition + marker.size()));
    }
    catch (const std::exception&) {
        return std::numeric_limits<int>::max();
    }
}

void printCardCandidates(
    const std::string& label,
    const std::vector<CardDetected>& candidates
) {
    std::cout << " " << label << " candidates:" << std::endl;

    if (candidates.empty()) {
        std::cout << "  <empty>" << std::endl;
        return;
    }

    for (const auto& candidate : candidates) {
        std::cout << "  - "
                  << candidate.card.value << " of "
                  << suitToString(candidate.card.type)
                  << " | confidence=" << candidate.confidence
                  << std::endl;
    }
}

bool isValidRecognizedCard(const Card& card) {
    const int type = static_cast<int>(card.type);
    return card.value >= 1 && card.value <= 10 && type >= 0 && type <= 3;
}

} // namespace


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Use the program with: ./briscola <game_folder>"
                  << std::endl;
        return 1;
    }

    std::string gameFolder = argv[1];

    if (!gameFolder.empty() &&
        gameFolder.back() != '/' &&
        gameFolder.back() != '\\') {
        gameFolder += "/";
    }

    std::string folderWithoutSlash =
        gameFolder.substr(0, gameFolder.size() - 1);

    size_t lastSlash =
        folderWithoutSlash.find_last_of("/\\");

    std::string gameName =
        (lastSlash == std::string::npos)
            ? folderWithoutSlash
            : folderWithoutSlash.substr(lastSlash + 1);

    std::string dataFolder =
        (lastSlash == std::string::npos)
            ? ""
            : folderWithoutSlash.substr(0, lastSlash + 1);

    std::string jsonPath =
        gameFolder + "prediction.json";

    std::string resultsFolder =
        dataFolder + "results/";

    // Find the ground truth CSV inside the game folder.
    std::vector<cv::String> csvFiles;

    cv::glob(
        gameFolder + "*.csv",
        csvFiles,
        false
    );

    if (csvFiles.empty()) {
        std::cerr << "No CSV ground truth found in: "
                  << gameFolder << std::endl;
        return 1;
    }

    if (csvFiles.size() > 1) {
        std::cerr << "More than one CSV found in: "
                  << gameFolder << std::endl;
        return 1;
    }

    std::string groundTruthPath = csvFiles[0];

    // Video test code. json da togliere
    std::vector<cv::String> videoFiles;

    cv::glob(
        gameFolder + "*.mp4",
        videoFiles,
        false
    );

        std::sort(
        videoFiles.begin(),
        videoFiles.end(),
        [](const cv::String& lhs, const cv::String& rhs) {
            const int lhsRound = getRoundNumberFromVideoPath(lhs);
            const int rhsRound = getRoundNumberFromVideoPath(rhs);

            if (lhsRound != rhsRound) {
                return lhsRound < rhsRound;
            }

            return lhs < rhs;
        }
    );

    std::cout << "Video files found: " << videoFiles.size() << std::endl;
    if (videoFiles.size() != 20) {
        std::cerr << "Warning: expected 20 round videos, found "
                  << videoFiles.size() << std::endl;
    }

    
    Detector detector("model/best.onnx");
    CardRecognizer recognizer("Briscola_Trentine");
    cv::namedWindow("Briscola video", cv::WINDOW_NORMAL);
    cv::resizeWindow("Briscola video", 1280, 720);

    GamePrediction prediction;
    
    std::vector<CardObservation> allBriscolaDetections;

    int fallbackRoundNumber = 1;

    for (const auto& videoPath : videoFiles) {
        int roundNumber = getRoundNumberFromVideoPath(videoPath);
        if (roundNumber == std::numeric_limits<int>::max()) {
            roundNumber = fallbackRoundNumber;
        }
        fallbackRoundNumber++;

        std::cout << "\nProcessing round " << roundNumber
                  << ": " << std::filesystem::path(videoPath).filename().string()
                  << std::endl;

        cv::VideoCapture video(videoPath);
        if (!video.isOpened()) {
            std::cout << "Unable to open video: " << videoPath << std::endl;
            continue;
        }

        /*
        Detector detector("model/best.onnx");
        CardRecognizer recognizer("Briscola_Trentine");
        cv::namedWindow("Briscola video", cv::WINDOW_NORMAL);
        cv::resizeWindow("Briscola video", 1280, 720);
        */

        cv::Mat frame;

        std::vector<CardObservation> northDetections;
        std::vector<CardObservation> southDetections;
        std::vector<CardObservation> briscolaDetections;

        int sampledFrames = 0;
        int detectorDetections = 0;
        int invalidBoxes = 0;
        int recognitionFailures = 0;
        int briscolaClassDetections = 0;
        int playedCardClassDetections = 0;
        int unknownClassDetections = 0;

        //who plays at first?
        int firstNorthFrame = -1;
        int firstSouthFrame = -1;

        // Limita i frame presi, non so se poi volete calibrare meglio o togliere
        // Sample a fixed number of frames from each video during CV testing.
        int totalFrames = static_cast<int>(
            video.get(cv::CAP_PROP_FRAME_COUNT)
        );

        int frameStep = std::max(1, totalFrames / 15);
        int frameIndex = 0;


        while (video.read(frame)) {

            // Skip frames
            if (frameIndex % frameStep != 0) {
                frameIndex++;
                continue;
            }

            std::cout << "Frame: " << frameIndex
                      << " | Size:\nx = " << frame.cols
                      << "\ny = " << frame.rows << std::endl;

            sampledFrames++;

            std::vector<Detection> detections = detector.detect(frame);
            
            // NB. For the next that will work on this: each detection contains:
            // - detection.box --> bb in the original frame coordinates
            // - detection.confidence --> confidence score
            // - detecion.classId --> detected class

            // The BB will be used as a ROI (region of interest) to crop the detected 
            // card from the original frame for further processing. I tried organizing code so that you don't have 
            // to deal with what i have written, just starting from the detections.

            // The idea i had was to:
            // 1) Take the crops from the original frame by the BB
            // 2) Using some pre-processing if useful, to deal with motion blur 
            // 3) Finding a way to make the card view frontal
            // 4) Choosing a feature matching technique to confront the keypoints obtained
            //    by each crop with SIFT with the keypoints obtained by the labeled cards Trentine
            //    that we keep into another folder (they are on google drive of the assignement)
            // 5) Classifying the type of card based on the matching for each frame of the video

            // Once this matching work, the last part for the last who will work here will be organizing
            // and ordering the detections, and counting points as it is described in the assignment

            for(const auto& detection : detections) {
                detectorDetections++;

                cv::Rect safebox=detection.box & cv::Rect(0, 0, frame.cols, frame.rows); // This is to avoid the case where the BB is partially outside the frame
                if(safebox.width<=0||safebox.height<=0) continue; // This is to avoid the case where the BB is completely outside the frame
                cv::Mat croppedcard=frame(safebox);
                std::vector<CardDetected> recognizedCards =
                    recognizer.identifyCard(croppedcard);

                std::cout << "[CV] frame=" << frameIndex
                          << " | class=" << detection.classId
                          << " | candidates=" << recognizedCards.size();

                if (recognizedCards.empty()) {
                    recognitionFailures++;
                    std::cout << " | rejected: no match" << std::endl;
                    continue;
                }

                // Keep all valid candidates from this detector box together.
                // The inner vector represents one observation and must not be counted
                // as multiple independent frames by the analyzer.
                std::vector<CardDetected> validCandidates;

                for (const auto& candidate : recognizedCards) {
                    if (isValidRecognizedCard(candidate.card)) {
                        validCandidates.push_back(candidate);
                    }
                }

                if (validCandidates.empty()) {
                    recognitionFailures++;
                    std::cout << " | rejected: invalid candidates" << std::endl;
                    continue;
                }

                std::cout << std::endl;

                // Store the complete candidate set for this observation.
                // ErrorResolver may need the second or third candidate later.
                if (detection.classId == BRISCOLA_CLASS_ID) {
                    briscolaClassDetections++;

                    briscolaDetections.push_back({validCandidates});
                    allBriscolaDetections.push_back({validCandidates});

                    continue;
                }

                if (detection.classId != PLAYED_CARD_CLASS_ID) {
                    unknownClassDetections++;
                    std::cout << "[CV] rejected: unknown detector class "
                              << detection.classId << std::endl;
                    continue;
                }

                playedCardClassDetections++;
                int centerY=safebox.y+safebox.height/2;
                //int centerX=safebox.x+safebox.width/2;
                if (centerY < frame.rows / 2) {
                    northDetections.push_back({validCandidates});
                    if (firstNorthFrame == -1) firstNorthFrame = frameIndex;
                } else {
                    southDetections.push_back({validCandidates});
                    if (firstSouthFrame == -1) firstSouthFrame = frameIndex;
                }
            }

            // Used for development, maybe it will be commented in the final project
            detector.drawDetections(frame, detections);

            // Showing each frame
            cv::imshow("Briscola video", frame);
            int key = cv::waitKey(30);

            // If user presses ESC the video stops
            if (key == 27) break;

            frameIndex++;
            //cv::imshow("Briscola video", frame);
            //if (cv::waitKey(30) == 27) break;
        }

        //round's data
        RoundPrediction currentRoundPred;
        currentRoundPred.round = roundNumber; 

        currentRoundPred.northDetected = getRankedCardsWithConfidence(northDetections);
        currentRoundPred.southDetected = getRankedCardsWithConfidence(southDetections);

        std::cout << "[ROUND " << roundNumber << "] "
                  << "sampled frames=" << sampledFrames
                  << " | detector detections=" << detectorDetections
                  << " | invalid ROI=" << invalidBoxes
                  << " | recognition failures=" << recognitionFailures
                  << " | briscola class=" << briscolaClassDetections
                  << " | played-card class=" << playedCardClassDetections
                  << " | unknown class=" << unknownClassDetections
                  << std::endl;
        std::cout << "[ROUND " << roundNumber << "] raw vectors before ranking: "
                  << "north=" << northDetections.size()
                  << ", south=" << southDetections.size()
                  << ", briscola=" << briscolaDetections.size()
                  << std::endl;
        std::cout << "[ROUND " << roundNumber << "] candidates after ranking: "
                  << "north=" << currentRoundPred.northDetected.size()
                  << ", south=" << currentRoundPred.southDetected.size()
                  << std::endl;

        PlayerDetected leaderPred;
        if(firstNorthFrame != -1 && (firstSouthFrame == -1 || firstNorthFrame < firstSouthFrame)) {
            leaderPred.player = Player::NORTH;
            leaderPred.confidence = 0.9; 
        } else if(firstSouthFrame != -1 && (firstNorthFrame == -1 || firstSouthFrame < firstNorthFrame)) {
            leaderPred.player = Player::SOUTH;
            leaderPred.confidence = 0.9; 
        } else {
            //if we can't determine the leader, we can set a default or handle it differently
            leaderPred.player = Player::NORTH;
            leaderPred.confidence = 0.5; 
        }
        currentRoundPred.leaderDetected.push_back(leaderPred);
        /*
        leaderPred.player = Player::NORTH;
        leaderPred.confidence = 1.0; 
        currentRoundPred.leaderDetected.push_back(leaderPred);
        */
        prediction.rounds.push_back(currentRoundPred);

        //std::vector<CardDetected> briscolaCandidates = getRankedCardsWithConfidence(briscolaDetections);
        // ------------------------------------------------


        //Card finalNorthCard=getMostFreqCard(northDetections);
        //Card finalSouthCard=getMostFreqCard(southDetections);
        //Card finalBriscola=getMostFreqCard(briscolaDetections);

        //Player leader=leaderPred.player;
        //Player winner=detWinner(finalNorthCard, finalSouthCard, finalBriscola, leader);
        
        //calculate points
        //int roundPoints = getCardPoints(finalNorthCard.value) + getCardPoints(finalSouthCard.value);

        std::cout << "\n================ GAME PREDICTION DATA ================" << std::endl;
        /*
        std::cout << "NORTH Candidates (sorted by confidence):" << std::endl;
        for (const auto& cd : currentRoundPred.northDetected) {
            std::cout << "  - Card: " << cd.card.value << " of " << suitToString(cd.card.type)
                    << " | Confidence: " << (cd.confidence * 100.0) << "%" << std::endl;
        }
        std::cout << "\nSOUTH Candidates (sorted by confidence):" << std::endl;
        for (const auto& cd : currentRoundPred.southDetected) {
            std::cout << "  - Card: " << cd.card.value << " of " << suitToString(cd.card.type)
                    << " | Confidence: " << (cd.confidence * 100.0) << "%" << std::endl;
        }
        std::cout << "\nBRISCOLA Candidates (sorted by confidence):" << std::endl;
        for (const auto& cd : briscolaCandidates) {
            std::cout << "  - Card: " << cd.card.value << " of " << suitToString(cd.card.type)
                    << " | Confidence: " << (cd.confidence * 100.0) << "%" << std::endl;
        }
        std::cout << "======================================================\n" << std::endl;
        std::cout << "\n=================== ROUND RESULT ===================" << std::endl;
        std::cout << "North Card: " << finalNorthCard.value << " of " << suitToString(finalNorthCard.type) << " (" << getCardPoints(finalNorthCard.value) << " pts)" << std::endl;
        std::cout << "South Card: " << finalSouthCard.value << " of " << suitToString(finalSouthCard.type) << " (" << getCardPoints(finalSouthCard.value) << " pts)" << std::endl;
        std::cout << "Briscola:   " << finalBriscola.value << " of " << suitToString(finalBriscola.type) << std::endl;
        std::cout << "Leader:     " << (leader == Player::NORTH ? "NORTH" : "SOUTH") << std::endl;
        std::cout << "Winner:     " << (winner == Player::NORTH ? "NORTH" : "SOUTH") << std::endl;
        std::cout << "Points Won: " << roundPoints << " pts" << std::endl;
        std::cout << "====================================================\n" << std::endl;
        */
        std::cout << "Leader rilevato: " << (leaderPred.player == Player::NORTH ? "NORTH" : "SOUTH") 
                  << " (Frame N: " << firstNorthFrame << ", Frame S: " << firstSouthFrame << ")" << std::endl;
        std::cout << "======================================================\n" << std::endl;

    }



    prediction.briscolaDetected =
    getRankedCardsWithConfidence(
        allBriscolaDetections
    );

    std::cout << "\n================ GamePrediction audit ================" << std::endl;
    std::cout << "Rounds: " << prediction.rounds.size() << std::endl;

    for (const auto& round : prediction.rounds) {
        std::cout << "Round " << round.round << std::endl;
        printCardCandidates("North", round.northDetected);
        printCardCandidates("South", round.southDetected);
        std::cout << " Leader candidates: " << round.leaderDetected.size()
                  << std::endl;
    }

    printCardCandidates("Briscola", prediction.briscolaDetected);
    std::cout << "=======================================================" << std::endl;

    // Read the json for debugging (probabilmente sta parte di json sarà meglio toglierla, ora mi serve per testare più partite plausibile)
    // GamePrediction prediction = JsonReader::readGamePrediction(jsonPath);

    /*
    // Known briscola: this test is only for UNKNOWN played cards
    prediction.briscolaDetected.push_back({
        { CardType::DENARI, 6 },
        0.99
    });

    // Two failed card detections
    prediction.rounds[7].leaderDetected.clear();
    prediction.rounds[7].southDetected.clear();
    prediction.rounds[7].northDetected.clear();

    prediction.rounds[8].leaderDetected.clear();
    prediction.rounds[8].southDetected.clear();
    prediction.rounds[8].northDetected.clear();

    // Leader detection is wrong
    prediction.rounds[9].leaderDetected[0].player = Player::SOUTH;
    */

    Game game = GameEngine::createGame(prediction);

    ValidationResult before = Validator::validate(game);

    std::cout << "Card issues before: "
              << before.cardIssues.size() << std::endl;

    int cardCorrections = ErrorResolver::resolveCardIssues(game);
    int briscolaCorrections = 0;
    int leaderCorrections = 0;

    ValidationResult afterCards = Validator::validate(game);

    if (afterCards.cardIssues.empty()) {

        briscolaCorrections =
            ErrorResolver::resolveBriscola(game);

        leaderCorrections =
            ErrorResolver::resolveLeaderIssues(game);

        briscolaCorrections +=
            ErrorResolver::resolveBriscola(game);
    }

    briscolaCorrections += ErrorResolver::resolveBriscola(game);

    ValidationResult finalValidation = Validator::validate(game);

    std::cout << "Card corrections: "
              << cardCorrections << std::endl;

    std::cout << "Briscola corrections: "
              << briscolaCorrections << std::endl;

    std::cout << "Leader corrections: "
              << leaderCorrections << std::endl;

    std::cout << "Card issues after: "
              << finalValidation.cardIssues.size() << std::endl;

    std::cout << "Leader issues after: "
              << finalValidation.leaderIssues.size() << std::endl;

    std::cout << "Corrections: "
              << cardCorrections +
                 briscolaCorrections +
                 leaderCorrections
              << std::endl;

    OutputWriter::writeTxt(
        game,
        resultsFolder + gameName + "_output.txt"
    );

    OutputWriter::writeCsv(
        game,
        resultsFolder + gameName + "_results.csv"
    );

    std::cout << "\nGround truth: " << groundTruthPath << std::endl;
    std::cout << "Evaluating metrics..." << std::endl;

    try {
        MetricsResult metrics =
            MetricsEvaluator::evaluate(
                game,
                groundTruthPath
            );

        std::cout << "Metrics calculated." << std::endl;

        MetricsEvaluator::printMetrics(metrics);

        MetricsEvaluator::writeMetrics(
            metrics,
            resultsFolder + gameName + "_metrics.txt"
        );
    }
    catch (const std::exception& e) {
        std::cerr << "METRICS ERROR: "
                  << e.what()
                  << std::endl;
        return 1;
    }

    return 0;
}
