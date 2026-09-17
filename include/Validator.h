// Luca Ferraro

#ifndef VALIDATOR_H
#define VALIDATOR_H

#include <string>
#include <vector>
#include "gameModels.h"


enum class CardIssueType {
    // Indicates that a card was detected more than once
    DUPLICATE_CARD,
    // Indicates that a card is not used in the game
    MISSING_CARD,
};

// Indicates where a detected card appears in the game.
struct CardPosition {
    int round;
    Player player;
};

// Describes a problem related to a card.
// Example: if 6 BASTONI is detected twice,
// type = DUPLICATE_CARD
//     card = 6 BASTONI
//     positions:
//         round 5, NORTH
//         round 18, SOUTH

// Example: if 3 COPPE is not detected,
// type = MISSING_CARD
//     card = 3 COPPE
//     positions: empty
struct CardIssue {
    CardIssueType type;
    Card card;

    // Empty for a missing card.
    // Contains all occurrences for a duplicated card.
    std::vector<CardPosition> positions;
};

// Indicates that the winner of a round is not detected as the leader of the following round
struct LeaderIssue {
    int previousRound;
    int nextRound;
};

// Complete result of the validation.
struct ValidationResult {
    std::vector<CardIssue> cardIssues;
    std::vector<LeaderIssue> leaderIssues;
};


// Checks the deck and the sequence of leaders after the initial game is built
// and after each correction step.
class Validator {

    public:
        // Reports missing or duplicated cards and inconsistent consecutive
        // winner/leader pairs.
        static ValidationResult validate(const Game& game);

        // check if there's the briscola type in the last three rounds, and if the loser of round 17 is the one who receives it
        static bool isBriscolaPositionValid(const Game& game);
};

#endif