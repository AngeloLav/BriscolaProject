// Author: Camilla Bellantuono
#ifndef RECOGNIZER_HPP
#define RECOGNIZER_HPP

#include "../model/gameModels.h"
#include <string>
#include <vector>
#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/calib3d.hpp>

struct refCard {
    Card cardInfo;
    std::string filename;
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
};

class CardRecognizer {
public:
    //loads the cards and calculates the Sift's descriptors
    CardRecognizer(const std::string& referenceFolderPath);
    //recives tje card's roi and returns the recognized card
    Card identifyCard(const cv::Mat& croppedcard);

private:
    std::vector<refCard> referenceDeck;
    cv::Ptr<cv::SIFT> siftDetector;
    void loadReferenceDeck(const std::string& folderPath);
    //extracts type and value from file name of cards
    Card parseCardInfoFromFilename(const std::string& filename);
};
#endif