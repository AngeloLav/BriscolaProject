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
