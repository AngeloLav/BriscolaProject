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

        Detector(const std::string& modelPath);
        std::vector<Detection> detect(const cv::Mat& frame);

        // Functions
        void drawDetections(
        cv::Mat& frame,
        const std::vector<Detection>& detections
        );
        
    private:

        cv::dnn::Net net;
};

#endif