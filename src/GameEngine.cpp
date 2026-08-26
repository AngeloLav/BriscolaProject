#include "GameEngine.h"
#include "BriscolaRules.h"

#include <stdexcept>


Game GameEngine::createGame(const GamePrediction& prediction) {
    if (prediction.briscolaDetected.empty()) {
        throw std::runtime_error("No briscola candidate available");
    }

    Game game;

    // Preserve all original predictions for future validation and error correction
    game.prediction = prediction;

    // Select the highest-confidence briscola candidate
    game.briscola = prediction.briscolaDetected[0].card;


    for (const auto& predictionRound : prediction.rounds) {

        if (predictionRound.northDetected.empty() ||
            predictionRound.southDetected.empty() ||
            predictionRound.leaderDetected.empty()) {

            throw std::runtime_error(
                "Missing candidate in round " + std::to_string(predictionRound.round)
            );
        }

        RoundResult round{};

        round.round = predictionRound.round;

        if (!predictionRound.northDetected.empty()) {
            round.north = predictionRound.northDetected[0].card;
        }

        if (!predictionRound.southDetected.empty()) {
            round.south = predictionRound.southDetected[0].card;
        }

        round.leader = predictionRound.leaderDetected[0].player;

        game.rounds.push_back(round);
    }

    computeGame(game);

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