// Author: Angelo Lavarini

#ifndef DETECTOR_HPP
#define DETECTOR_HPP

#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/dnn/dnn.hpp>

struct Detection {
    cv::Rect box;
    float confidence;
    int classId;
};

class Detector {
    public:

        // Responsible of loading the model
        Detector(const std::string& modelPath);
        // Responsible for making detections (uses the model, makes predictions, filter preditions, uses NMS)
        std::vector<Detection> detect(const cv::Mat& frame);
        // Draws BB on each frame, used for development
        void drawDetections(
        cv::Mat& frame,
        const std::vector<Detection>& detections
        );

    private:

        cv::dnn::Net net;
};

#endif