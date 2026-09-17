// Luca Ferraro

#include "confidence.h"

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