#include "Validator.h"
#include <array>


ValidationResult Validator::validate(const Game& game) {
    ValidationResult result;

    // Number of occurrences of every card
    std::array<std::array<int, 11>, 4> cardCount{};
    // Position where cards appears
    std::array<std::array<std::vector<CardPosition>, 11>,4> cardPositions;

    // Count all cards selected in the 20 rounds
    for (const auto& round : game.rounds) {

        int northType = static_cast<int>(round.north.type);
        int southType =  static_cast<int>(round.south.type);


        cardCount[northType][round.north.value]++;

        cardPositions[northType][round.north.value].push_back({
            round.round,
            Player::NORTH
        });

        cardCount[southType][round.south.value]++;

        cardPositions[southType][round.south.value].push_back({
            round.round,
            Player::SOUTH
        });
    }

    // Find missing and duplicated cards
    for (int type = 0; type < 4; type++) {
        for (int value = 1; value <= 10; value++) {

            Card card;
            card.type = static_cast<CardType>(type);
            card.value = value;

            if (cardCount[type][value] == 0) {
                CardIssue issue;

                issue.type = CardIssueType::MISSING_CARD;
                issue.card = card;

                result.cardIssues.push_back(issue);
            }

            else if (cardCount[type][value] > 1) {
                CardIssue issue;

                issue.type = CardIssueType::DUPLICATE_CARD;
                issue.card = card;
                issue.positions = cardPositions[type][value];

                result.cardIssues.push_back(issue);
            }
        }
    }

    // Check consistency between consecutive rounds
    for (size_t i = 0; i + 1 < game.rounds.size(); i++) {

        if (game.rounds[i].winner != game.rounds[i + 1].leader) {
            LeaderIssue issue;

            issue.previousRound = game.rounds[i].round;
            issue.nextRound = game.rounds[i + 1].round;

            result.leaderIssues.push_back(issue);
        }
    }

    return result;
}