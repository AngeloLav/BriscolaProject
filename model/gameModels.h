

#pragma once

#include <vector>
#include <string>


enum class CardType {
    COINS,
    CLUBS,
    CUPS,
    SPADES
};

// players identified by their position
enum class Player {
    NORTH,
    SOUTH
};

struct Card {
    CardType type;
    int value;      // 1, 2, 3, ... 10
}; 

// Represents a card detected by a computer vision model, along with the confidence of the detection
struct CardDetected {
    Card card;
    double confidence;
};

// Represents a possible prediction for the player who led the round
struct PlayerDetected {
    Player player;
    double confidence;
};

// prediction for a round (single video), vectors conteins the top candidates for each card and for the leader
struct roundPrediction {
    int round;

    std::vector<CardDetected> northDetected;
    std::vector<CardDetected> southDetected;

    std::vector<PlayerDetected> leaderDetected;
};

// prediction for the game produce by the CV procedure, contains predictions for all
// 20 rounds and candidates for the briscola card
struct GamePrediction {
    std::vector<roundPrediction> rounds;

    std::vector<CardDetected> briscolaDetected;
};

// Final result of a round after possible corrections and validations
struct RoundResult {
    int round;

    Card north;
    Card south;
    Player leader;

    Player winner;  // winner of the round
    int points;     // points won by the winner in this round
};

// Final result of the game after possible corrections and validations
struct Game {
    GamePrediction prediction;

    Card briscola;

    std::vector<RoundResult> rounds;

    int northScore = 0;
    int southScore = 0;

    Player winner;
};