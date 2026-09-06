#include <string>

struct Game;


class OutputWriter {

public:

    static void writeTxt(const Game& game, const std::string& filePath);

    static void writeCsv(const Game& game, const std::string& filePath);
};