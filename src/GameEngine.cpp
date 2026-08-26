#include "GameEngine.h"
#include "BriscolaRules.h"

#include <stdexcept>


Game GameEngine::createGame(const GamePrediction& prediction) {

    Game game{};

    // Preserve all original predictions for future validation and error correction
    game.prediction = prediction;

    // Select the highest-confidence briscola candidate if available
    if (!prediction.briscolaDetected.empty()) {
        game.briscola = prediction.briscolaDetected[0].card;
    }
    
    bool cardsComplete = true;

    for (const auto& predictionRound : prediction.rounds) {

        if (predictionRound.leaderDetected.empty()) {
            throw std::runtime_error(
                "Missing leader candidate in round " +
                std::to_string(predictionRound.round)
            );
        }

        RoundResult round{};   // inizializza tutto a 0

        round.round = predictionRound.round;

        if (!predictionRound.northDetected.empty()) {
            round.north = predictionRound.northDetected[0].card;
        }
        else {
            cardsComplete = false;
        }

        if (!predictionRound.southDetected.empty()) {
            round.south = predictionRound.southDetected[0].card;
        }
        else {
            cardsComplete = false;
        }

        round.leader = predictionRound.leaderDetected[0].player;

        game.rounds.push_back(round);
    }

    if (cardsComplete && game.briscola.value != 0) {
        computeGame(game);
    }

    return game;
}


void GameEngine::computeGame(Game& game) {
    game.northScore = 0;
    game.southScore = 0;

    // Compute winners and points for each round, and accumulate total scores
    for (auto& round : game.rounds) {
        round.winner =
            BriscolaRules::findWinner(
                round.north,
                round.south,
                game.briscola,
                round.leader
            );
        round.points =
            BriscolaRules::getRoundPoints(
                round.north,
                round.south
            );

        if (round.winner == Player::NORTH) {
            game.northScore += round.points;
        }
        else {
            game.southScore += round.points;
        }
    }

    if (game.northScore > game.southScore) {
        game.winner = Player::NORTH;
    }
    else {
        game.winner = Player::SOUTH;
    }
}