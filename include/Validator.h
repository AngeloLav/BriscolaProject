#include <string>
#include <vector>
#include "gameModels.h"


enum class IssueType {
    // Indicates that a card was detected more than once
    DUPLICATE_CARD,
    // Indicates that a card is not used in the game
    MISSING_CARD,
    BRISCOLA_NOT_FOUND,
    // Indicates that the winner of a round is not detected as the leader
    LEADER_CONFLICT,
    // Indicates that the total points of the game is not 120
    INVALID_TOTAL_POINTS
};