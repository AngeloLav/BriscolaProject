#include "Validator.h"
#include <array>


ValidationResult Validator::validate(const Game& game) {
    ValidationResult result;

    // Number of occurrences of every card.
    std::array<std::array<int, 11>, 4> cardCount{};
    // Position where cards appears
    std::array<std::array<CardPosition, 11>, 4> cardPosition;



    return result;
}