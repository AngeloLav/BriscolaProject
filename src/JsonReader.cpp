#include "JsonReader.h"

#include <fstream>
#include <stdexcept>
#include <libraries/json.hpp>

using json = nlohmann::json;


//Converts a string read from JSON into the corresponding CardType.
static CardType parseCardType(const std::string& type) {

    if (type == "DENARI")
        return CardType::COINS;

    if (type == "BASTONI")
        return CardType::CLUBS;

    if (type == "COPPE")
        return CardType::CUPS;

    if (type == "SPADE")
        return CardType::SPADES;

    throw std::runtime_error("Invalid card type: " + type);
}

//Converts a string read from JSON into the corresponding Player.
static Player parsePlayer(const std::string& player) {

    if (player == "NORTH")
        return Player::NORTH;

    if (player == "SOUTH")
        return Player::SOUTH;

    throw std::runtime_error("Invalid player: " + player);
}


// Reads a single card candidate from JSON.
static CardDetected parseCardDetected(const json& data) {

    CardDetected detected;

    detected.card.type =
        parseCardType(data.at("card").at("type").get<std::string>());

    detected.card.value =
        data.at("card").at("value").get<int>();

    detected.confidence =
        data.at("confidence").get<double>();

    return detected;
}


// Reads a leader candidate from JSON.
static PlayerDetected parsePlayerDetected(const json& data) {

    PlayerDetected detected;

    detected.player =
        parsePlayer(data.at("player").get<std::string>());

    detected.confidence =
        data.at("confidence").get<double>();

    return detected;
}


// Reads a complete GamePrediction from a JSON file.
GamePrediction JsonReader::readGamePrediction(const std::string& filePath) {

    std::ifstream inputFile(filePath);

    if (!inputFile.is_open()) {
        throw std::runtime_error(
            "Unable to open JSON file: " + filePath
        );
    }

    json data;
    inputFile >> data;

    GamePrediction prediction;


    // Read all round predictions.
    for (const auto& roundJson : data.at("rounds")) {

        RoundPrediction roundPrediction;

        roundPrediction.round =
            roundJson.at("round").get<int>();


        // Read North card candidates.
        for (const auto& candidate : roundJson.at("northDetected")) {

            roundPrediction.northDetected.push_back(
                parseCardDetected(candidate)
            );
        }


        // Read South card candidates.
        for (const auto& candidate : roundJson.at("southDetected")) {

            roundPrediction.southDetected.push_back(
                parseCardDetected(candidate)
            );
        }


        // Read leader candidates.
        for (const auto& candidate : roundJson.at("leaderDetected")) {

            roundPrediction.leaderDetected.push_back(
                parsePlayerDetected(candidate)
            );
        }


        prediction.rounds.push_back(roundPrediction);
    }


    // Read the global briscola candidates.
    for (const auto& candidate : data.at("briscolaDetected")) {

        prediction.briscolaDetected.push_back(
            parseCardDetected(candidate)
        );
    }


    return prediction;
}