#include "OutputWriter.h"
#include "GameEngine.h"

#include <fstream>
#include <stdexcept>


static std::string cardTypeToString(CardType type) {

    switch (type) {

        case CardType::DENARI:
            return "coins";

        case CardType::BASTONI:
            return "clubs";

        case CardType::COPPE:
            return "cups";

        case CardType::SPADE:
            return "spades";
    }

    return "unknown";
}


static std::string playerToString(Player player) {

    if (player == Player::NORTH) {
        return "North";
    }

    return "South";
}


void OutputWriter::writeTxt(const Game& game, const std::string& filePath) {

    std::ofstream output(filePath);

    if (!output.is_open()) {
        throw std::runtime_error("Unable to create output file: " + filePath);
    }


    for (const auto& round : game.rounds) {

        output << "Round " << round.round << "\n";

        output << "North : "
               << round.north.value << " , "
               << cardTypeToString(round.north.type) << "\n";

        output << "South : "
               << round.south.value << " , "
               << cardTypeToString(round.south.type) << "\n";

        output << "Briscola : "
               << game.briscola.value << " , "
               << cardTypeToString(game.briscola.type) << "\n";

        output << "Leader : "
               << playerToString(round.leader) << "\n";

        output << "Winner : "
               << playerToString(round.winner) << "\n";

        output << "Points : "
               << round.points << "\n";
    }


    output << "----\n";

    output << "Winner : "
           << playerToString(game.winner) << "\n";

    output << "Total Points South : "
           << game.southScore << "\n";

    output << "Total Points North : "
           << game.northScore << "\n";
}


void OutputWriter::writeCsv(const Game& game, const std::string& filePath) {

    std::ofstream output(filePath);

    if (!output.is_open()) {
        throw std::runtime_error("Unable to create output file: " + filePath);
    }


    output << "Round,"
           << "North Number,"
           << "North Suit,"
           << "South Number,"
           << "South Suit,"
           << "Briscola Number,"
           << "Briscola Suit,"
           << "Leader,"
           << "Winner,"
           << "Points\n";


    for (const auto& round : game.rounds) {

        output << round.round << ","
               << round.north.value << ","
               << cardTypeToString(round.north.type) << ","
               << round.south.value << ","
               << cardTypeToString(round.south.type) << ","
               << game.briscola.value << ","
               << cardTypeToString(game.briscola.type) << ","
               << playerToString(round.leader) << ","
               << playerToString(round.winner) << ","
               << round.points << "\n";
    }
}