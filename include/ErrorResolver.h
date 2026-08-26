#include "gameModels.h"
#include "Validator.h"


class ErrorResolver {

public:

    // Tries to correct duplicated and missing cards using 3 methods:
    static int resolveCardIssues(Game& game); // returns the number of corretions 

    static int resolveBriscola(Game& game);

};