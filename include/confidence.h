#pragma once

#include <vector>
#include "gameModels.h"

double calculateCardConfidence(
    const std::vector<CardDetected>& detections
);
