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
     * Corrects an incorrect or missing briscola prediction using game consistency.
     *
     * In the normal case, every detected briscola candidate is tested by recomputing
     * the game and counting winner-leader inconsistencies.
     *
     * The candidate producing the fewest leader issues is preferred. If multiple
     * candidates give the same result, the resolver checks whether the exact briscola
     * appears among the last three cards of the player who should have received it.
     * Recognition confidence is then used as the final tie-breaker.
     *
     * If the briscola detection is UNKNOWN, all four possible suits are tested.
     * The suit producing the fewest leader issues is preferred, while its presence
     * among the last three cards of the expected player is used as an additional
     * consistency check.
     *
     * Once the suit is found, the resolver tries to determine the exact briscola
     * value from those last three cards. If only one card of that suit is present,
     * the briscola is fully resolved. Otherwise, the suit is kept but the value
     * remains UNKNOWN.
     *
     * The last-three-cards constraint is not used as a strict rejection rule because
     * the winner of round 17 or one of the final card detections may still be wrong.
     */
    static int resolveBriscola(Game& game);


    /**
     * 3. WINNER / LEADER CONSISTENCY
     * Corrects inconsistencies between the winner of a round and the leader of
     * the following round.
     *
     * For every leader issue, the resolver considers two possible causes:
     * an incorrect leader prediction in the next round, or an incorrect card
     * prediction in the previous round that caused the wrong winner.
     *
     * Leader alternatives are tested by recomputing the game and checking how many
     * winner-leader inconsistencies remain.
     *
     * Card alternatives are also tested, but since the deck is already complete
     * after the first correction block, cards are swapped with their current
     * position instead of simply being replaced. This preserves the 40-card deck
     * consistency and avoids creating new duplicated or missing cards.
     *
     * Among all possible corrections, the one producing the fewest leader issues
     * is selected. Recognition confidence is used as a tie-breaker when multiple
     * corrections produce the same result.
     *
     * The process is repeated until all leader issues are solved or no safe
     * improvement can be found.
     *
     * Winners are never modified directly because they are computed from the cards,
     * leader and briscola.
     */
    static int resolveLeaderIssues(Game& game);

};