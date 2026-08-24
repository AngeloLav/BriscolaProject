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

        RoundResult round;

        round.round = predictionRound.round;

        // Initially select the highest confidence velues.
        round.north = predictionRound.northDetected[0].card;
        round.south = predictionRound.southDetected[0].card;
        round.leader = predictionRound.leaderDetected[0].player;

        game.rounds.push_back(round);
    }

    recomputeGame(game);

    return game;
}


