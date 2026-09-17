// Luca Ferraro

#ifndef CARD_TRACKING_H
#define CARD_TRACKING_H

#include <vector>

#include <opencv2/core.hpp>

#include "detector.hpp"
#include "gameModels.h"

// Keeps the small amount of state needed while a video changes from one
// visible played card to two cards on the table.
struct CardTrackingState {
    // First valid detection determines which player starts the round.
    int firstNorthFrame = -1;
    int firstSouthFrame = -1;

    // Once a second independent box is found, detections are assigned
    // to the other player and the first card is no longer updated.
    bool secondCardDetected = false;

    // Position and frame of the first card detected in the round.
    cv::Rect firstCardBox;
    int firstCardFrame = -1;
    int firstCardSide = -1;

    // Position and frame where the second card was detected.
    cv::Rect secondCardBox;
    int secondCardFrame = -1;

    // Last positions used to decide whether a new box is the second card or
    // another detection of the card that was already on the table.
    cv::Rect lastPlayedCardBox;
    cv::Rect trackedSecondCardBox;
    cv::Rect firstCardLockedBox;
    cv::Rect previousSecondCardBox;
};

// Collects the played-card boxes detected in the current frame.
// Boxes are clipped to the frame so that they can safely be used as ROIs.
std::vector<cv::Rect> findPlayedCardBoxes(
    const std::vector<Detection>& detections,
    const cv::Mat& frame,
    int playedCardClassId
);

// Keeps the position of the only visible played card until the switch occurs.
void updateLastPlayedCardBox(
    const std::vector<cv::Rect>& playedCardBoxes,
    CardTrackingState& tracking
);

// Looks for the second card and switches the stored observations to the
// correct player when the new card is found.
// If the second card is further north than the first card which had been 
// identified as north, swap them, and vice versa. 
void switchToSecondCard(
    const std::vector<cv::Rect>& playedCardBoxes,
    const cv::Mat& frame,
    int frameIndex,
    double secondCardDistanceThreshold,
    CardTrackingState& tracking,
    std::vector<CardObservation>& northDetections,
    std::vector<CardObservation>& southDetections
);

// Keeps only the detection that belongs to the second card after the switch.
// Returns false for boxes that still belong to the first card.
bool followSecondCard(
    const cv::Rect& currentBox,
    const std::vector<cv::Rect>& playedCardBoxes,
    CardTrackingState& tracking
);

// Stores a recognized played card on the side that currently owns it.
// The frame vectors are used only for the optional per-frame console output.
void storePlayedCardObservation(
    const std::vector<CardDetected>& validCandidates,
    const cv::Rect& currentBox,
    bool isNorthZone,
    int frameIndex,
    CardTrackingState& tracking,
    std::vector<CardObservation>& northDetections,
    std::vector<CardObservation>& southDetections,
    std::vector<CardDetected>& frameNorthCandidates,
    std::vector<CardDetected>& frameSouthCandidates
);

#endif