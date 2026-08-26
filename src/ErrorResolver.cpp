#include "ErrorResolver.h"
#include "GameEngine.h"

#include <limits>


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
                // Check the alternatives starting from candidate 1.
                for (size_t i = 1; i < candidates->size(); i++) {

                    const CardDetected& candidate = (*candidates)[i];

                    // The alternative is useful only if the card is currently missing
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


        // Apply the best duplicate correction and validate again
        if (correctionFound) {

            if (bestPlayer == Player::NORTH) {
                game.rounds[bestRoundIndex].north = bestCard;
            }
            else {
                game.rounds[bestRoundIndex].south = bestCard;
            }

            corrections++;

            continue;
        }


        /*
         * No duplicate correction was found.
         * Check whether there is exactly one UNKNOWN position and
         * exactly one missing card left.
         */

        int unknownCount = 0;
        int unknownRoundIndex = -1;
        Player unknownPlayer = Player::NORTH;


        for (size_t i = 0; i < game.rounds.size(); i++) {

            if (game.rounds[i].north.value == 0) {
                unknownCount++;
                unknownRoundIndex = static_cast<int>(i);
                unknownPlayer = Player::NORTH;
            }

            if (game.rounds[i].south.value == 0) {
                unknownCount++;
                unknownRoundIndex = static_cast<int>(i);
                unknownPlayer = Player::SOUTH;
            }
        }


        int missingCount = 0;
        Card missingCard;


        for (const auto& issue : validation.cardIssues) {

            if (issue.type == CardIssueType::MISSING_CARD) {
                missingCount++;
                missingCard = issue.card;
            }
        }


        // One UNKNOWN + one missing card gives a forced correction
        if (unknownCount == 1 && missingCount == 1) {

            if (unknownPlayer == Player::NORTH) {
                game.rounds[unknownRoundIndex].north = missingCard;
            }
            else {
                game.rounds[unknownRoundIndex].south = missingCard;
            }

            corrections++;

            continue;
        }


        // Nothing else can be safely corrected in this block
        break;
    }


    // Compute winners and scores only if all card detections are complete
    bool cardsComplete = true;

    for (const auto& round : game.rounds) {

        if (round.north.value == 0 || round.south.value == 0) {
            cardsComplete = false;
            break;
        }
    }


    if (cardsComplete) {
        GameEngine::computeGame(game);
    }


    return corrections;
}



int ErrorResolver::resolveBriscola(Game& game) {

    // Briscola correction requires all played cards to be known
    for (const auto& round : game.rounds) {

        if (round.north.value == 0 || round.south.value == 0) {
            return 0;
        }
    }


    /*
     * NORMAL CASE:
     * Try all briscola candidates detected
     */
    if (!game.prediction.briscolaDetected.empty()) {

        Card originalBriscola = game.briscola;
        Card bestBriscola = game.briscola;

        int bestLeaderIssues = std::numeric_limits<int>::max();
        double bestConfidence = -1.0;


        for (const auto& candidate : game.prediction.briscolaDetected) {

            game.briscola = candidate.card;

            GameEngine::computeGame(game);

            ValidationResult validation = Validator::validate(game);

            int leaderIssues = static_cast<int>(validation.leaderIssues.size());


            if (leaderIssues < bestLeaderIssues ||
                (leaderIssues == bestLeaderIssues && candidate.confidence > bestConfidence)) {

                bestLeaderIssues = leaderIssues;
                bestConfidence = candidate.confidence;
                bestBriscola = candidate.card;
            }
        }


        game.briscola = bestBriscola;

        GameEngine::computeGame(game);


        if (!sameCard(originalBriscola, bestBriscola)) {
            return 1;
        }

        return 0;
    }


    /*
     * UNKNOWN BRISCOLA:
     * The value is unknown, but the suit can be inferred by trying
     * all four suits and checking winner-leader consistency.
     */

    Game originalGame = game;

    int bestLeaderIssues = std::numeric_limits<int>::max();
    CardType bestType = static_cast<CardType>(0);

    bool solutionFound = false;
    bool ambiguous = false;


    for (int type = 0; type < 4; type++) {

        Card testBriscola{};
        testBriscola.type = static_cast<CardType>(type);

        // Temporary valid value. Only the suit affects round winners.
        testBriscola.value = 1;

        game.briscola = testBriscola;

        GameEngine::computeGame(game);

        ValidationResult validation = Validator::validate(game);

        int leaderIssues = static_cast<int>(validation.leaderIssues.size());


        if (!solutionFound || leaderIssues < bestLeaderIssues) {

            bestLeaderIssues = leaderIssues;
            bestType = testBriscola.type;

            solutionFound = true;
            ambiguous = false;
        }
        else if (leaderIssues == bestLeaderIssues) {

            ambiguous = true;
        }
    }


    // More than one suit gives the same result: do not choose arbitrarily
    if (!solutionFound || ambiguous) {
        game = originalGame;
        return 0;
    }


    // Suit resolved, value still unknown
    game = originalGame;

    game.briscola.type = bestType;
    game.briscola.value = 0;


    /*
     * computeGame can still be used here because BriscolaRules only needs
     * the briscola suit to determine round winners.
     */
    GameEngine::computeGame(game);

    return 1;
}