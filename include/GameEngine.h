// Luca Ferraro

#ifndef GAME_ENGINE_H
#define GAME_ENGINE_H

#include "gameModels.h"

// Builds the final game model from the predictions produced by the video
// analysis and computes the results of the individual rounds.
class GameEngine {

    public:

        // Creates the initial game using the highest confidence values.
        // When allowLowConfidenceCards is true, a candidate below the normal
        // threshold is still kept instead of being replaced with UNKNOWN.
        // The complete prediction is copied into the Game for later corrections.
        static Game createGame(const GamePrediction& prediction, bool allowLowConfidenceCards = false);

        // Computes winners, points and total scores.
        // A round with a missing card is skipped, while the other complete
        // rounds can still be evaluated.
        static void computeGame(Game& game);
};

#endif