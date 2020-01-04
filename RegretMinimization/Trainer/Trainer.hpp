//
// Created by 阿部 拳之 on 2019-08-05.
//

#ifndef REGRETMINIMIZATION_TRAINER_HPP
#define REGRETMINIMIZATION_TRAINER_HPP

#include <array>
#include <unordered_map>
#include <random>
#include <string>
#include <tuple>

namespace Kuhn {
class Game;
}

namespace Trainer {
class Node;
}

namespace Trainer {

class Trainer {
public:
    explicit Trainer(const std::string &mode);

    ~Trainer();

    void train(const int iterations);

    float CFR(const Kuhn::Game &game, const int playerIndex, const float* ps, const int depth);

private:
    void writeStrategyToJson(const int iteration = -1) const;

    float chanceSamplingCFR(const Kuhn::Game &game, const int playerIndex, const float p0, const float p1, const int depth);

    float externalSamplingCFR(const Kuhn::Game &game, const int playerIndex, const int depth);

    std::tuple<float, float> outcomeSamplingCFR(const Kuhn::Game &game, const int playerIndex, const int iteration , const float p0, const float p1, const float s, const int depth);

    std::mt19937 mEngine;
    std::unordered_map<std::string, Node *> mNodeMap;
    uint64_t mNodeTouchedCnt;
    Kuhn::Game *mGame;
    std::string mFolderPath;
    const std::string &mModeStr;
};

} // namespace

#endif //REGRETMINIMIZATION_TRAINER_HPP
