#include "ErrorResolver.h"
#include "GameEngine.h"

#include <cmath>
#include <functional>
#include <limits>
#include <vector>


// Returns true if two cards have the same type and value
static bool sameCard(const Card& first, const Card& second) {
    return first.type == second.type && first.value == second.value;
}


/*
 * Correct card recognition errors using the constraints of the Briscola deck.
 *
 * The resolver only works on suspicious positions:
 * - cards that appear more than once;
 * - cards for which the detection failed (UNKNOWN).
 *
 * Missing cards are used as possible replacements. This also handles the case
 * where the first candidate is wrong and no alternative candidate was detected:
 * the consistency checker can still try one of the missing cards.
 *
 * The best solution is the one with the fewest card issues.
 * If more solutions have the same number of issues, recognition confidence is
 * used to choose between them. If they are still equivalent, no correction
 * is applied because the result would be ambiguous.
 *
 * Winner/leader consistency is not considered here. Using missing cards should be enough
 */
int ErrorResolver::resolveCardIssues(Game& game) {

    ValidationResult validation = Validator::validate(game);

    int originalIssueCount = static_cast<int>(validation.cardIssues.size());

    std::vector<Card> missingCards;
    std::vector<CardPosition> suspectPositions;


    // Collect missing cards and positions containing duplicated cards
    for (const auto& issue : validation.cardIssues) {

        if (issue.type == CardIssueType::MISSING_CARD) {
            missingCards.push_back(issue.card);
        }

        else if (issue.type == CardIssueType::DUPLICATE_CARD) {

            for (const auto& position : issue.positions) {
                suspectPositions.push_back(position);
            }
        }
    }


    // A position with no detected card is also suspicious
    for (const auto& round : game.rounds) {

        if (round.north.value == 0) {
            suspectPositions.push_back({
                round.round,
                Player::NORTH
            });
        }

        if (round.south.value == 0) {
            suspectPositions.push_back({
                round.round,
                Player::SOUTH
            });
        }
    }


    if (suspectPositions.empty() || missingCards.empty()) {
        return 0;
    }


    Game testGame = game;
    Game bestGame = game;

    int bestIssueCount = std::numeric_limits<int>::max();
    double bestConfidence = -1.0;

    bool solutionFound = false;
    bool ambiguous = false;

    std::vector<bool> usedMissing(missingCards.size(), false);


    // Return the confidence assigned to a card.
    // Cards generated only by the consistency checker have confidence 0.
    auto getConfidence = [&](int roundIndex, Player player, const Card& card) {

            const RoundPrediction& prediction = game.prediction.rounds[roundIndex];
            const std::vector<CardDetected>& candidates = (player == Player::NORTH) ? prediction.northDetected : prediction.southDetected;

            for (const auto& candidate : candidates) {

                if (sameCard(candidate.card, card)) {
                    return candidate.confidence;
                }
            }

            return 0.0;
        };


    std::function<void(size_t, double)> tryAssignments;

    tryAssignments = [&](size_t positionIndex, double confidenceSum) {

            // All suspicious positions have been tested
            if (positionIndex == suspectPositions.size()) {

                ValidationResult result = Validator::validate(testGame);

                int issueCount = static_cast<int>(result.cardIssues.size());


                if (!solutionFound || issueCount < bestIssueCount || (issueCount == bestIssueCount && confidenceSum > bestConfidence + 1e-9)) {
                    bestIssueCount = issueCount;
                    bestConfidence = confidenceSum;
                    bestGame = testGame;

                    solutionFound = true;
                    ambiguous = false;
                }

                else if (issueCount == bestIssueCount && std::abs(confidenceSum - bestConfidence) <= 1e-9) {
                    ambiguous = true;
                }

                return;
            }


            const CardPosition& position = suspectPositions[positionIndex];

            int roundIndex = position.round - 1;

            Card& card = (position.player == Player::NORTH) ? testGame.rounds[roundIndex].north : testGame.rounds[roundIndex].south;
            Card originalCard = card;


            // If a card was detected, also try keeping the original prediction
            if (originalCard.value != 0) {
                double confidence =
                    getConfidence(
                        roundIndex,
                        position.player,
                        originalCard
                    );

                tryAssignments(positionIndex + 1, confidenceSum + confidence);
            }


            // Try each currently missing card in this position
            for (size_t i = 0; i < missingCards.size(); i++) {

                if (usedMissing[i]) {
                    continue;
                }

                card = missingCards[i];
                usedMissing[i] = true;

                double confidence = getConfidence(roundIndex, position.player, missingCards[i]);

                tryAssignments(positionIndex + 1, confidenceSum + confidence);

                usedMissing[i] = false;
                card = originalCard;
            }


            card = originalCard;
        };


    tryAssignments(0, 0.0);


    // Do not change the game if the best solution is not an improvement
    // or if two equally good solutions cannot be distinguished.
    if (!solutionFound || bestIssueCount >= originalIssueCount || ambiguous) {
        return 0;
    }


    int corrections = 0;

    for (size_t i = 0; i < game.rounds.size(); i++) {

        if (!sameCard(game.rounds[i].north, bestGame.rounds[i].north)) {
            corrections++;
        }

        if (!sameCard(game.rounds[i].south, bestGame.rounds[i].south)) {
            corrections++;
        }
    }


    game = bestGame;


    // Recompute the game only when all cards are known.
    bool cardsComplete = true;

    for (const auto& round : game.rounds) {

        if (round.north.value == 0 ||
            round.south.value == 0) {

            cardsComplete = false;
            break;
        }
    }

    if (cardsComplete && game.briscola.value != 0) {
        GameEngine::computeGame(game);
    }


    return corrections;
}