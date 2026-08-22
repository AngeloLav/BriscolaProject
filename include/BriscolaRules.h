#include "gameModels.h"

namespace BriscolaRules {

    // Returns the strength of a card to find the winner of a round
    int getCardStrength(const Card& card);

    // Returns the points of a card
    int getCardPoints(const Card& card);

    // Returns the total number of points collected in a round
    int getRoundPoints(const Card& north, const Card& south);


}