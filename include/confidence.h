// Luca Ferraro

#ifndef CONFIDENCE_H
#define CONFIDENCE_H

#include <vector>
#include "gameModels.h"

double calculateCardConfidence(
    const std::vector<CardDetected>& detections
);

#endif