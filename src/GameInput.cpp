#include "GameInput.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>

// Extracts the numeric round from filenames such as round1.mp4 and round10.mp4
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

// Reads the game folder, ground truth CSV and round videos
bool loadGameInput(int argc, char** argv, GameInput& input) {
    if (argc != 2) {
        std::cout << "Use the program with: ./briscola <game_folder>"
                  << std::endl;
        return false;
    }

    input.gameFolder = argv[1];

    if (!input.gameFolder.empty() &&
        input.gameFolder.back() != '/' &&
        input.gameFolder.back() != '\\') {
        input.gameFolder += "/";
    }

    std::string folderWithoutSlash = input.gameFolder.substr(0, input.gameFolder.size() - 1);
    size_t lastSlash = folderWithoutSlash.find_last_of("/\\");

    input.gameName = (lastSlash == std::string::npos)
            ? folderWithoutSlash
            : folderWithoutSlash.substr(lastSlash + 1);

    std::string dataFolder = (lastSlash == std::string::npos)
            ? ""
            : folderWithoutSlash.substr(0, lastSlash + 1);

    input.resultsFolder = dataFolder + "results/";
    std::filesystem::create_directories(input.resultsFolder);

    // Find the ground truth CSV inside the game folder.
    std::vector<cv::String> csvFiles;

    cv::glob(input.gameFolder + "*.csv", csvFiles, false);

    if (csvFiles.empty()) {
        std::cerr << "No CSV ground truth found in: "
                  << input.gameFolder << std::endl;
        return false;
    }

    if (csvFiles.size() > 1) {
        std::cerr << "More than one CSV found in: "
                  << input.gameFolder << std::endl;
        return false;
    }

    input.groundTruthPath = csvFiles[0];

    // Find and sort videos by round number instead of alphabetically.
    cv::glob(input.gameFolder + "*.mp4", input.videoFiles, false);

    std::sort(
        input.videoFiles.begin(),
        input.videoFiles.end(),
        [](const cv::String& lhs, const cv::String& rhs) {
            const int lhsRound = getRoundNumberFromVideoPath(lhs);
            const int rhsRound = getRoundNumberFromVideoPath(rhs);

            if (lhsRound != rhsRound) {
                return lhsRound < rhsRound;
            }

            return lhs < rhs;
        }
    );

    std::cout << "Video files found: " << input.videoFiles.size() << std::endl;
    if (input.videoFiles.size() != 20) {
        std::cerr << "Warning: expected 20 round videos, found "
                  << input.videoFiles.size() << std::endl;
    }

    return true;
}
