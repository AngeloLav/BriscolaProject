// Author: Angelo Lavarini

#include "detector.hpp"

#include <iostream>
#include <opencv2/imgproc.hpp>

Detector::Detector(const std::string& modelPath) {
    net = cv::dnn::readNetFromONNX(modelPath);

    if (net.empty())
    {
        throw std::runtime_error(
            "Unable to load ONNX model: " + modelPath + "\n"
        );
    }
    std::cout << "Model loaded successfully\n"; 
}

std::vector<Detection> Detector::detect(const cv::Mat& frame) {
    cv::Mat blob = cv::dnn::blobFromImage(
            frame,
            1.0 / 255.0,        // This is normalization of pixel intensity values between 0-1, since yolo works with this format          
            cv::Size(640, 640), // Size required by Yolo
            cv::Scalar(),       // Subtracts mean values from channels; here its initialized all to (0, 0, 0, 0)
            true,               // BGR to RGB                 
            false               // Cropping
        );

        // NOTE: i trained the model on a dataset whose images where all resized by stretching them to 640x640, 
        // already ready for the Yolo format. The resizing performed in blobFromImage is coherent to the format used in the dataset

        // Vectors use to store data about each valid prediction
        const int numClasses = 2;
        std::vector<cv::Rect> boxes;
        std::vector<float> confidences;
        std::vector<int> classIds;

        // We will need these 2 factors to invert the stretching required by Yolo (from 640x640 to original size of the frame)
        float xFactor = static_cast<float>(frame.cols) / 640.0f;
        float yFactor = static_cast<float>(frame.rows) / 640.0f;

        net.setInput(blob);

        // Model inference
        cv::Mat output = net.forward();

        cv::Mat predictions(
            output.size[1],     // 6 values: cx, cy, w, h, score class 0, score class 1
            output.size[2],     // Total candidate detections 8400
            CV_32F,
            output.ptr<float>() // We can use this pointer to access address of each detection
        );  
        // We apply the transpose so that every row is a prediction for each candidate
        predictions = predictions.t();

        // Filtering the candidates:
        const float CONF_THRESHOLD = 0.25f;

        for (int i = 0; i < predictions.rows; ++i)
        {
            float* data = predictions.ptr<float>(i);

            float cx = data[0];
            float cy = data[1];
            float w  = data[2];
            float h  = data[3];

            float score0 = data[4];
            float score1 = data[5];

            int classId;
            float confidence;

            if (score0 > score1)
            {
                classId = 0;
                confidence = score0;
            }
            else
            {
                classId = 1;
                confidence = score1;
            }

            // Better filtering here boxes with low confidence score
            if (confidence < CONF_THRESHOLD) continue;

            // Convert into rect box format
            int left = static_cast<int>((cx - w / 2.0f) * xFactor);
            int top  = static_cast<int>((cy - h / 2.0f) * yFactor);

            int width  = static_cast<int>(w * xFactor);
            int height = static_cast<int>(h * yFactor);

            // Save the data in these 3 vectors
            boxes.emplace_back(left, top, width, height); // Builds the rect inside the vector
            confidences.push_back(confidence);
            classIds.push_back(classId);
        }

        std::vector<int> indices;

        cv::dnn::NMSBoxes(
            boxes,          // Our previous rects
            confidences,    // Respective confidences values
            CONF_THRESHOLD, // Confidence treshold
            0.45f,          // IoU
            indices         // Vector where survivor predictions are saved
        );

        std::vector<Detection> detections;

        for (int index : indices) {
            
            Detection detection;

            detection.box = boxes[index];
            detection.confidence = confidences[index];
            detection.classId = classIds[index];
            detections.push_back(detection);
        }

        return detections;
}

// Function used to draw BB; used during development for visualizing models inference
void Detector::drawDetections(
    cv::Mat& frame,
    const std::vector<Detection>& detections
) {
    const std::vector<std::string> classNames = {
        "Briscola-Cards",
        "Played-Card"
    };

    for (const Detection& detection : detections)
    {
        const cv::Rect& box = detection.box;

        cv::rectangle(
            frame,
            box,
            cv::Scalar(0, 255, 0),
            2
        );

        std::string label =
            classNames[detection.classId] + " " +
            std::to_string(detection.confidence).substr(0, 4);

        cv::putText(
            frame,
            label,
            cv::Point(box.x, std::max(20, box.y - 5)),
            cv::FONT_HERSHEY_SIMPLEX,
            0.6,
            cv::Scalar(0, 255, 0),
            2
        );
    }
}