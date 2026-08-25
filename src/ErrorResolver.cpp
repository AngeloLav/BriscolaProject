#include "ErrorResolver.h"
#include "GameEngine.h"


static bool sameCard(const Card& first, const Card& second) {

    return first.type == second.type && first.value == second.value;
}


static bool isMissingCard(const Card& card, const ValidationResult& validation) {
    for (const auto& issue : validation.cardIssues) {

        if (issue.type == CardIssueType::MISSING_CARD && sameCard(issue.card, card)) {
            return true;
        }
    }

    return false;
}



int ErrorResolver::resolveCardIssues(Game& game) {

    int corrections = 0;

    while (true) {
        ValidationResult validation = Validator::validate(game);

        bool correctionFound = false;
        double bestConfidenceLoss = std::numeric_limits<double>::max();
        int bestRoundIndex = -1;
        Player bestPlayer = Player::NORTH;
        Card bestCard;


        // Look at every duplicated card
        for (const auto& issue : validation.cardIssues) {

            if (issue.type != CardIssueType::DUPLICATE_CARD) {
                continue;
            }

            // Check every position where the duplicated card appears
            for (const auto& position : issue.positions) {

                int roundIndex = position.round - 1;
                const RoundPrediction& prediction = game.prediction.rounds[roundIndex];
                const std::vector<CardDetected>* candidates;

                if (position.player == Player::NORTH) {
                    candidates = &prediction.northDetected;
                }
                else {
                    candidates = &prediction.southDetected;
                }

                // Candidate 0 is the original top prediction.
                // We check the alternatives starting from candidate 1.
                for (size_t i = 1; i < candidates->size(); i++) {

                    const CardDetected& candidate = (*candidates)[i];

                    // An alternative is useful only if it corresponds to a card currently missing from the game
                    if (!isMissingCard(candidate.card, validation)) {
                        continue;
                    }

                    double confidenceLoss = (*candidates)[0].confidence - candidate.confidence;

                    if (confidenceLoss < bestConfidenceLoss) {
                        bestConfidenceLoss = confidenceLoss;
                        bestRoundIndex = roundIndex;
                        bestPlayer = position.player;
                        bestCard = candidate.card;
                        correctionFound = true;
                    }
                }
            }
        }

        // No valid correction was found
        if (!correctionFound) {
            break;
        }

        // Apply the best correction
        if (bestPlayer == Player::NORTH) {
            game.rounds[bestRoundIndex].north = bestCard;
        }
        else {
            game.rounds[bestRoundIndex].south = bestCard;
        }

        corrections++;
    
    }

    // Card corrections may change winners and scores.
    GameEngine::computeGame(game);

    return corrections;

}