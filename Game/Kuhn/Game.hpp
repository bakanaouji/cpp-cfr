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

/// @class Game
/// @brief Kuhn poker
class Game {
public:
    /// @param engine Mersenne Twister pseudo-random generator
    explicit Game(std::mt19937 &engine);

    /// @brief Copy constructor
    /// @param obj source game
    Game(const Game &obj);

    /// @brief Get the name of this game
    /// @return name of game
    static std::string name();

    /// @brief Get the number of players
    /// @return number of players
    static int playerNum();

    /// @brief reset and start new game
    /// @param skipChanceAction whether to sample an action of the chance player.
    ///                         If this argument is set to false, `step` function is called recursively until
    ///                         the acting player becomes a player other than the chance player.
    void reset(bool skipChanceAction = true);

    /// @brief transition current node to the next node
    /// @param action action
    void step(int action);

    /// @brief Get the payoff of the specified player
    /// @param playerIndex index of the player
    /// @return payoff
    double payoff(int playerIndex) const;

    /// @brief Get the information set string representation
    /// @return string representation of the current information set
    std::string infoSetStr() const;

    /// @brief Check whether the game is over
    /// @return if the game is over, `true` is returned
    bool done() const;

    /// @brief Get the number of available actions at the current information set
    /// @return number of available actions
    int actionNum() const;

    /// @brief Get the index of the acting player
    /// @return index of acting player
    int currentPlayer() const;

    /// @brief Get the likelihood that the last action is chosen by the chance player
    /// @return likelihood
    double chanceProbability() const;

    /// @brief Check whether the current acting player is the chance player
    /// @return if the acting player is the chance player, `true` is returned
    bool isChanceNode() const;

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

#endif //GAME_GAME_HPP
