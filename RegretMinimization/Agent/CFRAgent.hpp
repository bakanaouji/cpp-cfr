//
// Created by 阿部 拳之 on 2019/10/29.
//

#ifndef GAME_CFRAGENT_HPP
#define GAME_CFRAGENT_HPP

#include <random>
#include <string>
#include <unordered_map>
#include "Node.hpp"

namespace Kuhn {
class Game;
}

namespace Agent {

/// CFRエージェント
class CFRAgent {
public:
    /// コンストラクタ
    explicit CFRAgent(std::mt19937 &engine, const std::string &path);

    /// デストラクタ
    ~CFRAgent();

    /// 行動を決定
    int action(const Kuhn::Game &game) const;

    /// get probability of choosing each action
    const float *strategy(const Kuhn::Game &game) const;

private:
    /// 乱数生成器
    std::mt19937 &mEngine;
    std::unordered_map<std::string, Trainer::Node *> mStrategy;
};

} // namespace

#endif //GAME_CFRAGENT_HPP
