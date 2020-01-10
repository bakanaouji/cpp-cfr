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

/// @brief Counterfactual regret minimization algorithm
/// @param T Type of Game
template <typename T>
class Trainer {
public:
    /// @param mode variant of CFR algorithm
    /// @param seed random seed
    /// @param strategyPaths paths to the binary files that represent fixed strategies
    explicit Trainer(const std::string &mode, const uint32_t seed, const std::vector<std::string> &strategyPaths = {});

    ~Trainer();

    /// @brief Calculate the expected payoff of each player
    /// @param game game
    /// @param strategies list of strategies for each player
    /// @return list of expected payoffs
    static std::vector<float> CalculatePayoff(const T &game, const std::vector<std::function<const float *(const T &)>> &strategies);

    /// @brief Execute the CFR algorithm to compute an approximate Nash equilibrium
    /// @param iterations number of iterations of CFR
    void train(const int iterations);

private:
    /// @brief Main procedure of vanilla CFR
    /// @param game game
    /// @param playerIndex player whose strategy is updated in the current iteration
    /// @param pi the probability of reaching the current game node if all players other than the acting player always choose actions leading to the current game node
    /// @param po the probability of reaching the current game node if the acting player always chooses actions leading to the current game node
    /// @return expected payoff of the specified player at the current game node
    float CFR(const T &game, const int playerIndex, const float pi, const float po);

    /// @brief Main procedure of chance-sampling MCCFR
    /// @param game game
    /// @param playerIndex player whose strategy is updated in the current iteration
    /// @param pi the probability of reaching the current game node if all players other than the acting player always choose actions leading to the current game node
    /// @param po the probability of reaching the current game node if the acting player and the chance player always choose actions leading to the current game node
    /// @return estimated expected payoff of the specified player at the current game node
    float chanceSamplingCFR(const T &game, const int playerIndex, const float pi, const float po);

    /// @brief Main procedure of external-sampling MCCFR
    /// @param game game
    /// @param playerIndex player whose strategy is updated in the current iteration
    /// @return estimated expected payoff of the specified player at the current game node
    float externalSamplingCFR(const T &game, const int playerIndex);

    /// @brief Main procedure of outcome-sampling MCCFR
    /// @param game game
    /// @param playerIndex player whose strategy is updated in the current iteration
    /// @param pi the probability of reaching the current game node if all players other than the acting player always choose actions leading to the current game node
    /// @param po the probability of reaching the current game node if the acting player and the chance player always choose actions leading to the current game node
    /// @param s the probability of reaching the current game node if the chance player always chooses actions leading to the current game node and the other players act according to the sample profile
    /// @return estimated expected payoff of the specified player at the current game node, and the probability of reaching the terminal game node if the chance player always chooses actions leading to the terminal game node
    std::tuple<float, float> outcomeSamplingCFR(const T &game, const int playerIndex, const int iteration , const float pi, const float po, const float s);

    /// @brief Save the current average strategy as a binary file
    /// @param iteration current iteration
    void writeStrategyToBin(const int iteration = -1) const;

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
