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


std::vector<CardDetected> CardRecognizer::identifyCard(
    const cv::Mat& croppedCard
) {
    std::vector<CardDetected> candidates;

    if (croppedCard.empty() || referenceDeck.empty()) {
        return {};
    }

    cv::Mat grayCrop;

    if (croppedCard.channels() == 3) {
        cv::cvtColor(
            croppedCard,
            grayCrop,
            cv::COLOR_BGR2GRAY
        );
    }
    else {
        grayCrop = croppedCard;
    }

    std::vector<cv::KeyPoint> cropKeypoints;
    cv::Mat cropDescriptors;

    siftDetector->detectAndCompute(
        grayCrop,
        cv::noArray(),
        cropKeypoints,
        cropDescriptors
    );

    if (cropDescriptors.empty() ||
        cropKeypoints.size() < 4) {

        return {};
    }


    struct PreliminaryCandidate {
        const refCard* reference;

        std::vector<cv::Point2f> srcPoints;
        std::vector<cv::Point2f> dstPoints;

        double ratioQuality = 0.0;
    };


    std::vector<PreliminaryCandidate> preliminary;

    preliminary.reserve(referenceDeck.size());

    cv::BFMatcher matcher(cv::NORM_L2);


    /*
     * STEP 1
     *
     * Perform the cheaper BF matching against all reference cards.
     * Do NOT run RANSAC yet.
     */
    for (const auto& refCard : referenceDeck) {

        if (refCard.descriptors.empty()) {
            continue;
        }

        std::vector<std::vector<cv::DMatch>> knnMatches;

        matcher.knnMatch(
            cropDescriptors,
            refCard.descriptors,
            knnMatches,
            2
        );


        std::vector<cv::Point2f> srcPoints;
        std::vector<cv::Point2f> dstPoints;

        double ratioQuality = 0.0;


        for (const auto& matchPair : knnMatches) {

            if (matchPair.size() < 2) {
                continue;
            }

            const cv::DMatch& best = matchPair[0];
            const cv::DMatch& second = matchPair[1];

            if (best.distance < 0.75f * second.distance) {

                srcPoints.push_back(
                    cropKeypoints[best.queryIdx].pt
                );

                dstPoints.push_back(
                    refCard.keypoints[best.trainIdx].pt
                );

                if (second.distance > 0.0f) {
                    ratioQuality +=
                        1.0 -
                        static_cast<double>(
                            best.distance / second.distance
                        );
                }
            }
        }


        if (srcPoints.size() >= 4) {

            PreliminaryCandidate candidate;

            candidate.reference = &refCard;
            candidate.srcPoints = std::move(srcPoints);
            candidate.dstPoints = std::move(dstPoints);
            candidate.ratioQuality = ratioQuality;

            preliminary.push_back(
                std::move(candidate)
            );
        }
    }


    if (preliminary.empty()) {
        return {};
    }


    /*
     * STEP 2
     *
     * Rank references before expensive RANSAC.
     *
     * Number of Lowe-valid matches is the primary criterion.
     * Ratio quality is used as tie breaker.
     */
    std::sort(
        preliminary.begin(),
        preliminary.end(),

        [](const PreliminaryCandidate& a,
           const PreliminaryCandidate& b) {

            if (a.srcPoints.size() !=
                b.srcPoints.size()) {

                return a.srcPoints.size() >
                       b.srcPoints.size();
            }

            return a.ratioQuality >
                   b.ratioQuality;
        }
    );


    /*
     * Start conservatively.
     *
     * If accuracy remains unchanged, try 5 later.
     */
    constexpr std::size_t RANSAC_TOP_K = 8;

    const std::size_t ransacCount =
        std::min(
            RANSAC_TOP_K,
            preliminary.size()
        );


    /*
     * STEP 3
     *
     * Expensive geometric verification only
     * for the strongest preliminary candidates.
     */
    for (std::size_t i = 0;
         i < ransacCount;
         ++i) {

        const auto& preliminaryCandidate =
            preliminary[i];

        cv::Mat inlierMask;

        cv::Mat H = cv::findHomography(
            preliminaryCandidate.srcPoints,
            preliminaryCandidate.dstPoints,
            cv::RANSAC,
            5.0,
            inlierMask
        );


        if (H.empty()) {
            continue;
        }


        int inlierCount =
            cv::countNonZero(inlierMask);


        if (inlierCount <= 4) {
            continue;
        }


        CardDetected candidate;

        candidate.card =
            preliminaryCandidate.reference->cardInfo;

        candidate.confidence =
            static_cast<double>(inlierCount);

        candidates.push_back(candidate);
    }


    std::sort(
        candidates.begin(),
        candidates.end(),

        [](const CardDetected& a,
           const CardDetected& b) {

            return a.confidence >
                   b.confidence;
        }
    );


    return candidates;
}


//DEPRECATED: This function is kept for reference only. 
/* 
std::vector<CardDetected> CardRecognizer::identifyCard(const cv::Mat& croppedCard) {
    std::vector<CardDetected> candidates;
    if (croppedCard.empty() || referenceDeck.empty()) {
        return {};
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
        return {};
    }

    cv::BFMatcher matcher(cv::NORM_L2);
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
                // Keep only homographies supported by more than four RANSAC inliers.
                // Lower values produced unreliable matches during video tests.
                if (inlierCount > 4) {
                    CardDetected candidate;
                    candidate.card = refCard.cardInfo;
                    // Keep the absolute SIFT score. Normalization is performed
                    // only after observations from different frames are combined.
                    candidate.confidence = static_cast<double>(inlierCount);
                    candidates.push_back(candidate);
                }
            }
        }
    }

    if (candidates.empty())
        return {};

    // Keep the strongest alternatives produced for this single crop.
    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const CardDetected& a, const CardDetected& b) {
            return a.confidence > b.confidence;
        }
    );

    return candidates;
}
*/