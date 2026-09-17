// Luca Ferraro

#ifndef GAME_MODELS_H
#define GAME_MODELS_H

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

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

    // Bounding box information for temporal tracking
    cv::Rect bbox;

    // Frame index where the detection was obtained
    int frameIndex;
};

// Represents a possible observation of a card
struct CardObservation {
    std::vector<CardDetected> candidates;
};

// Represents a possible prediction for the player who led the round
struct PlayerDetected {
    Player player;
    double confidence;
};

// prediction for a round (single video), vectors conteins the top candidates for each card and for the leader
struct RoundPrediction {
    int round;

    std::vector<CardDetected> northDetected;
    std::vector<CardDetected> southDetected;

    std::vector<PlayerDetected> leaderDetected;
};

// prediction for the game produce by the CV procedure, contains predictions for all
// 20 rounds and candidates for the briscola card
struct GamePrediction {
    std::vector<RoundPrediction> rounds;

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

#endif