//author: Camilla Bellantuono
#ifndef ANALYZER_HPP
#define ANALYZER_HPP
#include "../model/gameModels.h"
#include <vector>
#include <string>

//the part of the code that is commented was used as test to see if it worked for calculating the winner of the round
//int getCardPoints(int value);
//int getCardRank(int value);
std::string suitToString(CardType type);
Card getMostFreqCard(const std::vector<Card>& cards);
//Player detWinner(Card northCard, Card southCard, Card briscolaCard, Player leader);
std::vector<CardDetected> getRankedCardsWithConfidence(const std::vector<Card>& cards);
#endif