//
// Copyright (c) 2020 Kenshi Abe
//

#ifndef GAME_KUHN_HPP
#define GAME_KUHN_HPP

namespace Kuhn {

/// @enum Action
/// @brief actions available in Kuhn poker
enum class Action : int {
    NONE = -1, ///< None
    PASS = 0,  ///< Check or Fold
    BET,       ///< Bet or Call
    NUM        ///< Number of actions
};

} // namespace

#endif //GAME_KUHN_HPP
