#include "ErrorResolver.h"
#include "GameEngine.h"

#include <limits>
#include <cmath>


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



static double getCardConfidence(const Game& game, int roundIndex, Player player, const Card& card) {

    const RoundPrediction& prediction = game.prediction.rounds[roundIndex];

    const std::vector<CardDetected>& candidates =
        (player == Player::NORTH) ? prediction.northDetected : prediction.southDetected;

    for (const auto& candidate : candidates) {

        if (sameCard(candidate.card, card)) {
            return candidate.confidence;
        }
    }

    return 0.0;
}


static double getLeaderConfidence(const Game& game, int roundIndex, Player player) {

    const RoundPrediction& prediction = game.prediction.rounds[roundIndex];

    for (const auto& candidate : prediction.leaderDetected) {

        if (candidate.player == player) {
            return candidate.confidence;
        }
    }

    return 0.0;
}


static bool findCardPosition(const Game& game, const Card& card, int& roundIndex, Player& player) {

    for (size_t i = 0; i < game.rounds.size(); i++) {

        if (sameCard(game.rounds[i].north, card)) {
            roundIndex = static_cast<int>(i);
            player = Player::NORTH;
            return true;
        }

        if (sameCard(game.rounds[i].south, card)) {
            roundIndex = static_cast<int>(i);
            player = Player::SOUTH;
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
        * If there are still missing cards, check if they correspond exactly to
        * the UNKNOWN positions.
        */

        std::vector<CardPosition> unknownPositions;
        std::vector<Card> missingCards;


        // Find UNKNOWN positions
        for (size_t i = 0; i < game.rounds.size(); i++) {

            if (game.rounds[i].north.value == 0) {
                unknownPositions.push_back({
                    static_cast<int>(i + 1),
                    Player::NORTH
                });
            }

            if (game.rounds[i].south.value == 0) {
                unknownPositions.push_back({
                    static_cast<int>(i + 1),
                    Player::SOUTH
                });
            }
        }


        // Find missing cards
        for (const auto& issue : validation.cardIssues) {

            if (issue.type == CardIssueType::MISSING_CARD) {
                missingCards.push_back(issue.card);
            }
        }


        /*
        * If the remaining missing cards correspond exactly to the UNKNOWN
        * positions, assign them in order.
        *
        * With one UNKNOWN the solution is forced.
        * With multiple UNKNOWNs the assignment is only a temporary consistent
        * solution that can later be improved using winner-leader constraints.
        */
        if (!unknownPositions.empty() &&
            unknownPositions.size() == missingCards.size()) {

            for (size_t i = 0; i < unknownPositions.size(); i++) {

                int roundIndex = unknownPositions[i].round - 1;

                if (unknownPositions[i].player == Player::NORTH) {
                    game.rounds[roundIndex].north = missingCards[i];
                }
                else {
                    game.rounds[roundIndex].south = missingCards[i];
                }

                corrections++;
            }

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
     * Try all briscola candidates detected.
     */
    if (!game.prediction.briscolaDetected.empty()) {

        Card originalBriscola = game.briscola;
        Card bestBriscola = game.briscola;

        int bestLeaderIssues = std::numeric_limits<int>::max();
        double bestConfidence = -1.0;
        bool bestPositionValid = false;


        for (const auto& candidate : game.prediction.briscolaDetected) {

            game.briscola = candidate.card;

            GameEngine::computeGame(game);

            ValidationResult validation = Validator::validate(game);

            int leaderIssues = static_cast<int>(validation.leaderIssues.size());


            // The loser of round 17 receives the briscola
            Player briscolaPlayer;

            if (game.rounds[16].winner == Player::NORTH) {
                briscolaPlayer = Player::SOUTH;
            }
            else {
                briscolaPlayer = Player::NORTH;
            }


            bool positionValid = false;


            // Check if the exact briscola appears among that player's last three cards
            for (size_t i = 17; i < game.rounds.size(); i++) {

                Card card;

                if (briscolaPlayer == Player::NORTH) {
                    card = game.rounds[i].north;
                }
                else {
                    card = game.rounds[i].south;
                }


                if (sameCard(card, candidate.card)) {
                    positionValid = true;
                    break;
                }
            }


            /*
             * Selection priority:
             * 1. fewer leader issues
             * 2. valid briscola position
             * 3. higher recognition confidence
             */
            if (leaderIssues < bestLeaderIssues ||
                (leaderIssues == bestLeaderIssues && positionValid && !bestPositionValid) ||
                (leaderIssues == bestLeaderIssues && positionValid == bestPositionValid &&
                 candidate.confidence > bestConfidence)) {

                bestLeaderIssues = leaderIssues;
                bestConfidence = candidate.confidence;
                bestPositionValid = positionValid;
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
     * Try all four suits and select the one producing the fewest leader issues.
     * The presence of that suit among the last three cards of the player who
     * receives the briscola is used as a tie-breaker.
     */

    Game originalGame = game;

    int bestLeaderIssues = std::numeric_limits<int>::max();
    CardType bestType = static_cast<CardType>(0);

    bool bestPositionValid = false;
    bool solutionFound = false;
    bool ambiguous = false;


    for (int type = 0; type < 4; type++) {

        Card testBriscola{};
        testBriscola.type = static_cast<CardType>(type);
        testBriscola.value = 1;

        game.briscola = testBriscola;

        GameEngine::computeGame(game);

        ValidationResult validation = Validator::validate(game);

        int leaderIssues = static_cast<int>(validation.leaderIssues.size());


        // The loser of round 17 receives the briscola
        Player briscolaPlayer;

        if (game.rounds[16].winner == Player::NORTH) {
            briscolaPlayer = Player::SOUTH;
        }
        else {
            briscolaPlayer = Player::NORTH;
        }


        bool positionValid = false;


        // Check if a card of this suit appears among that player's last three cards
        for (size_t i = 17; i < game.rounds.size(); i++) {

            Card card;

            if (briscolaPlayer == Player::NORTH) {
                card = game.rounds[i].north;
            }
            else {
                card = game.rounds[i].south;
            }


            if (card.type == testBriscola.type) {
                positionValid = true;
                break;
            }
        }


        if (!solutionFound ||
            leaderIssues < bestLeaderIssues ||
            (leaderIssues == bestLeaderIssues && positionValid && !bestPositionValid)) {

            bestLeaderIssues = leaderIssues;
            bestType = testBriscola.type;
            bestPositionValid = positionValid;

            solutionFound = true;
            ambiguous = false;
        }
        else if (leaderIssues == bestLeaderIssues &&
                 positionValid == bestPositionValid) {

            ambiguous = true;
        }
    }


    // More than one equally good suit: do not choose arbitrarily
    if (!solutionFound || ambiguous) {
        game = originalGame;
        return 0;
    }


    // Restore the game and use the resolved suit
    game = originalGame;

    game.briscola.type = bestType;
    game.briscola.value = 1;

    GameEngine::computeGame(game);


    // The loser of round 17 receives the briscola
    Player briscolaPlayer;

    if (game.rounds[16].winner == Player::NORTH) {
        briscolaPlayer = Player::SOUTH;
    }
    else {
        briscolaPlayer = Player::NORTH;
    }


    int possibleBriscole = 0;
    Card possibleBriscola{};


    // Look for possible briscole among that player's last three cards
    for (size_t i = 17; i < game.rounds.size(); i++) {

        Card card;

        if (briscolaPlayer == Player::NORTH) {
            card = game.rounds[i].north;
        }
        else {
            card = game.rounds[i].south;
        }


        if (card.type == bestType) {
            possibleBriscole++;
            possibleBriscola = card;
        }
    }


    // Only one possible card: suit and value are both resolved
    if (possibleBriscole == 1) {
        game.briscola = possibleBriscola;
    }
    else {
        // Suit is known, but the value cannot be determined yet
        game.briscola.type = bestType;
        game.briscola.value = 0;
    }


    GameEngine::computeGame(game);

    return 1;
}


int ErrorResolver::resolveLeaderIssues(Game& game) {

    int corrections = 0;


    while (true) {

        ValidationResult validation = Validator::validate(game);

        // Card consistency must already be solved before this block
        if (!validation.cardIssues.empty()) {
            break;
        }

        int currentLeaderIssues = static_cast<int>(validation.leaderIssues.size());

        if (currentLeaderIssues == 0) {
            break;
        }


        Game bestGame = game;

        int bestLeaderIssues = currentLeaderIssues;
        double bestConfidenceLoss = std::numeric_limits<double>::max();
        int bestCorrectionCount = 0;

        bool correctionFound = false;
        bool ambiguous = false;


        // Check every winner-leader inconsistency
        for (const auto& issue : validation.leaderIssues) {

            int previousRoundIndex = issue.previousRound - 1;
            int nextRoundIndex = issue.nextRound - 1;


            /*
             * CASE 1:
             * The leader of the next round may be wrong.
             */
            const RoundPrediction& nextPrediction = game.prediction.rounds[nextRoundIndex];

            Player currentLeader = game.rounds[nextRoundIndex].leader;
            double currentLeaderConfidence = getLeaderConfidence(game, nextRoundIndex, currentLeader);


            for (const auto& candidate : nextPrediction.leaderDetected) {

                if (candidate.player == currentLeader) {
                    continue;
                }


                Game testGame = game;

                testGame.rounds[nextRoundIndex].leader = candidate.player;

                GameEngine::computeGame(testGame);

                ValidationResult testValidation = Validator::validate(testGame);

                int leaderIssues = static_cast<int>(testValidation.leaderIssues.size());
                double confidenceLoss = currentLeaderConfidence - candidate.confidence;


                if (leaderIssues < bestLeaderIssues ||
                    (leaderIssues == bestLeaderIssues &&
                     leaderIssues < currentLeaderIssues &&
                     confidenceLoss < bestConfidenceLoss - 1e-9)) {

                    bestLeaderIssues = leaderIssues;
                    bestConfidenceLoss = confidenceLoss;
                    bestGame = testGame;
                    bestCorrectionCount = 1;

                    correctionFound = true;
                    ambiguous = false;
                }
                else if (leaderIssues == bestLeaderIssues &&
                         leaderIssues < currentLeaderIssues &&
                         std::abs(confidenceLoss - bestConfidenceLoss) <= 1e-9) {

                    ambiguous = true;
                }
            }


            /*
             * CASE 2:
             * The winner of the previous round may be wrong because
             * one of its two cards was misdetected.
             *
             * Since all 40 cards are already present, replacing only one
             * card would create a duplicate and a missing card. Therefore
             * the candidate card is swapped with its current position.
             */

            const RoundPrediction& previousPrediction = game.prediction.rounds[previousRoundIndex];


            // Try NORTH card alternatives
            for (const auto& candidate : previousPrediction.northDetected) {

                Card currentCard = game.rounds[previousRoundIndex].north;

                if (sameCard(candidate.card, currentCard)) {
                    continue;
                }


                int otherRoundIndex;
                Player otherPlayer;

                if (!findCardPosition(game, candidate.card, otherRoundIndex, otherPlayer)) {
                    continue;
                }


                // Do not swap with the same position
                if (otherRoundIndex == previousRoundIndex && otherPlayer == Player::NORTH) {
                    continue;
                }


                Game testGame = game;

                Card otherCard;

                if (otherPlayer == Player::NORTH) {
                    otherCard = testGame.rounds[otherRoundIndex].north;
                    testGame.rounds[otherRoundIndex].north = currentCard;
                }
                else {
                    otherCard = testGame.rounds[otherRoundIndex].south;
                    testGame.rounds[otherRoundIndex].south = currentCard;
                }

                testGame.rounds[previousRoundIndex].north = otherCard;


                GameEngine::computeGame(testGame);

                ValidationResult testValidation = Validator::validate(testGame);


                // The swap must preserve the deck consistency
                if (!testValidation.cardIssues.empty()) {
                    continue;
                }


                int leaderIssues = static_cast<int>(testValidation.leaderIssues.size());


                double beforeConfidence =
                    getCardConfidence(game, previousRoundIndex, Player::NORTH, currentCard) +
                    getCardConfidence(game, otherRoundIndex, otherPlayer, otherCard);

                double afterConfidence =
                    getCardConfidence(game, previousRoundIndex, Player::NORTH, otherCard) +
                    getCardConfidence(game, otherRoundIndex, otherPlayer, currentCard);

                double confidenceLoss = beforeConfidence - afterConfidence;


                if (leaderIssues < bestLeaderIssues ||
                    (leaderIssues == bestLeaderIssues &&
                     leaderIssues < currentLeaderIssues &&
                     confidenceLoss < bestConfidenceLoss - 1e-9)) {

                    bestLeaderIssues = leaderIssues;
                    bestConfidenceLoss = confidenceLoss;
                    bestGame = testGame;
                    bestCorrectionCount = 2;

                    correctionFound = true;
                    ambiguous = false;
                }
                else if (leaderIssues == bestLeaderIssues &&
                         leaderIssues < currentLeaderIssues &&
                         std::abs(confidenceLoss - bestConfidenceLoss) <= 1e-9) {

                    ambiguous = true;
                }
            }


            // Try SOUTH card alternatives
            for (const auto& candidate : previousPrediction.southDetected) {

                Card currentCard = game.rounds[previousRoundIndex].south;

                if (sameCard(candidate.card, currentCard)) {
                    continue;
                }


                int otherRoundIndex;
                Player otherPlayer;

                if (!findCardPosition(game, candidate.card, otherRoundIndex, otherPlayer)) {
                    continue;
                }


                if (otherRoundIndex == previousRoundIndex && otherPlayer == Player::SOUTH) {
                    continue;
                }


                Game testGame = game;

                Card otherCard;

                if (otherPlayer == Player::NORTH) {
                    otherCard = testGame.rounds[otherRoundIndex].north;
                    testGame.rounds[otherRoundIndex].north = currentCard;
                }
                else {
                    otherCard = testGame.rounds[otherRoundIndex].south;
                    testGame.rounds[otherRoundIndex].south = currentCard;
                }

                testGame.rounds[previousRoundIndex].south = otherCard;


                GameEngine::computeGame(testGame);

                ValidationResult testValidation = Validator::validate(testGame);


                if (!testValidation.cardIssues.empty()) {
                    continue;
                }


                int leaderIssues = static_cast<int>(testValidation.leaderIssues.size());


                double beforeConfidence =
                    getCardConfidence(game, previousRoundIndex, Player::SOUTH, currentCard) +
                    getCardConfidence(game, otherRoundIndex, otherPlayer, otherCard);

                double afterConfidence =
                    getCardConfidence(game, previousRoundIndex, Player::SOUTH, otherCard) +
                    getCardConfidence(game, otherRoundIndex, otherPlayer, currentCard);

                double confidenceLoss = beforeConfidence - afterConfidence;


                if (leaderIssues < bestLeaderIssues ||
                    (leaderIssues == bestLeaderIssues &&
                     leaderIssues < currentLeaderIssues &&
                     confidenceLoss < bestConfidenceLoss - 1e-9)) {

                    bestLeaderIssues = leaderIssues;
                    bestConfidenceLoss = confidenceLoss;
                    bestGame = testGame;
                    bestCorrectionCount = 2;

                    correctionFound = true;
                    ambiguous = false;
                }
                else if (leaderIssues == bestLeaderIssues &&
                         leaderIssues < currentLeaderIssues &&
                         std::abs(confidenceLoss - bestConfidenceLoss) <= 1e-9) {

                    ambiguous = true;
                }
            }
        }


        // No safe improvement was found
        if (!correctionFound || ambiguous) {
            break;
        }


        game = bestGame;
        corrections += bestCorrectionCount;
    }


    GameEngine::computeGame(game);

    return corrections;
}