//
// Copyright (c) 2020 Kenshi Abe
//

#ifndef GAME_CONSTANT_HPP
#define GAME_CONSTANT_HPP

namespace Kuhn {

/// number of players
static const int PlayerNum = 2;
/// number of cards in deck
static const int CardNum = PlayerNum + 1;
/// @brief Get number of chance actions at root node
/// @return number of actions
static constexpr int ChanceActionNum() {
    int actionNum = 1;
    for (int i = 0; i < CardNum; ++i) {
        actionNum *= (i + 1);
    }
    return actionNum;
}

}

#endif //GAME_CONSTANT_HPP
