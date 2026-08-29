#include "gameModels.h"
#include <string>


struct MetricsResult {

    int correctCards = 0;
    int correctPlayers = 0;
    int correctBriscola = 0;
    int correctResult = 0;
};


class MetricsEvaluator {

public:

    static MetricsResult evaluate(const Game& game, const std::string& groundTruthPath);

    static void printMetrics(const MetricsResult& metrics);

    static void writeMetrics(const MetricsResult& metrics, const std::string& filePath);
};