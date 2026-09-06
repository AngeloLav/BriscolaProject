#include "gameModels.h"

class BriscolaRules {
    public:

        // Returns the strength of a card to find the winner of a round
        static int getCardStrength(const Card& card);

        // Returns the points of a card
        static int getCardPoints(const Card& card);

        // Returns the total number of points collected in a round
        static int getRoundPoints(const Card& north, const Card& south);

        // Determines the winner of a round according to the Briscola rules
        static Player findWinner(
            const Card& north,
            const Card& south,
            const Card& briscola,
            Player leader
        );

};