#include "gameModels.h"

class GameEngine {

    public:

        // Creates the initial game using the highest confidence values
        static Game createGame(const GamePrediction& prediction);

        // Computes winners, points and total scores
        static void computeGame(Game& game);
};