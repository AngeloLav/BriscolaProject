#include "confidence.h"

#include <cmath>


double calculateCardConfidence(
    const std::vector<CardDetected>& detections
)
{
    if (detections.empty())
        return 0.0;


    double scoreSum = 0.0;
    double movementSum = 0.0;


    for (size_t i = 0; i < detections.size(); i++)
    {
        scoreSum += detections[i].confidence;


        if (i > 0)
        {
            cv::Point previousCenter(
                detections[i - 1].bbox.x + detections[i - 1].bbox.width / 2,
                detections[i - 1].bbox.y + detections[i - 1].bbox.height / 2
            );


            cv::Point currentCenter(
                detections[i].bbox.x + detections[i].bbox.width / 2,
                detections[i].bbox.y + detections[i].bbox.height / 2
            );


            movementSum += cv::norm(currentCenter - previousCenter);
        }
    }


    double meanScore =
        scoreSum / static_cast<double>(detections.size());


    double averageMovement = 0.0;

    if (detections.size() > 1)
    {
        averageMovement =
            movementSum /
            static_cast<double>(detections.size() - 1);
    }


    // Stable cards receive a higher confidence score
    double stabilityFactor =
        1.0 / (1.0 + averageMovement);


    return meanScore * stabilityFactor;
}
