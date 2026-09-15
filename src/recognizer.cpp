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


    // ---------------------------------------------------------
    // 1. Convert crop to grayscale
    // ---------------------------------------------------------

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


    // ---------------------------------------------------------
    // 2. Extract SIFT features from the detected card
    // ---------------------------------------------------------

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


    // ---------------------------------------------------------
    // Preliminary result before geometric verification.
    //
    // BF + Lowe ratio are performed on all 40 reference cards.
    // RANSAC will be performed only on the strongest candidates.
    // ---------------------------------------------------------

    struct PreliminaryCandidate {

        const refCard* reference = nullptr;

        std::vector<cv::Point2f> srcPoints;
        std::vector<cv::Point2f> dstPoints;

        double ratioQuality = 0.0;
    };


    std::vector<PreliminaryCandidate> preliminary;

    preliminary.reserve(referenceDeck.size());


    cv::BFMatcher matcher(cv::NORM_L2);


    // ---------------------------------------------------------
    // 3. BF matching against ALL 40 reference cards
    // ---------------------------------------------------------

    for (const auto& referenceCard : referenceDeck) {

        if (referenceCard.descriptors.empty()) {
            continue;
        }


        std::vector<std::vector<cv::DMatch>> knnMatches;

        matcher.knnMatch(
            cropDescriptors,
            referenceCard.descriptors,
            knnMatches,
            2
        );


        std::vector<cv::Point2f> srcPoints;
        std::vector<cv::Point2f> dstPoints;

        double ratioQuality = 0.0;


        // Lowe ratio test
        for (const auto& matchPair : knnMatches) {

            if (matchPair.size() < 2) {
                continue;
            }


            const cv::DMatch& best =
                matchPair[0];

            const cv::DMatch& second =
                matchPair[1];


            if (best.distance <
                0.75f * second.distance) {


                srcPoints.push_back(
                    cropKeypoints[
                        best.queryIdx
                    ].pt
                );


                dstPoints.push_back(
                    referenceCard.keypoints[
                        best.trainIdx
                    ].pt
                );


                // Extra quality measure used only as
                // tie-breaker between preliminary candidates.
                if (second.distance > 0.0f) {

                    ratioQuality +=
                        1.0 -
                        static_cast<double>(
                            best.distance /
                            second.distance
                        );
                }
            }
        }


        // Homography requires at least 4 point pairs.
        if (srcPoints.size() >= 4) {

            PreliminaryCandidate candidate;

            candidate.reference =
                &referenceCard;

            candidate.srcPoints =
                std::move(srcPoints);

            candidate.dstPoints =
                std::move(dstPoints);

            candidate.ratioQuality =
                ratioQuality;


            preliminary.push_back(
                std::move(candidate)
            );
        }
    }


    if (preliminary.empty()) {
        return {};
    }


    // ---------------------------------------------------------
    // 4. Rank candidates BEFORE expensive RANSAC
    //
    // Primary criterion:
    //     number of matches that passed Lowe ratio.
    //
    // Tie-breaker:
    //     overall Lowe-ratio quality.
    // ---------------------------------------------------------

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


    // ---------------------------------------------------------
    // 5. Run expensive RANSAC only on the strongest candidates
    //
    // All 40 cards have STILL been compared through BF/Lowe.
    // This only limits geometric verification.
    //
    // 8 is intentionally conservative.
    // If accuracy remains unchanged it could later be tested
    // with 5; if recall decreases it can be increased.
    // ---------------------------------------------------------

    constexpr std::size_t RANSAC_TOP_K = 8;


    const std::size_t ransacCount =
        std::min(
            RANSAC_TOP_K,
            preliminary.size()
        );


    for (std::size_t i = 0;
         i < ransacCount;
         ++i) {


        const auto& preliminaryCandidate =
            preliminary[i];


        cv::Mat inlierMask;


        cv::Mat homography =
            cv::findHomography(
                preliminaryCandidate.srcPoints,
                preliminaryCandidate.dstPoints,
                cv::RANSAC,
                5.0,
                inlierMask
            );


        if (homography.empty()) {
            continue;
        }


        const int inlierCount =
            cv::countNonZero(
                inlierMask
            );


        // Same acceptance logic as before:
        // require more than 4 RANSAC inliers.
        if (inlierCount <= 4) {
            continue;
        }


        CardDetected candidate;

        candidate.card =
            preliminaryCandidate
                .reference
                ->cardInfo;

        candidate.confidence =
            static_cast<double>(
                inlierCount
            );


        candidates.push_back(
            candidate
        );
    }


    // ---------------------------------------------------------
    // 6. Final ranking based on RANSAC inliers
    // ---------------------------------------------------------

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
}*/
