#include "detector.hpp"
#include "recognizer.hpp"
#include "../model/gameModels.h"
#include "analyzer.hpp"
#include "GameInput.h"

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

#include "GameEngine.h"
#include "BriscolaRules.h"
#include "ErrorResolver.h"
#include "OutputWriter.h"
#include "MetricsEvaluator.h"


namespace {

constexpr int BRISCOLA_CLASS_ID = 0;
constexpr int PLAYED_CARD_CLASS_ID = 1;
constexpr int FRAME_SCANNED_NUMBER = 40;
const double SECOND_CARD_DISTANCE_THRESHOLD = 100;
constexpr bool PRINT_FRAME_DETECTIONS = true;
constexpr bool ENABLE_ERROR_CORRECTION = true;

void printCards(const std::vector<CardDetected>& candidates) {
    if (candidates.empty()) {
        std::cout << "-";
        return;
    }

    size_t candidatesToPrint = std::min<size_t>(3, candidates.size());

    for (size_t i = 0; i < candidatesToPrint; i++) {
        const auto& candidate = candidates[i];

        std::cout << (i == 0 ? "" : " | ")
                  << candidate.card.value << " "
                  << suitToString(candidate.card.type)
                  << " (" << candidate.confidence << ")";
    }
}

void printCardCandidates(
    const std::string& label,
    const std::vector<CardDetected>& candidates
) {
    std::cout << " " << label << ": ";
    printCards(candidates);
    std::cout << std::endl;
}

void printFrameDetections(
    int round,
    int frame,
    const std::vector<CardDetected>& north,
    const std::vector<CardDetected>& south,
    const std::vector<CardDetected>& briscola
) {
    std::cout << "[Round " << round << " | Frame " << frame << "] \n N: ";
    printCards(north);
    std::cout << " \nS: ";
    printCards(south);
    std::cout << " \nB: ";
    printCards(briscola);
    std::cout << std::endl;
}

const char* playerToString(Player player) {
    return player == Player::NORTH ? "NORTH" : "SOUTH";
}

bool isValidRecognizedCard(const Card& card) {
    const int type = static_cast<int>(card.type);
    return card.value >= 1 && card.value <= 10 && type >= 0 && type <= 3;
}

} // namespace


int main(int argc, char** argv) {
    GameInput input;
    if (!loadGameInput(argc, argv, input)) {
        return 1;
    }
    
    Detector detector("model/best.onnx");
    CardRecognizer recognizer("Briscola_Trentine");
    //cv::namedWindow("Briscola video", cv::WINDOW_NORMAL);
    //cv::resizeWindow("Briscola video", 1280, 720);

    GamePrediction prediction;
    
    std::vector<CardObservation> allBriscolaDetections;

    int fallbackRoundNumber = 1;

    for (const auto& videoPath : input.videoFiles) {
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

        // --- Orientation adjustment ---
        double orientation =
            video.get(cv::CAP_PROP_ORIENTATION_META);

        bool orientationEnabled =
            video.set(
                cv::CAP_PROP_ORIENTATION_AUTO,
                1
            );


        /*
        Detector detector("model/best.onnx");
        CardRecognizer recognizer("Briscola_Trentine");
        cv::namedWindow("Briscola video", cv::WINDOW_NORMAL);
        cv::resizeWindow("Briscola video", 1280, 720);
        */

        cv::Mat frame;

        std::vector<CardObservation> northDetections;
        std::vector<CardObservation> southDetections;

        // First valid detection determines which player starts the round.
        int firstNorthFrame = -1;
        int firstSouthFrame = -1;

        // Once a second independent box is found, detections are assigned
        // to the other player and the first card is no longer updated.
        bool secondCardDetected = false;

        cv::Rect firstCardBox;

        int firstCardFrame = -1;

        int firstCardSide = -1;

        cv::Rect secondCardBox;

        int secondCardFrame = -1;

        cv::Rect lastPlayedCardBox;
        cv::Rect trackedSecondCardBox;
        cv::Rect firstCardLockedBox;
        cv::Rect previousSecondCardBox;

        // Limita i frame presi, non so se poi volete calibrare meglio o togliere
        // Sample a fixed number of frames from each video during CV testing.
        int totalFrames = static_cast<int>(
            video.get(cv::CAP_PROP_FRAME_COUNT)
        );

        int firstTwoThirdsFrames = std::max(1, totalFrames * 2 / 3);
        int frameStep = std::max(1, firstTwoThirdsFrames / FRAME_SCANNED_NUMBER);
        int frameIndex = 0;
        int scannedFrameCount = 0;


        while (frameIndex < firstTwoThirdsFrames &&
       scannedFrameCount < FRAME_SCANNED_NUMBER && video.read(frame)) {

           
            // Skip frames
            if (frameIndex % frameStep != 0) {
                frameIndex++;
                continue;
            }

            std::vector<Detection> detections = detector.detect(frame);
            std::vector<CardDetected> frameNorthCandidates;
            std::vector<CardDetected> frameSouthCandidates;
            std::vector<CardDetected> frameBriscolaCandidates;

            bool scanBriscola = scannedFrameCount % std::max(1, FRAME_SCANNED_NUMBER / 3) == 0;

            // Count the played-card boxes in this frame before processing them. 2 boxes are needed to make the switch
            std::vector<cv::Rect> playedCardBoxes;
            for (const auto& detection : detections) {
                if (detection.classId != PLAYED_CARD_CLASS_ID) {
                    continue;
                }

                cv::Rect box =
                    detection.box & cv::Rect(0, 0, frame.cols, frame.rows);

                if (box.width > 0 && box.height > 0) {
                    playedCardBoxes.push_back(box);
                }
            }

            if (!secondCardDetected && playedCardBoxes.size() == 1) {
                lastPlayedCardBox = playedCardBoxes[0];
            }

            // The box farthest from the previous position is the new card.
            if (!secondCardDetected &&
                !firstCardBox.empty() &&
                playedCardBoxes.size() >= 2) {
                cv::Rect previousBox = lastPlayedCardBox.empty()
                    ? firstCardBox
                    : lastPlayedCardBox;
                cv::Rect oldCardBox;
                cv::Rect newCardBox;
                double minDistance = std::numeric_limits<double>::max();
                double maxDistance = -1.0;

                for (const auto& box : playedCardBoxes) {
                    double distance =
                        cv::norm(
                            cv::Point(box.x, box.y) -
                            cv::Point(previousBox.x, previousBox.y)
                        );

                    if (distance < minDistance) {
                        minDistance = distance;
                        oldCardBox = box;
                    }

                    if (distance > maxDistance) {
                        maxDistance = distance;
                        newCardBox = box;
                    }
                }

                bool newCardIsNorth =
                    newCardBox.y + newCardBox.height / 2 < frame.rows / 2;
                int newCardSide = newCardIsNorth ? 0 : 1;

                // If both boxes are South and the new one is farther South,
                // the first card was detected after crossing the centre.
                bool wrongFirstSide =
                    firstCardSide == 1 &&
                    newCardSide == 1 &&
                    newCardBox.y + newCardBox.height / 2 >
                        oldCardBox.y + oldCardBox.height / 2;

                if (maxDistance > SECOND_CARD_DISTANCE_THRESHOLD &&
                    (newCardSide != firstCardSide || wrongFirstSide)) {
                    if (wrongFirstSide) {
                        std::swap(northDetections, southDetections);
                        std::swap(firstNorthFrame, firstSouthFrame);
                        firstCardSide = 1 - firstCardSide;
                    }

                    secondCardDetected = true;
                    secondCardBox = newCardBox;
                    trackedSecondCardBox = newCardBox;
                    secondCardFrame = frameIndex;
                    previousSecondCardBox = secondCardBox;

                    firstCardLockedBox = oldCardBox;
                }
            }
            
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
                // Skip Briscola detections unless it's time to scan for it.
                // I scan for briscola only more or less 3 times because are enough and avoid computation waste
                if (detection.classId == BRISCOLA_CLASS_ID &&
                    !scanBriscola) {
                    continue;
                }

                cv::Rect safebox=detection.box & cv::Rect(0, 0, frame.cols, frame.rows); // This is to avoid the case where the BB is partially outside the frame
                if(safebox.width<=0||safebox.height<=0) continue; // This is to avoid the case where the BB is completely outside the frame
                cv::Mat croppedcard=frame(safebox);

                cv::Rect currentBox = safebox;
                bool isNorthZone =
                    currentBox.y + currentBox.height / 2 < frame.rows / 2;

                // After the switch, keep following the second card by position.
                // If i have only one card, skip the bounding box that is at the same position as the first card
                if (detection.classId == PLAYED_CARD_CLASS_ID &&
                    secondCardDetected)
                {
                    if (playedCardBoxes.size() == 1)
                    {
                        cv::Point detectedCenter(
                            playedCardBoxes[0].x + playedCardBoxes[0].width / 2,
                            playedCardBoxes[0].y + playedCardBoxes[0].height / 2
                        );

                        cv::Point oldCardCenter(
                            firstCardLockedBox.x + firstCardLockedBox.width / 2,
                            firstCardLockedBox.y + firstCardLockedBox.height / 2
                        );

                        double distanceFromOld =
                            cv::norm(detectedCenter - oldCardCenter);

                        // Ignore the old stationary card after the switch
                        if (distanceFromOld > firstCardLockedBox.width * 0.3)
                        {
                            trackedSecondCardBox = playedCardBoxes[0];
                        }
                    }
                    else
                    {
                        double minDistance = std::numeric_limits<double>::max();
                        cv::Rect closest;

                        cv::Point target(
                            trackedSecondCardBox.x + trackedSecondCardBox.width / 2,
                            trackedSecondCardBox.y + trackedSecondCardBox.height / 2
                        );

                        for (const auto& box : playedCardBoxes)
                        {
                            cv::Point center(
                                box.x + box.width / 2,
                                box.y + box.height / 2
                            );

                            double d = cv::norm(center - target);

                            if (d < minDistance)
                            {
                                minDistance = d;
                                closest = box;
                            }
                        }

                        if (currentBox != closest)
                            continue;

                        trackedSecondCardBox = closest;
                    }
                }


                std::vector<CardDetected> recognizedCards =
                    recognizer.identifyCard(croppedcard);

                if (recognizedCards.empty()) {
                    continue;
                }

                // Keep all valid candidates from this detector box together.
                // The inner vector represents one observation and must not be counted
                // as multiple independent frames by the analyzer.
                std::vector<CardDetected> validCandidates;

                for (auto candidate : recognizedCards) {
                    if (isValidRecognizedCard(candidate.card)) {
                        // Position and frame are used to measure temporal stability.
                        candidate.bbox = safebox;
                        candidate.frameIndex = frameIndex;
                        validCandidates.push_back(candidate);
                    }
                }

                if (validCandidates.empty()) {
                    continue;
                }

                // Store the complete candidate set for this observation.
                // ErrorResolver may need the second or third candidate later.
                if (detection.classId == BRISCOLA_CLASS_ID) {
                    allBriscolaDetections.push_back({validCandidates});
                    frameBriscolaCandidates.insert(
                        frameBriscolaCandidates.end(),
                        validCandidates.begin(),
                        validCandidates.end()
                    );

                    continue;
                }

                if (detection.classId != PLAYED_CARD_CLASS_ID) {
                    continue;
                }

                // Detect transition from first played card to second played card
                if (!secondCardDetected)
                {
                    if (firstCardBox.empty())
                    {
                        // Lock the first played card and the side of its player.
                        firstCardBox = currentBox;
                        firstCardFrame = frameIndex;
                        firstCardSide = isNorthZone ? 0 : 1;
                    }
                }

                if (!secondCardDetected)
                {
                    // Until two boxes are present in the same frame, every
                    // recognized card still belongs to the first player.

                    if (firstCardSide == 0)
                    {
                        northDetections.push_back({validCandidates});

                        frameNorthCandidates.insert(
                            frameNorthCandidates.end(),
                            validCandidates.begin(),
                            validCandidates.end()
                        );

                        if (firstNorthFrame == -1) {
                            firstNorthFrame = frameIndex;
                        }
                    }
                    else
                    {
                        southDetections.push_back({validCandidates});

                        frameSouthCandidates.insert(
                            frameSouthCandidates.end(),
                            validCandidates.begin(),
                            validCandidates.end()
                        );

                        if (firstSouthFrame == -1) {
                            firstSouthFrame = frameIndex;
                        }
                    }
                }
                else
                {
                    // Assign new detections to the second player
                    if (firstCardSide == 0)
                    {
                        southDetections.push_back({validCandidates});

                        frameSouthCandidates.insert(
                            frameSouthCandidates.end(),
                            validCandidates.begin(),
                            validCandidates.end()
                        );

                        if (firstSouthFrame == -1) {
                            firstSouthFrame = frameIndex;
                        }
                    }
                    else
                    {
                        northDetections.push_back({validCandidates});

                        frameNorthCandidates.insert(
                            frameNorthCandidates.end(),
                            validCandidates.begin(),
                            validCandidates.end()
                        );

                        if (firstNorthFrame == -1) {
                            firstNorthFrame = frameIndex;
                        }
                    }
                }
            }

            if (PRINT_FRAME_DETECTIONS) {
                printFrameDetections(
                    roundNumber,
                    frameIndex,
                    frameNorthCandidates,
                    frameSouthCandidates,
                    frameBriscolaCandidates
                );
            }

            frameIndex++;
            scannedFrameCount++;
        }

        //round's data
        RoundPrediction currentRoundPred;
        currentRoundPred.round = roundNumber; 

        currentRoundPred.northDetected = getRankedCardsWithConfidence(northDetections);
        currentRoundPred.southDetected = getRankedCardsWithConfidence(southDetections);

        std::cout << "\nRound " << roundNumber << std::endl;
        std::cout << "First card locked:" << std::endl;
        if (firstCardSide == -1) {
            std::cout << "- player: UNKNOWN" << std::endl;
            std::cout << "- frame: -" << std::endl;
            std::cout << "- bbox: -" << std::endl;
        } else {
            std::cout << "- player: "
                      << (firstCardSide == 0 ? "NORTH" : "SOUTH")
                      << std::endl;
            std::cout << "- frame: " << firstCardFrame << std::endl;
            std::cout << "- bbox: "
                      << firstCardBox.x << ","
                      << firstCardBox.y << ","
                      << firstCardBox.width << ","
                      << firstCardBox.height << std::endl;
        }

        std::cout << "Second card detected:" << std::endl;
        if (!secondCardDetected) {
            std::cout << "- frame: -" << std::endl;
            std::cout << "- bbox: -" << std::endl;
        } else {
            std::cout << "- frame: " << secondCardFrame << std::endl;
            std::cout << "- bbox: "
                      << secondCardBox.x << ","
                      << secondCardBox.y << ","
                      << secondCardBox.width << ","
                      << secondCardBox.height << std::endl;
        }

        std::cout << "Final candidates:" << std::endl;
        printCardCandidates("North", currentRoundPred.northDetected);
        printCardCandidates("South", currentRoundPred.southDetected);

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
        prediction.rounds.push_back(currentRoundPred);
    }



    prediction.briscolaDetected =
    getRankedCardsWithConfidence(
        allBriscolaDetections
    );

    // Use the same confidence scale for every card in the game.
    double maxCardConfidence = 0.0;
    for (const auto& round : prediction.rounds) {
        for (const auto& candidate : round.northDetected) {
            maxCardConfidence = std::max(maxCardConfidence, candidate.confidence);
        }
        for (const auto& candidate : round.southDetected) {
            maxCardConfidence = std::max(maxCardConfidence, candidate.confidence);
        }
    }
    for (const auto& candidate : prediction.briscolaDetected) {
        maxCardConfidence = std::max(maxCardConfidence, candidate.confidence);
    }

    if (maxCardConfidence > 0.0) {
        for (auto& round : prediction.rounds) {
            for (auto& candidate : round.northDetected) {
                candidate.confidence /= maxCardConfidence;
            }
            for (auto& candidate : round.southDetected) {
                candidate.confidence /= maxCardConfidence;
            }
        }
        for (auto& candidate : prediction.briscolaDetected) {
            candidate.confidence /= maxCardConfidence;
        }
    }

    std::cout << "\n================ GamePrediction ================" << std::endl;
    std::cout << "Rounds: " << prediction.rounds.size() << std::endl;

    for (const auto& round : prediction.rounds) {
        std::cout << "Round " << round.round << std::endl;
        printCardCandidates(" North top 3", round.northDetected);
        printCardCandidates(" South top 3", round.southDetected);

        std::cout << " Leader: ";
        if (round.leaderDetected.empty()) {
            std::cout << "UNKNOWN" << std::endl;
        } else {
            std::cout << playerToString(round.leaderDetected[0].player)
                      << std::endl;
        }

        std::cout << "-----------------------------------------------" << std::endl;
    }

    printCardCandidates("Briscola top 3", prediction.briscolaDetected);
    std::cout << "=======================================================" << std::endl;

    Game game = GameEngine::createGame(prediction);

    ValidationResult before = Validator::validate(game);

    int cardCorrections = 0;
    int briscolaCorrections = 0;
    int leaderCorrections = 0;

    if (ENABLE_ERROR_CORRECTION) {

        cardCorrections = ErrorResolver::resolveCardIssues(game);

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
    }

    ValidationResult finalValidation = Validator::validate(game);

    std::cout << "\n================ CORRECTIONS ================" << std::endl;
    std::cout << "Card issues: "
              << before.cardIssues.size()
              << " -> "
              << finalValidation.cardIssues.size()
              << std::endl;
    std::cout << "Leader issues: "
              << finalValidation.leaderIssues.size()
              << std::endl;
    std::cout << "Corrections: cards=" << cardCorrections
              << ", briscola=" << briscolaCorrections
              << ", leaders=" << leaderCorrections
              << std::endl;
    std::cout << "==============================================" << std::endl;

    OutputWriter::writeTxt(
        game,
        input.resultsFolder + input.gameName + "_output.txt"
    );

    OutputWriter::writeCsv(
        game,
        input.resultsFolder + input.gameName + "_results.csv"
    );

    std::cout << "\n================ METRICS ================" << std::endl;
    std::cout << "Ground truth: " << input.groundTruthPath << std::endl;

    try {
        MetricsResult metrics =
            MetricsEvaluator::evaluate(
                game,
                input.groundTruthPath
            );

        MetricsEvaluator::printMetrics(metrics);

        MetricsEvaluator::writeMetrics(
            metrics,
            input.resultsFolder + input.gameName + "_metrics.txt"
        );
        std::cout << "==========================================" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "METRICS ERROR: "
                  << e.what()
                  << std::endl;
        return 1;
    }

    return 0;
}
