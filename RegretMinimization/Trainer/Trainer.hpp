//
// Copyright (c) 2020 Kenshi Abe
//

#ifndef REGRETMINIMIZATION_TRAINER_HPP
#define REGRETMINIMIZATION_TRAINER_HPP

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

    static std::vector<float> CalculatePayoff(const T &game, const std::vector<std::function<const float *(const T &)>> &strategies);

    void train(const int iterations);

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
