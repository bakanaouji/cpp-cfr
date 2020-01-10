//
// Copyright (c) 2020 Kenshi Abe
//

#include "Game.hpp"
#include "Kuhn.hpp"

#include <algorithm>
#include <cstring>

namespace Kuhn {

/// @param engine Mersenne Twister pseudo-random generator
Game::Game(std::mt19937 &engine) : mEngine(engine), mPayoff(), mCurrentPlayer(-1), mChanceProbability(0.0f), mFirstBetTurn(-1), mBetPlayerNum(0), mTurnNum(0), mDone(false) {
    for (auto & infoSet : mInfoSets) {
        for (uint8_t & i : infoSet) {
            i = 0;
        }
    }
}

/// @brief Copy constructor
/// @param obj source game
Game::Game(const Game &obj) : mEngine(obj.mEngine), mCards(obj.mCards), mPayoff(obj.mPayoff),
                              mCurrentPlayer(obj.mCurrentPlayer), mChanceProbability(obj.mChanceProbability), mFirstBetTurn(obj.mFirstBetTurn), mBetPlayerNum(obj.mBetPlayerNum), mTurnNum(obj.mTurnNum), mDone(obj.mDone) {
    for (int i = 0; i < PlayerNum; ++i) {
        std::memcpy(mInfoSets[i], obj.mInfoSets[i], obj.mTurnNum + 1);
    }
}

/// @brief Get the name of this game
/// @return name of game
std::string Game::name() {
    return "kuhn";
}

/// @brief Get the number of players
/// @return number of players
int Game::playerNum() {
    return PlayerNum;
}

/// @brief reset and start new game
/// @param skipChanceAction whether to sample an action of the chance player.
///                         If this argument is set to false, `step` function is called recursively until
///                         the acting player becomes a player other than the chance player.
void Game::reset(bool skipChanceAction) {
    if (!skipChanceAction) {
        mCurrentPlayer = PlayerNum + 1;
        return;
    }

    for (int i = 0; i < CardNum; ++i) {
        mCards[i] = i;
    }
    // shuffle cards
    for (int c1 = mCards.size() - 1; c1 > 0; --c1) {
        const int c2 = mEngine() % (c1 + 1);
        const int tmp = mCards[c1];
        mCards[c1] = mCards[c2];
        mCards[c2] = tmp;
    }
    for (int i = 0; i < PlayerNum; ++i) {
        mInfoSets[i][0] = mCards[i];
    }
    mTurnNum = 0;
    mCurrentPlayer = 0;
    mFirstBetTurn = -1;
    mBetPlayerNum = 0;
    mDone = false;
}

/// @brief transition current node to the next node
/// @param action action
void Game::step(const int action) {
    // chance node action
    if (mCurrentPlayer == PlayerNum + 1) {
        constexpr int ChanceAN = ChanceActionNum();
        mChanceProbability = 1.0f / float(ChanceAN);
        for (int i = 0; i < CardNum; ++i) {
            mCards[i] = i;
        }
        // shuffle cards
        int a = action;
        for (int c1 = mCards.size() - 1; c1 > 0; --c1) {
            const int c2 = a % (c1 + 1);
            const int tmp = mCards[c1];
            mCards[c1] = mCards[c2];
            mCards[c2] = tmp;
            a = (int) a / (c1 + 1);
        }
        for (int i = 0; i < PlayerNum; ++i) {
            mInfoSets[i][0] = mCards[i];
        }
        mTurnNum = 0;
        mCurrentPlayer = 0;
        mFirstBetTurn = -1;
        mBetPlayerNum = 0;
        mDone = false;
        return;
    }

    // update history
    mTurnNum += 1;
    mBetPlayerNum += action;
    for (int i = 0; i < PlayerNum; ++i) {
        mInfoSets[i][mTurnNum] = action;
    }
    if (mFirstBetTurn == -1 && action == 1) {
        mFirstBetTurn = mTurnNum;
    }

    // update payoff
    const int plays = mTurnNum;
    const int player = plays % PlayerNum;
    int opponents[PlayerNum - 1];
    for (int i = 0; i < PlayerNum - 1; ++i) {
        opponents[i] = (player + i + 1) % PlayerNum;
    }
    if (plays > 1) {
        const bool terminalPass = (mFirstBetTurn > 0 && (mTurnNum - mFirstBetTurn == PlayerNum - 1)) || (mTurnNum == PlayerNum && mFirstBetTurn == -1 && mInfoSets[0][mTurnNum] == 0);
        // all bet
        if (mBetPlayerNum == PlayerNum) {
            const size_t winPlayer = std::distance(mCards.begin(), std::max_element(mCards.begin(), mCards.begin() + PlayerNum));
            mPayoff[winPlayer] = 2 * (PlayerNum - 1);
            for (int i = 0; i < PlayerNum; ++i) {
                if (i == winPlayer) {
                    continue;
                }
                mPayoff[i] = -2;
            }
            mDone = true;
        } else if (terminalPass) {
            // all fold
            if (mBetPlayerNum == 0) {
                const size_t winPlayer = std::distance(mCards.begin(), std::max_element(mCards.begin(), mCards.begin() + PlayerNum));
                mPayoff[winPlayer] = PlayerNum - 1;
                for (int i = 0; i < PlayerNum; ++i) {
                    if (i == winPlayer) {
                        continue;
                    }
                    mPayoff[i] = -1;
                }
                mDone = true;
            }
            // only one player bet
            else if (mBetPlayerNum == 1) {
                mPayoff[player] = PlayerNum - 1;
                for (int i = 0; i < PlayerNum; ++i) {
                    if (i == player) {
                        continue;
                    }
                    mPayoff[i] = -1;
                }
                mDone = true;
            }
            // more than two players bet
            else if (mBetPlayerNum >= 2) {
                std::array<int, PlayerNum> card{};
                card.fill(-1);
                std::array<bool, PlayerNum> isBet{};
                isBet.fill(false);
                for (int i = 0; i < mTurnNum; ++i) {
                    if (mInfoSets[0][i + 1] == 1) {
                        card[i % PlayerNum] = mCards[i % PlayerNum];
                        isBet[i % PlayerNum] = true;
                    }
                }
                const size_t winPlayer = std::distance(card.begin(), std::max_element(card.begin(), card.end()));
                mPayoff[winPlayer] = 2 * (mBetPlayerNum - 1) + (PlayerNum - mBetPlayerNum);
                for (int i = 0; i < PlayerNum; ++i) {
                    if (!isBet[i]) {
                        mPayoff[i] = -1;
                        continue;
                    }
                    if (i == winPlayer) {
                        continue;
                    }
                    mPayoff[i] = -2;
                }
                mDone = true;
            }
        }
    }
    mCurrentPlayer = player;
}

/// @brief Get the payoff of the specified player
/// @param playerIndex index of the player
/// @return payoff
float Game::payoff(const int playerIndex) const {
    return mPayoff[playerIndex];
}

/// @brief Get the information set string representation
/// @return string representation of the current information set
std::string Game::infoSetStr() const {
    return std::string((char *) mInfoSets[mCurrentPlayer], mTurnNum + 1);
}

/// @brief Check whether the game is over
/// @return if the game is over, `true` is returned
bool Game::done() const {
    return mDone;
}

/// @brief Get the number of available actions at the current information set
/// @return number of available actions
int Game::actionNum() const {
    if (mCurrentPlayer == PlayerNum + 1) {
        constexpr int ChanceAN = ChanceActionNum();
        return ChanceAN;
    }
    return (int) Action::NUM;
}

/// @brief Get the index of the acting player
/// @return index of acting player
int Game::currentPlayer() const {
    return mCurrentPlayer;
}

/// @brief Get the likelihood that the last action is chosen by the chance player
/// @return likelihood
float Game::chanceProbability() const {
    return mChanceProbability;
}

/// @brief Check whether the current acting player is the chance player
/// @return if the acting player is the chance player, `true` is returned
bool Game::isChanceNode() const {
    return mCurrentPlayer == PlayerNum + 1;
}

} // namespace
