// Luca Ferraro

#include "confidence.h"

#include <algorithm>

// Returns the average SIFT score for a card across the observations.
double calculateCardConfidence(
    const std::vector<CardDetected>& detections
)
{
    if (detections.empty())
        return 0.0;

    double scoreSum = 0.0;

    for (const auto& detection : detections)
    {
        scoreSum += detection.confidence;
    }

    return scoreSum / static_cast<double>(detections.size());
}

void normalizeCardConfidences(GamePrediction& prediction)
{
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
}
