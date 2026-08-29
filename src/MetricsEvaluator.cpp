#include "MetricsEvaluator.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>


struct GroundTruthRound {

    Card north;
    Card south;
    Card briscola;

    Player leader;
    Player winner;

    int points;
};


static bool sameCard(const Card& first, const Card& second) {

    return first.type == second.type && first.value == second.value;
}


static std::string trim(const std::string& text) {

    size_t first = text.find_first_not_of(" \t\r\n");

    if (first == std::string::npos) {
        return "";
    }

    size_t last = text.find_last_not_of(" \t\r\n");

    return text.substr(first, last - first + 1);
}


static CardType parseCardType(const std::string& type) {

    std::string value = trim(type);

    if (value == "coins") {
        return CardType::DENARI;
    }

    if (value == "clubs") {
        return CardType::BASTONI;
    }

    if (value == "cups") {
        return CardType::COPPE;
    }

    if (value == "spades") {
        return CardType::SPADE;
    }

    throw std::runtime_error("Invalid card type: " + value);
}


static Player parsePlayer(const std::string& player) {

    std::string value = trim(player);

    if (value == "North") {
        return Player::NORTH;
    }

    if (value == "South") {
        return Player::SOUTH;
    }

    throw std::runtime_error("Invalid player: " + value);
}


static std::vector<std::string> splitCsvLine(const std::string& line) {

    std::vector<std::string> values;

    std::stringstream stream(line);
    std::string value;

    while (std::getline(stream, value, ',')) {
        values.push_back(trim(value));
    }

    return values;
}


static std::vector<GroundTruthRound> readGroundTruth(const std::string& filePath) {

    std::ifstream input(filePath);

    if (!input.is_open()) {
        throw std::runtime_error("Unable to open ground truth file: " + filePath);
    }


    std::vector<GroundTruthRound> rounds;

    std::string line;

    // Skip CSV header
    std::getline(input, line);


    while (std::getline(input, line)) {

        if (trim(line).empty()) {
            continue;
        }


        std::vector<std::string> values = splitCsvLine(line);

        if (values.size() != 10) {
            throw std::runtime_error("Invalid ground truth CSV line");
        }


        GroundTruthRound round{};

        round.north.value = std::stoi(values[1]);
        round.north.type = parseCardType(values[2]);

        round.south.value = std::stoi(values[3]);
        round.south.type = parseCardType(values[4]);

        round.briscola.value = std::stoi(values[5]);
        round.briscola.type = parseCardType(values[6]);

        round.leader = parsePlayer(values[7]);
        round.winner = parsePlayer(values[8]);

        round.points = std::stoi(values[9]);


        rounds.push_back(round);
    }


    return rounds;
}


MetricsResult MetricsEvaluator::evaluate(const Game& game, const std::string& groundTruthPath) {

    std::vector<GroundTruthRound> groundTruth = readGroundTruth(groundTruthPath);


    if (groundTruth.size() != game.rounds.size()) {
        throw std::runtime_error("Ground truth and predicted game have different number of rounds");
    }


    MetricsResult metrics;


    /*
     * CARD RECOGNITION
     * Two cards for each of the 20 rounds = 40 predictions.
     */
    for (size_t i = 0; i < game.rounds.size(); i++) {

        if (sameCard(game.rounds[i].north, groundTruth[i].north)) {
            metrics.correctCards++;
        }

        if (sameCard(game.rounds[i].south, groundTruth[i].south)) {
            metrics.correctCards++;
        }
    }


    /*
     * PLAYER IDENTIFICATION
     * Leader and winner for each round = 40 predictions.
     */
    for (size_t i = 0; i < game.rounds.size(); i++) {

        if (game.rounds[i].leader == groundTruth[i].leader) {
            metrics.correctPlayers++;
        }

        if (game.rounds[i].winner == groundTruth[i].winner) {
            metrics.correctPlayers++;
        }
    }


    /*
     * BRISCOLA RECOGNITION
     * The briscola is the same for the whole game.
     */
    if (!groundTruth.empty() && sameCard(game.briscola, groundTruth[0].briscola)) {
        metrics.correctBriscola = 1;
    }


    /*
     * FINAL RESULT
     * Reconstruct ground truth scores from the 20 rounds.
     */
    int groundTruthNorthScore = 0;
    int groundTruthSouthScore = 0;


    for (const auto& round : groundTruth) {

        if (round.winner == Player::NORTH) {
            groundTruthNorthScore += round.points;
        }
        else {
            groundTruthSouthScore += round.points;
        }
    }


    Player groundTruthWinner;

    if (groundTruthNorthScore > groundTruthSouthScore) {
        groundTruthWinner = Player::NORTH;
    }
    else {
        groundTruthWinner = Player::SOUTH;
    }


    if (game.winner == groundTruthWinner) {
        metrics.correctResult++;
    }

    if (game.northScore == groundTruthNorthScore) {
        metrics.correctResult++;
    }

    if (game.southScore == groundTruthSouthScore) {
        metrics.correctResult++;
    }


    return metrics;
}


void MetricsEvaluator::printMetrics(const MetricsResult& metrics) {

    std::cout << "Card recognition accuracy: "
              << metrics.correctCards << "/40" << std::endl;

    std::cout << "Player identification accuracy: "
              << metrics.correctPlayers << "/40" << std::endl;

    std::cout << "Briscola recognition accuracy: "
              << metrics.correctBriscola << "/1" << std::endl;

    std::cout << "Game result accuracy: "
              << metrics.correctResult << "/3" << std::endl;
}


void MetricsEvaluator::writeMetrics(const MetricsResult& metrics, const std::string& filePath) {

    std::ofstream output(filePath);

    if (!output.is_open()) {
        throw std::runtime_error("Unable to create metrics file: " + filePath);
    }


    output << "Card recognition accuracy: "
           << metrics.correctCards << "/40\n";

    output << "Player identification accuracy: "
           << metrics.correctPlayers << "/40\n";

    output << "Briscola recognition accuracy: "
           << metrics.correctBriscola << "/1\n";

    output << "Game result accuracy: "
           << metrics.correctResult << "/3\n";
}