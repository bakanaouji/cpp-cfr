//
// Created by 阿部 拳之 on 2019-08-05.
//

#ifndef REGRETMINIMIZATION_TRAINER_HPP
#define REGRETMINIMIZATION_TRAINER_HPP

#include <array>
#include <functional>
#include <random>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Trainer {
class Node;
}

namespace Trainer {

template <typename T>
class Trainer {
public:
    explicit Trainer(const std::string &mode, const std::vector<std::string> &strategyPaths = {});

    ~Trainer();

    void train(const int iterations);

    float calculatePayoff(const T &game, const int playerIndex, const std::vector<std::function<const float *(const T &)>> &strategies);

private:
    void writeStrategyToJson(const int iteration = -1) const;

    float CFR(const T &game, const int playerIndex, const float pi, const float po);

    float chanceSamplingCFR(const T &game, const int playerIndex, const float pi, const float po);

    float externalSamplingCFR(const T &game, const int playerIndex);

    std::tuple<float, float> outcomeSamplingCFR(const T &game, const int playerIndex, const int iteration , const float pi, const float po, const float s);

    std::mt19937 mEngine;
    std::unordered_map<std::string, Node *> mNodeMap;
    uint64_t mNodeTouchedCnt;
    T *mGame;
    std::string mFolderPath;
    const std::string &mModeStr;
    std::unordered_map<std::string, Node *> *mFixedStrategies;
    bool *mUpdate;
};

} // namespace

#endif //REGRETMINIMIZATION_TRAINER_HPP
