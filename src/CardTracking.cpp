// Luca Ferraro

#include "CardTracking.h"

#include <algorithm>
#include <limits>

namespace {

constexpr double OLD_CARD_DISTANCE_RATIO = 0.3;

} // namespace

std::vector<cv::Rect> findPlayedCardBoxes(
    const std::vector<Detection>& detections,
    const cv::Mat& frame,
    int playedCardClassId
) {
    // Count the played-card boxes in this frame before processing them. 2 boxes are needed to make the switch
    std::vector<cv::Rect> playedCardBoxes;
    for (const auto& detection : detections) {
        if (detection.classId != playedCardClassId) {
            continue;
        }

        cv::Rect box = detection.box & cv::Rect(0, 0, frame.cols, frame.rows);

        if (box.width > 0 && box.height > 0) {
            playedCardBoxes.push_back(box);
        }
    }

    return playedCardBoxes;
}

void updateLastPlayedCardBox(
    const std::vector<cv::Rect>& playedCardBoxes,
    CardTrackingState& tracking
) {
    if (!tracking.secondCardDetected && playedCardBoxes.size() == 1) {
        tracking.lastPlayedCardBox = playedCardBoxes[0];
    }
}

void switchToSecondCard(
    const std::vector<cv::Rect>& playedCardBoxes,
    const cv::Mat& frame,
    int frameIndex,
    double secondCardDistanceThreshold,
    CardTrackingState& tracking,
    std::vector<CardObservation>& northDetections,
    std::vector<CardObservation>& southDetections
) {
    // The box farthest from the previous position is the new card.
    if (!tracking.secondCardDetected &&
        !tracking.firstCardBox.empty() &&
        playedCardBoxes.size() >= 2) {
        cv::Rect previousBox = tracking.lastPlayedCardBox.empty()
            ? tracking.firstCardBox
            : tracking.lastPlayedCardBox;
        cv::Rect oldCardBox;
        cv::Rect newCardBox;
        double minDistance = std::numeric_limits<double>::max();
        double maxDistance = -1.0;

        for (const auto& box : playedCardBoxes) {
            double distance =
                cv::norm(
                    cv::Point(box.x, box.y) -
                    cv::Point(previousBox.x, previousBox.y)
                );

            if (distance < minDistance) {
                minDistance = distance;
                oldCardBox = box;
            }

            if (distance > maxDistance) {
                maxDistance = distance;
                newCardBox = box;
            }
        }

        bool newCardIsNorth = newCardBox.y + newCardBox.height / 2 < frame.rows / 2;
        int newCardSide = newCardIsNorth ? 0 : 1;

        // If both boxes are South and the new one is farther South,
        // the first card was detected after crossing the centre.
        bool wrongFirstSide =
            tracking.firstCardSide == 1 &&
            newCardSide == 1 &&
            newCardBox.y + newCardBox.height / 2 > oldCardBox.y + oldCardBox.height / 2;

        if (maxDistance > secondCardDistanceThreshold &&
            (newCardSide != tracking.firstCardSide || wrongFirstSide)) {
            if (wrongFirstSide) {
                std::swap(northDetections, southDetections);
                std::swap(tracking.firstNorthFrame, tracking.firstSouthFrame);
                tracking.firstCardSide = 1 - tracking.firstCardSide;
            }

            tracking.secondCardDetected = true;
            tracking.secondCardBox = newCardBox;
            tracking.trackedSecondCardBox = newCardBox;
            tracking.secondCardFrame = frameIndex;
            tracking.previousSecondCardBox = tracking.secondCardBox;

            tracking.firstCardLockedBox = oldCardBox;
        }
    }
}

bool followSecondCard(
    const cv::Rect& currentBox,
    const std::vector<cv::Rect>& playedCardBoxes,
    CardTrackingState& tracking
) {
    // The caller has already confirmed that the second card was found.
    if (playedCardBoxes.size() == 1)
    {
        cv::Point detectedCenter(
            playedCardBoxes[0].x + playedCardBoxes[0].width / 2,
            playedCardBoxes[0].y + playedCardBoxes[0].height / 2
        );

        cv::Point oldCardCenter(
            tracking.firstCardLockedBox.x + tracking.firstCardLockedBox.width / 2,
            tracking.firstCardLockedBox.y + tracking.firstCardLockedBox.height / 2
        );

        double distanceFromOld =
            cv::norm(detectedCenter - oldCardCenter);

        // Ignore the old stationary card after the switch
        if (distanceFromOld > tracking.firstCardLockedBox.width * OLD_CARD_DISTANCE_RATIO)
        {
            tracking.trackedSecondCardBox = playedCardBoxes[0];
        }
    }
    else
    {
        double minDistance = std::numeric_limits<double>::max();
        cv::Rect closest;

        cv::Point target(
            tracking.trackedSecondCardBox.x + tracking.trackedSecondCardBox.width / 2,
            tracking.trackedSecondCardBox.y + tracking.trackedSecondCardBox.height / 2
        );

        for (const auto& box : playedCardBoxes)
        {
            cv::Point center(
                box.x + box.width / 2,
                box.y + box.height / 2
            );

            double d = cv::norm(center - target);

            if (d < minDistance)
            {
                minDistance = d;
                closest = box;
            }
        }

        if (currentBox != closest)
            return false;

        tracking.trackedSecondCardBox = closest;
    }

    return true;
}

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
) {
    // Detect transition from first played card to second played card
    if (!tracking.secondCardDetected)
    {
        if (tracking.firstCardBox.empty())
        {
            // Lock the first played card and the side of its player.
            tracking.firstCardBox = currentBox;
            tracking.firstCardFrame = frameIndex;
            tracking.firstCardSide = isNorthZone ? 0 : 1;
        }
    }

    if (!tracking.secondCardDetected)
    {
        // Until two boxes are present in the same frame, every
        // recognized card still belongs to the first player.
        if (tracking.firstCardSide == 0)
        {
            northDetections.push_back({validCandidates});

            frameNorthCandidates.insert(
                frameNorthCandidates.end(),
                validCandidates.begin(),
                validCandidates.end()
            );

            if (tracking.firstNorthFrame == -1) {
                tracking.firstNorthFrame = frameIndex;
            }
        }
        else
        {
            southDetections.push_back({validCandidates});

            frameSouthCandidates.insert(
                frameSouthCandidates.end(),
                validCandidates.begin(),
                validCandidates.end()
            );

            if (tracking.firstSouthFrame == -1) {
                tracking.firstSouthFrame = frameIndex;
            }
        }
    }
    else
    {
        // Assign new detections to the second player
        if (tracking.firstCardSide == 0)
        {
            southDetections.push_back({validCandidates});

            frameSouthCandidates.insert(
                frameSouthCandidates.end(),
                validCandidates.begin(),
                validCandidates.end()
            );

            if (tracking.firstSouthFrame == -1) {
                tracking.firstSouthFrame = frameIndex;
            }
        }
        else
        {
            northDetections.push_back({validCandidates});

            frameNorthCandidates.insert(
                frameNorthCandidates.end(),
                validCandidates.begin(),
                validCandidates.end()
            );

            if (tracking.firstNorthFrame == -1) {
                tracking.firstNorthFrame = frameIndex;
            }
        }
    }
}
