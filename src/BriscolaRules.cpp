#include "BriscolaRules.h"

#include <stdexcept>


int BriscolaRules::getCardStrength(const Card& card) {

    switch (card.value) {
        case 1:  return 10;
        case 3:  return 9;
        case 10: return 8;
        case 9:  return 7;
        case 8:  return 6;
        case 7:  return 5;
        case 6:  return 4;
        case 5:  return 3;
        case 4:  return 2;
        case 2:  return 1;

        default:
            throw std::invalid_argument("Invalid card value");
    }
}


int BriscolaRules::getCardPoints(const Card& card) {

    switch (card.value) {
        case 1:  return 11;
        case 3:  return 10;
        case 10: return 4;
        case 9:  return 3;
        case 8:  return 2;
        case 7:  return 0;
        case 6:  return 0;
        case 5:  return 0;
        case 4:  return 0;
        case 2:  return 0;

        default:
            throw std::invalid_argument("Invalid card value");
    }
}


int BriscolaRules::getRoundPoints(const Card& north, const Card& south) {
    return getCardPoints(north) + getCardPoints(south);
}


Player BriscolaRules::findWinner(const Card& north, const Card& south, const Card& briscola, Player leader) {
    const Card& firstCard = (leader == Player::NORTH) ? north : south;

    const Card& secondCard = (leader == Player::NORTH) ? south : north;

    Player secondPlayer = (leader == Player::NORTH) ? Player::SOUTH : Player::NORTH;

    bool firstIsBriscola = 0;
    bool secondIsBriscola = 0;

    bool firstIsBriscola = firstCard.type == briscola.type;
    bool secondIsBriscola = secondCard.type == briscola.type;

    // If at least one briscola card is played.
    if (firstIsBriscola || secondIsBriscola) {

        // If both cards are briscola, the strongest one wins.
        if (firstIsBriscola && secondIsBriscola) {
            return getCardStrength(firstCard) > getCardStrength(secondCard) ? leader : secondPlayer;
        }

        // Otherwise, the only briscola card wins.
        return firstIsBriscola ? leader : secondPlayer;
    }

    // If the second card has a different type, the first played card wins.
    if (firstCard.type != secondCard.type) {
        return leader;
    }

    // Same type: the strongest card wins.
    return getCardStrength(firstCard) > getCardStrength(secondCard) ? leader : secondPlayer;

}