// Luca Ferraro

#ifndef METRICS_EVALUATOR_H
#define METRICS_EVALUATOR_H

#include "gameModels.h"
#include <string>


// Counts the correct predictions in the four groups printed by the evaluator.
struct MetricsResult {

    // Two card positions are checked for every round.
    int correctCards = 0;

    // The leader and the winner are checked for every round.
    int correctPlayers = 0;

    // The single briscola card is checked once for the whole game.
    int correctBriscola = 0;

    // Three values are checked: final winner, North score and South score.
    int correctResult = 0;
};


// Compares the computed game with the ground-truth CSV and writes the summary
// either to the console or to a separate metrics file.
class MetricsEvaluator {

public:

    // Reads the ground truth and returns the number of correct predictions.
    static MetricsResult evaluate(const Game& game, const std::string& groundTruthPath);

    // Prints the metrics in the same format used in the results file.
    static void printMetrics(const MetricsResult& metrics);

    // Saves the metrics summary to the requested text file.
    static void writeMetrics(const MetricsResult& metrics, const std::string& filePath);
};

#endif