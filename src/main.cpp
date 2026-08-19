// Standard include
#include <iostream>
#include <string>

#include <vector>

// OpenCV
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

// Model libraries
#include <opencv2/dnn/dnn.hpp>


int main(int argc, char** argv) {
   
    if (argc < 2) {
        std::cout << "Use the program with: ./briscola <video.mp4>" << std::endl;
        return 1;
    }

    // Analyzing video and check to see if the path is correct
    std::string videoPath = argv[1];
    cv::VideoCapture video(videoPath);
    if (!video.isOpened()) {
        std::cout << "Unable to open video: " << videoPath << std::endl;
        return 1;
    }

    // Loading the model
    cv::dnn::Net net = cv::dnn::readNetFromONNX("model/best.onnx");
    if (net.empty())
    {
        std::cerr << "Unable to load ONNX model\n";
        return 1;
    }
    std::cout << "Model loaded successfully\n";


    cv::Mat frame;
    int frameIndex {0};

    // Main loop
    while (video.read(frame)){

        std::cout << "Frame: " << frameIndex << " | Size:\nx = " << frame.cols << "\ny = " << frame.rows << std::endl;

        cv::Mat blob = cv::dnn::blobFromImage(
        frame,
        1.0 / 255.0,               
        cv::Size(640, 640),
        cv::Scalar(),
        // BGR to RGB
        true,                      
        false
        );

        const int numClasses = 2;

        std::vector<cv::Rect> boxes;
        std::vector<float> confidences;
        std::vector<int> classIds;

        float xFactor = static_cast<float>(frame.cols) / 640.0f;
        float yFactor = static_cast<float>(frame.rows) / 640.0f;

        net.setInput(blob);

        cv::Mat output = net.forward();

        cv::Mat predictions(
            output.size[1],
            output.size[2],
            CV_32F,
            output.ptr<float>()
        );  
        predictions = predictions.t();


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

            if (confidence < CONF_THRESHOLD)
                continue;

            int left = static_cast<int>((cx - w / 2.0f) * xFactor);
            int top  = static_cast<int>((cy - h / 2.0f) * yFactor);

            int width  = static_cast<int>(w * xFactor);
            int height = static_cast<int>(h * yFactor);

            boxes.emplace_back(left, top, width, height);
            confidences.push_back(confidence);
            classIds.push_back(classId);
        }

        std::vector<int> indices;

        cv::dnn::NMSBoxes(
            boxes,
            confidences,
            CONF_THRESHOLD,
            0.45f,
            indices
        );

        std::vector<std::string> classNames = {
            "Briscola-Cards",
            "Played-Card"
        };

        for (int index : indices)
        {
            const cv::Rect& box = boxes[index];

            cv::rectangle(
                frame,
                box,
                cv::Scalar(0, 255, 0),
                2
            );

            std::string label =
                classNames[classIds[index]]
                + " "
                + std::to_string(confidences[index]).substr(0, 4);

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

        cv::imshow("Briscola video", frame);

        int key = cv::waitKey(30);

        // If user presses ESC the video stops
        if (key == 27) break;

        frameIndex++;
    }
    
    return 0;
}


