// Luca Ferraro

#ifndef GAME_INPUT_H
#define GAME_INPUT_H

#include <string>
#include <vector>

#include <opencv2/core.hpp>

// All paths and files needed to process one game.
struct GameInput {
    // Folder containing the round videos and the ground-truth CSV.
    std::string gameFolder;

    // Name used when creating files in the results folder.
    std::string gameName;

    // CSV file used by MetricsEvaluator for the final comparison.
    std::string groundTruthPath;

    // Folder where the text output, CSV output and metrics are saved.
    std::string resultsFolder;

    // Round videos sorted by their numeric round suffix.
    std::vector<cv::String> videoFiles;
};

// Reads the command-line folder, finds its CSV and loads its 20 round videos.
// Returns false if the input is missing, ambiguous or incomplete.
bool loadGameInput(int argc, char** argv, GameInput& input);

// Extracts the numeric round from a video filename. When no valid number is
// found, the function returns the largest representable int as a fallback mark.
int getRoundNumberFromVideoPath(const cv::String& videoPath);

#endif