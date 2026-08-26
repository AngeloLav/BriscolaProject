#include "gameModels.h"
#include "Validator.h"


class ErrorResolver {

public:

    // Tries to correct duplicated and missing cards using 3 methods:

    /*
    * 1. DUPLICATED CARDS
    * Correct card recognition errors using the constraints of the Briscola deck.
    *
    * Duplicated cards are checked first. For every duplicated card, the resolver
    * looks at the alternative candidates detected and checks whether one
    * of them corresponds to a card currently missing from the game.
    *
    * Among the possible corrections, the one with the smallest confidence loss
    * is selected. After every correction the game is validated again, so the
    * remaining duplicated and missing cards are updated.
    *
    * A failed detection is represented by a card with value 0. After the normal
    * duplicate correction, if only one UNKNOWN position and one missing card
    * remain, the missing card can be assigned directly because the solution is
    * forced.
    *
    * Multiple UNKNOWN positions are left unresolved here because the deck
    * constraint alone cannot determine which missing card belongs to each position. 
    */
    static int resolveCardIssues(Game& game); // returns the number of corretions 

    /**
     * 2. BRISCOLA CORRECTION
     * Corrects an incorrect briscola prediction using winner-leader consistency.
     *
     * Every briscola candidate detected is tested by temporarily assigning it to
     * the game and recomputing all round winners.
     *
     * For each candidate, the game is validated and the number of inconsistencies
     * between the winner of a round and the leader of the following round is counted.
     *
     * The candidate producing the fewest leader issues is selected as the most
     * consistent briscola for the game.
     *
     * If multiple candidates produce the same number of issues, the candidate with
     * the highest recognition confidence is selected.
     *
     * After selecting the best candidate, the game is recomputed so that all winners
     * and scores are consistent with the corrected briscola.
     *
     * If the briscola detection is UNKNOWN, try all four types and select the one producing the fewest leader issues
     */
    static int resolveBriscola(Game& game);

};