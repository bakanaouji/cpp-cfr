//
// Copyright (c) 2020 Kenshi Abe
//

#ifndef REGRETMINIMIZATION_KUHN_HPP
#define REGRETMINIMIZATION_KUHN_HPP

namespace Kuhn {

enum class Action : int {
    NONE = -1,
    PASS = 0,
    BET,
    NUM
};

} // namespace

#endif //REGRETMINIMIZATION_KUHN_HPP
