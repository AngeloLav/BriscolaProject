// Luca Ferraro

#include "OutputWriter.h"
#include "GameEngine.h"

#include <fstream>
#include <stdexcept>


static std::string cardTypeToString(CardType type) {

    switch (type) {

        case CardType::COINS:
            return "coins";

        case CardType::CLUBS:
            return "clubs";

        case CardType::CUPS:
            return "cups";

        case CardType::SPADES:
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

static void writeCardCsv(std::ofstream& output, const Card& card) {

    if (card.value == 0) {
        return;
    }

    output << card.value << ","
           << cardTypeToString(card.type);
}


void OutputWriter::writeTxt(const Game& game, const std::string& filePath) {

    std::ofstream output(filePath);

    if (!output.is_open()) {
        throw std::runtime_error("Unable to create output file: " + filePath);
    }


    for (const auto& round : game.rounds) {

        output << "Round " << round.round << "\n";

        output << "North : ";
        if (round.north.value != 0) {
            output << round.north.value << " , "
                   << cardTypeToString(round.north.type);
        }
        output << "\n";

        output << "South : ";
        if (round.south.value != 0) {
            output << round.south.value << " , "
                   << cardTypeToString(round.south.type);
        }
        output << "\n";

        output << "Briscola : ";
        if (game.briscola.value != 0) {
            output << game.briscola.value << " , "
                   << cardTypeToString(game.briscola.type);
        }
        output << "\n";

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
        << "North_Number,"
        << "North_Suit,"
        << "South_Number,"
        << "South_Suit,"
        << "Briscola_Number,"
        << "Briscola_Suit,"
        << "Leader,"
        << "Winner,"
        << "Points\n";


    for (const auto& round : game.rounds) {

        output << round.round << ",";
        writeCardCsv(output, round.north);
        output << ",";
        writeCardCsv(output, round.south);
        output << ",";
        writeCardCsv(output, game.briscola);
        output << ","
               << playerToString(round.leader) << ","
               << playerToString(round.winner) << ","
               << round.points << "\n";
    }
}
