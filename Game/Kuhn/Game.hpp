//
// Copyright (c) 2020 Kenshi Abe
//

#ifndef GAME_GAME_HPP
#define GAME_GAME_HPP

#include <array>
#include <random>
#include <string>
#include "Constant.hpp"

namespace Kuhn {

class Game {
public:
    explicit Game(std::mt19937 &engine);

    Game(const Game &obj);

    void reset(const bool skipChanceAction = true);

    void step(const int action);

    float payoff(const int playerIndex) const;

    std::string infoSetStr() const;

    int actionNum() const;

    int currentPlayer() const;

    float chanceProbability() const;

    static int playerNum();

    bool done() const;

    bool isChanceNode() const;

    std::string name() const;

private:
    std::mt19937 &mEngine;
    std::array<int, CardNum> mCards;
    std::array<float, PlayerNum> mPayoff;
    int mCurrentPlayer;
    float mChanceProbability;
    int mFirstBetTurn;
    int mBetPlayerNum;
    int mTurnNum;
    bool mDone;
    uint8_t mInfoSets[PlayerNum][10];
};

} // namespace

#endif //GAME_GAME_HPP
