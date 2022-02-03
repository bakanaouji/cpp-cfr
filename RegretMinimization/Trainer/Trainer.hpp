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
    using InfoSets = typename std::unordered_map<std::string, std::vector<std::tuple<T, double>>>;

    /// @param mode variant of CFR algorithm
    /// @param seed random seed
    /// @param strategyPaths paths to the binary files that represent fixed strategies
    explicit Trainer(const std::string &mode, uint32_t seed, const std::vector<std::string> &strategyPaths = {});

    ~Trainer();

    /// @brief Calculate the expected payoff of each player
    /// @param game game
    /// @param strategies list of strategies for each player
    /// @return list of expected payoffs
    static std::vector<double> CalculatePayoff(const T &game, const std::vector<std::function<const double *(const T &)>> &strategies);

    /// @brief Calculate exploitability of a given strategy profile
    /// @param game game
    /// @param strategies list of strategies for each player
    /// @return exploitability of a given strategy profile
    static double CalculateExploitability(const T &game, const std::vector<std::function<const double *(const T &)>> &strategies);

    /// @brief Fill the ordered map that maps information sets to game nodes and reach probabilities
    /// @param game game
    /// @param playerIndex player whose strategy is updated in the current iteration
    /// @param strategies list of strategies for each player
    /// @param po the probability of reaching the current game node if the acting player always chooses actions leading to the current game node
    /// @param infoSets ordered map that maps information sets to game nodes and reach probabilities
    /// @return exploitability of a given strategy profile
    static void CreateInfoSets(const T &game, int playerIndex, const std::vector<std::function<const double *(const T &)>> &strategies, double po, InfoSets &infoSets);

    /// @brief Calculate best response value for a given player
    /// @param game game
    /// @param playerIndex player whose strategy is updated in the current iteration
    /// @param strategies list of strategies for each player
    /// @param bestResponseStrategies best response strategy for a given player
    /// @param po the probability of reaching the current game node if the acting player always chooses actions leading to the current game node
    /// @param infoSets ordered map that maps information sets to game nodes and reach probabilities
    /// @return best response value for a given player
    static double CalculateBestResponseValue(const T &game, int playerIndex, const std::vector<std::function<const double *(const T &)>> &strategies, std::unordered_map<std::string, std::vector<double>> &bestResponseStrategies, double po, const InfoSets &infoSets);

    /// @brief Execute the CFR algorithm to compute an approximate Nash equilibrium
    /// @param iterations number of iterations of CFR
    void train(int iterations);

private:
    /// @brief Main procedure of vanilla CFR
    /// @param game game
    /// @param playerIndex player whose strategy is updated in the current iteration
    /// @param pi the probability of reaching the current game node if all players other than the acting player always choose actions leading to the current game node
    /// @param po the probability of reaching the current game node if the acting player always chooses actions leading to the current game node
    /// @return expected payoff of the specified player at the current game node
    double CFR(const T &game, int playerIndex, double pi, double po);

    /// @brief Main procedure of chance-sampling MCCFR
    /// @param game game
    /// @param playerIndex player whose strategy is updated in the current iteration
    /// @param pi the probability of reaching the current game node if all players other than the acting player always choose actions leading to the current game node
    /// @param po the probability of reaching the current game node if the acting player and the chance player always choose actions leading to the current game node
    /// @return estimated expected payoff of the specified player at the current game node
    double chanceSamplingCFR(const T &game, int playerIndex, double pi, double po);

    /// @brief Main procedure of external-sampling MCCFR
    /// @param game game
    /// @param playerIndex player whose strategy is updated in the current iteration
    /// @return estimated expected payoff of the specified player at the current game node
    double externalSamplingCFR(const T &game, int playerIndex);

    /// @brief Main procedure of outcome-sampling MCCFR
    /// @param game game
    /// @param playerIndex player whose strategy is updated in the current iteration
    /// @param pi the probability of reaching the current game node if all players other than the acting player always choose actions leading to the current game node
    /// @param po the probability of reaching the current game node if the acting player and the chance player always choose actions leading to the current game node
    /// @param s the probability of reaching the current game node if the chance player always chooses actions leading to the current game node and the other players act according to the sample profile
    /// @return estimated expected payoff of the specified player at the current game node, and the probability of reaching the terminal game node if the chance player always chooses actions leading to the terminal game node
    std::tuple<double, double> outcomeSamplingCFR(const T &game, int playerIndex, int iteration , double pi, double po, double s);

    /// @brief Save the current average strategy as a binary file
    /// @param iteration current iteration
    void writeStrategyToBin(int iteration = -1) const;

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
