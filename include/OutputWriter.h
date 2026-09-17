// Luca Ferraro

#ifndef OUTPUT_WRITER_H
#define OUTPUT_WRITER_H

#include <string>

struct Game;


// Writes the final game in the two formats used by the project.
// Unknown cards and results for incomplete rounds are left blank in the files.
class OutputWriter {

public:

    // Writes a readable report with one block for each round.
    static void writeTxt(const Game& game, const std::string& filePath);

    // Writes the same information using the ground-truth CSV column layout.
    static void writeCsv(const Game& game, const std::string& filePath);
};

#endif