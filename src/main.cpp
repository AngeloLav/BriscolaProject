#include "detector.hpp"

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


#include "JsonReader.h"
#include "GameEngine.h"


int main(int argc, char** argv) {
 /*  
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

    cv::Mat frame;
    int frameIndex {0};

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


        // Used for development, maybe it will be commented in the final project
        detector.drawDetections(frame, detections);

        // Showing each frame
        cv::imshow("Briscola video", frame);

        int key = cv::waitKey(30);

        // If user presses ESC the video stops
        if (key == 27) break;

        frameIndex++;
    }
*/

    
    // Read the json for debugging (probabilmente sta parte di json sarà meglio toglierla, ora mi serve per testare più partite plausibile)
     try {

        GamePrediction prediction =
            JsonReader::readGamePrediction(
                "json_games/briscola_game_test_clean_v2.json"
            );

        std::cout
            << "Rounds loaded: "
            << prediction.rounds.size()
            << std::endl;

        std::cout
            << "Briscola candidates: "
            << prediction.briscolaDetected.size()
            << std::endl;

    }
    catch (const std::exception& e) {

        std::cerr
            << "Error: "
            << e.what()
            << std::endl;

        return 1;
    }


    GamePrediction prediction =
    JsonReader::readGamePrediction(
        "briscola_game_test_clean_v2.json"
    );

    Game game = GameEngine::createGame(prediction);

    std::cout << "Rounds: "
            << game.rounds.size()
            << std::endl;

    std::cout << "North score: "
            << game.northScore
            << std::endl;

    std::cout << "South score: "
            << game.southScore
            << std::endl;

    std::cout << "Total points: "
            << game.northScore + game.southScore
            << std::endl;

    return 0;
}


