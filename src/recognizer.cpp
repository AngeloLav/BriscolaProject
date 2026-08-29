// Author: Camilla Bellantuono
#include "recognizer.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

namespace fs = std::filesystem;

CardRecognizer::CardRecognizer(const std::string& referenceFolderPath) {
    siftDetector = cv::SIFT::create();
    loadReferenceDeck(referenceFolderPath);
}

void CardRecognizer::loadReferenceDeck(const std::string& folderPath) {
    std::cout << "Loading reference deck from: " << folderPath << std::endl;
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            std::string filePath=entry.path().string();
            std::string filename=entry.path().filename().string();
            cv::Mat refImg=cv::imread(filePath, cv::IMREAD_GRAYSCALE);
            if (refImg.empty()) continue;

            refCard ref;
            ref.filename = filename;
            ref.cardInfo = parseCardInfoFromFilename(filename);

            siftDetector->detectAndCompute(refImg, cv::noArray(), ref.keypoints, ref.descriptors);
            if(!ref.descriptors.empty()) {
                referenceDeck.push_back(ref);
            }
        }  
    }
    std::cout<<"Loaded with success: " << referenceDeck.size() << " reference cards" << std::endl;
}

Card CardRecognizer::parseCardInfoFromFilename(const std::string& filename) {
    Card card;
    card.type=CardType::COINS;
    card.value=1;

    std::string lowerName = filename;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    if (lowerName.find("coins") != std::string::npos) {
        card.type = CardType::COINS;
    } else if (lowerName.find("clubs") != std::string::npos) {
        card.type = CardType::CLUBS;
    } else if (lowerName.find("cups") != std::string::npos) {
        card.type = CardType::CUPS;
    } else if (lowerName.find("spades") != std::string::npos) {
        card.type = CardType::SPADES;
    }
    
    for (int v = 10; v >= 1; --v) {
        if (lowerName.find(std::to_string(v)) != std::string::npos) {
            card.value = v;
            break;
        }
    }

    return card;
}


Card CardRecognizer::identifyCard(const cv::Mat& croppedCard) {
    Card bestMatchCard = { CardType::COINS, 0 };
    if (croppedCard.empty() || referenceDeck.empty()) {
        return bestMatchCard;
    }

    cv::Mat grayCrop;
    if (croppedCard.channels() == 3) {
        cv::cvtColor(croppedCard, grayCrop, cv::COLOR_BGR2GRAY);
    } else {
        grayCrop = croppedCard;
    }

    std::vector<cv::KeyPoint> cropKeypoints;
    cv::Mat cropDescriptors;
    siftDetector->detectAndCompute(grayCrop, cv::noArray(), cropKeypoints, cropDescriptors);
    if (cropDescriptors.empty()||cropKeypoints.size() < 4) {
        return bestMatchCard;
    }

    cv::BFMatcher matcher(cv::NORM_L2);
    int maxInliers = -1;
    //iterate through each reference card and perform matching
    for (const auto& refCard:referenceDeck) {
        if (refCard.descriptors.empty()) continue;
        std::vector<std::vector<cv::DMatch>> knnMatches;
        matcher.knnMatch(cropDescriptors, refCard.descriptors, knnMatches, 2);

        std::vector<cv::Point2f> srcPoints;
        std::vector<cv::Point2f> dstPoints;
        for (const auto& matchPair:knnMatches) {
            if (matchPair.size()>=2) {
                if (matchPair[0].distance<0.75f * matchPair[1].distance) {
                    srcPoints.push_back(cropKeypoints[matchPair[0].queryIdx].pt);
                    dstPoints.push_back(refCard.keypoints[matchPair[0].trainIdx].pt);
                }
            }
        }

        if (srcPoints.size()>=4) {
            cv::Mat inlierMask;
            cv::Mat H=cv::findHomography(srcPoints, dstPoints, cv::RANSAC, 5.0, inlierMask);
            if (!H.empty()) {
                int inlierCount=cv::countNonZero(inlierMask);
                if (inlierCount>maxInliers) {
                    maxInliers=inlierCount;
                    bestMatchCard=refCard.cardInfo;
                }
            }
        }
    }
    return bestMatchCard;
}