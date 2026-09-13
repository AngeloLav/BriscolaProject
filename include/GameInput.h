#ifndef GAME_INPUT_H
#define GAME_INPUT_H

#include <string>
#include <vector>

#include <opencv2/core.hpp>

struct GameInput {
    std::string gameFolder;
    std::string gameName;
    std::string groundTruthPath;
    std::string resultsFolder;
    std::vector<cv::String> videoFiles;
};

bool loadGameInput(int argc, char** argv, GameInput& input);
int getRoundNumberFromVideoPath(const cv::String& videoPath);

#endif
