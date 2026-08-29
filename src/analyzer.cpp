//author: Camilla Bellantuono
#include "analyzer.hpp"
#include <map>
#include <algorithm>

//points that are assigned to each card
int getCardPoints(int value){
    switch(value){
        case 1:return 11; //card ace
        case 3:return 10; //card three
        case 10:return 4; //card ten
        case 9:return 3; //card nine
        case 8:return 2; //card eight
        default:return 0; //all other cards have no power
    }
}
//this is the hierarchy of the cards based on how much they value
int getCardRank(int value){
    switch(value){
        case 1:return 10;
        case 3:return 9;
        case 10:return 8;
        case 9:return 7;
        case 8:return 6;
        case 7:return 5;
        case 6:return 4;
        case 5:return 3;
        case 4:return 2;
        case 2:return 1;
        default:return 0;
    }
}
std::string suitToString(CardType type) {
    switch (type) {
        case CardType::COINS: return "COINS";
        case CardType::CLUBS: return "CLUBS";
        case CardType::SPADES: return "SPADES";
        case CardType::CUPS: return "CUPS";
    }return "Unknown";
}
//we look for the most present card in the video (briscola)
Card getMostFreqCard(const std::vector<Card>& cards){
    if (cards.empty()) return { CardType::COINS, 0 };

    std::map<std::pair<int, int>, int> counts;
    for (const auto& c : cards) {
        if (c.value>0) {
            counts[{static_cast<int>(c.type), c.value}]++;
        }
    }
    std::pair<int, int> bestKey={static_cast<int>(CardType::COINS), 0};
    int maxCount = 0;
    for (const auto& entry:counts) {
        if (entry.second>maxCount) {
            maxCount=entry.second;
            bestKey=entry.first;
        }
    }
    return {static_cast<CardType>(bestKey.first), bestKey.second};
}
//we are determining if the winner is the south part or the north part
Player detWinner(Card northCard, Card southCard, Card briscolaCard, Player leader) {
    bool northIsBriscola=(northCard.type==briscolaCard.type);
    bool southIsBriscola=(southCard.type==briscolaCard.type);
    //both play briscola so wins the briscola with higher rank
    if (northIsBriscola && southIsBriscola) {
        return (getCardRank(northCard.value)>getCardRank(southCard.value)) ? Player::NORTH : Player::SOUTH;
    }
    //just one of them plays briscola
    if (northIsBriscola) return Player::NORTH;
    if (southIsBriscola) return Player::SOUTH;
    //the cards played by both have the same type
    if (northCard.type==southCard.type) {
        return (getCardRank(northCard.value)>getCardRank(southCard.value)) ? Player::NORTH : Player::SOUTH;
    }
    return leader;
}
//this function returns a vector of CardDetected which contains the cards detected and their confidence
std::vector<CardDetected> getRankedCardsWithConfidence(const std::vector<Card>& cards) {
    std::vector<CardDetected> result;
    if (cards.empty()) return result;
    std::map<std::pair<int, int>, int> counts;
    for (const auto& c : cards) {
        if (c.value > 0) {
            counts[{static_cast<int>(c.type), c.value}]++;
        }
    }
    double total = static_cast<double>(cards.size());
    for (const auto& entry : counts) {
        CardDetected cd;
        cd.card = { static_cast<CardType>(entry.first.first), entry.first.second };
        cd.confidence = entry.second / total; 
        result.push_back(cd);
    }

    std::sort(result.begin(), result.end(), [](const CardDetected& a, const CardDetected& b) {
        return a.confidence > b.confidence;
    });

    return result;
}
