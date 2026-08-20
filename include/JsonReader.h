#include <string>
#include "gameModels.h"

class JsonReader {
public:
    static GamePrediction readGamePrediction(const std::string& filePath);
};