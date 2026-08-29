#include "detector.hpp"
#include "recognizer.hpp"
#include "../model/gameModels.h"
#include "analyzer.hpp"

// Standard include
#include <iostream>
#include <string>
#include <vector>

// OpenCV
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

// Model libraries
#include <opencv2/dnn/dnn.hpp>


int main(int argc, char** argv) {
   
    if (argc < 2) {
        std::cout << "Use the program with: ./briscola <video.mp4>" << std::endl;
        return 1;
    }

    // Analyzing video and check to see if the path is correct
    std::string videoPath = argv[1];
    cv::VideoCapture video(videoPath);
    if (!video.isOpened()) {
        std::cout << "Unable to open video: " << videoPath << std::endl;
        return 1;
    }

    // Loading the model
    Detector detector("model/best.onnx");
    CardRecognizer recognizer("Briscola_Trentine");
    cv::namedWindow("Briscola video", cv::WINDOW_NORMAL);
    cv::resizeWindow("Briscola video", 1280, 720);

    cv::Mat frame;
    int frameIndex {0};

    //vectors to store the detected cards for each player and the briscola card
    std::vector<Card> northDetections;
    std::vector<Card> southDetections;
    std::vector<Card> briscolaDetections;

    // Main loop
    while (video.read(frame)){

        std::cout << "Frame: " << frameIndex << " | Size:\nx = " << frame.cols << "\ny = " << frame.rows << std::endl;

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
            cv::Rect safebox=detection.box & cv::Rect(0, 0, frame.cols, frame.rows); // This is to avoid the case where the BB is partially outside the frame
            if(safebox.width<=0||safebox.height<=0) continue; // This is to avoid the case where the BB is completely outside the frame
            cv::Mat croppedcard=frame(safebox);
            Card recognizedCard=recognizer.identifyCard(croppedcard);
            std::cout <<"Card found: "<< detection.classId
                      << ", value= " << recognizedCard.value << std::endl;
            if(recognizedCard.value==0) continue; 
            int centerY=safebox.y+safebox.height/2;
            int centerX=safebox.x+safebox.width/2;
            if(centerX>frame.cols*0.65||centerX<frame.cols*0.20){
                briscolaDetections.push_back(recognizedCard);
            }else if(centerY<frame.rows/2){
                northDetections.push_back(recognizedCard);
            }else{
                southDetections.push_back(recognizedCard);
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
        cv::imshow("Briscola video", frame);

        if (cv::waitKey(30) == 27) break;
    }








// --- COSTRUZIONE DEI DATI PER IL TUO COMPAGNO ---
    roundPrediction currentRoundPred;
    currentRoundPred.round = 1; // Numero del round/video corrente

    // Calcola le carte candidate con le relative confidence per North e South
    currentRoundPred.northDetected = getRankedCardsWithConfidence(northDetections);
    currentRoundPred.southDetected = getRankedCardsWithConfidence(southDetections);

    // Esempio per il Leader con confidence (puoi integrarla in base alla logica di rilevamento leader)
    PlayerDetected leaderPred;
    leaderPred.player = Player::NORTH;
    leaderPred.confidence = 1.0; 
    currentRoundPred.leaderDetected.push_back(leaderPred);

    // Anche per la Briscola puoi ottenere le candidate con confidence
    std::vector<CardDetected> briscolaCandidates = getRankedCardsWithConfidence(briscolaDetections);
    // ------------------------------------------------












    Card finalNorthCard=getMostFreqCard(northDetections);
    Card finalSouthCard=getMostFreqCard(southDetections);
    Card finalBriscola=getMostFreqCard(briscolaDetections);

    Player leader=Player::NORTH;
    Player winner=detWinner(finalNorthCard, finalSouthCard, finalBriscola, leader);
    
    //calculate points
    int roundPoints = getCardPoints(finalNorthCard.value) + getCardPoints(finalSouthCard.value);

    std::cout << "\n================ GAME PREDICTION DATA ================" << std::endl;
    
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
    
    return 0;
}


