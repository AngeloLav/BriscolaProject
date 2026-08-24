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