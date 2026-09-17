// Luca Ferraro

#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "gameModels.h"

class GameEngine {

    public:

        // Creates the initial game using the highest confidence values
        static Game createGame(const GamePrediction& prediction, bool allowLowConfidenceCards = false);

        // Computes winners, points and total scores
        static void computeGame(Game& game);
};

#endif
