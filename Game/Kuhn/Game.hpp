//
// Created by 阿部 拳之 on 2019-08-05.
//

#ifndef REGRETMINIMIZATION_GAME_HPP
#define REGRETMINIMIZATION_GAME_HPP

#include <array>
#include <random>
#include "Constant.hpp"

namespace Kuhn {

class Game {
public:
    explicit Game(std::mt19937 &engine);

    Game(const Game &obj);

    void reset();

    void resetForCFR();

    void step(const int action);

    double payoff(const int playerIndex) const;

    std::string infoSetStr() const;

    int actionNum() const;

    int currentPlayer() const;

    double chanceProbability() const;

    int playerNum() const;

    bool done() const;

private:
    std::mt19937 &mEngine;
    std::array<int, CardNum> mCards;
    std::array<double, PlayerNum> mPayoff;
    int mCurrentPlayer;
    double mChanceProbability;
    int mFirstBetTurn;
    int mBetPlayerNum;
    int mTurnNum;
    bool mDone;
    uint8_t mInfoSets[PlayerNum][10];
};

} // namespace

#endif //REGRETMINIMIZATION_GAME_HPP
