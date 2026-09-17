// Luca Ferraro

#ifndef CONFIDENCE_H
#define CONFIDENCE_H

#include <vector>
#include "gameModels.h"

// Calculates the average recognition score for all observations of one card
// that come from different frames of the same video.
double calculateCardConfidence(
    const std::vector<CardDetected>& detections
);

// Puts card confidences on a common scale across the whole game prediction.
// This is done after all rounds have been combined, so candidates from one
// round can be compared with candidates from another round.
void normalizeCardConfidences(GamePrediction& prediction);

#endif