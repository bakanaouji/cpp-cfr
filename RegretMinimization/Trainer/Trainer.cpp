//
// Copyright (c) 2020 Kenshi Abe
//

#include "Trainer.hpp"

#include <iostream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/unordered_map.hpp>
#include "Node.hpp"

namespace Trainer {

/// @param mode variant of CFR algorithm
/// @param seed random seed
/// @param strategyPaths paths to the binary files that represent fixed strategies
template <typename T>
Trainer<T>::Trainer(const std::string &mode, const uint32_t seed, const std::vector<std::string> &strategyPaths) : mEngine(seed), mNodeTouchedCnt(0), mModeStr(mode) {
    mGame = new T(mEngine);
    mFolderPath = "../strategies/" + mGame->name();
    boost::filesystem::create_directories(mFolderPath);
    mFixedStrategies = new std::unordered_map<std::string, Node *>[mGame->playerNum()];
    mUpdate = new bool[mGame->playerNum()];
    for (int i = 0; i < mGame->playerNum(); ++i) {
        if (strategyPaths.size() >= i + 1 && !strategyPaths[i].empty()) {
            std::cout << "load strategy \"" << strategyPaths[i] << "\" as static player " << i << std::endl;
            std::ifstream ifs(strategyPaths[i]);
            boost::archive::binary_iarchive ia(ifs);
            ia >> mFixedStrategies[i];
            ifs.close();
            mUpdate[i] = false;
        } else {
            mUpdate[i] = true;
        }
    }
}

template <typename T>
Trainer<T>::~Trainer() {
    for (auto &itr : mNodeMap) {
        delete itr.second;
    }
    for (int i = 0; i < mGame->playerNum(); ++i) {
        if (mUpdate[i]) {
            continue;
        }
        for (auto &itr : mFixedStrategies[i]) {
            delete itr.second;
        }
    }
    delete[] mFixedStrategies;
    delete[] mUpdate;
    delete mGame;
}

/// @brief Calculate the expected payoff of each player
/// @param game game
/// @param strategies list of strategies for each player
/// @return list of expected payoffs
template <typename T>
std::vector<double> Trainer<T>::CalculatePayoff(const T &game, const std::vector<std::function<const double *(const T &)>> &strategies) {
    // return payoff for terminal states
    if (game.done()) {
        std::vector<double> payoffs(game.playerNum());
        for (int i = 0; i < game.playerNum(); ++i) {
            payoffs[i] = game.payoff(i);
        }
        return payoffs;
    }

    // chance node turn
    const int actionNum = game.actionNum();
    if (game.isChanceNode()) {
        std::vector<double> nodeUtils(game.playerNum());
        for (int i = 0; i < game.playerNum(); ++i) {
            nodeUtils[i] = 0.0;
        }
        for (int a = 0; a < actionNum; ++a) {
            auto game_cp(game);
            game_cp.step(a);
            const double chanceProbability = game_cp.chanceProbability();
            std::vector<double> utils = CalculatePayoff(game_cp, strategies);
            for (int i = 0; i < game.playerNum(); ++i) {
                nodeUtils[i] += chanceProbability * utils[i];
            }
        }
        return nodeUtils;
    }

    // for each action, recursively calculate payoff with additional history and probability
    const int player = game.currentPlayer();
    std::vector<double> nodeUtils(game.playerNum());
    for (int i = 0; i < game.playerNum(); ++i) {
        nodeUtils[i] = 0.0;
    }
    for (int a = 0; a < actionNum; ++a) {
        auto game_cp(game);
        game_cp.step(a);
        std::vector<double> utils = CalculatePayoff(game_cp, strategies);
        for (int i = 0; i < game.playerNum(); ++i) {
            nodeUtils[i] += strategies[player](game)[a] * utils[i];
        }
    }
    return nodeUtils;
}

/// @brief Calculate exploitability of a given strategy profile
/// @param game game
/// @param strategies list of strategies for each player
/// @return exploitability of a given strategy profile
template <typename T>
double Trainer<T>::CalculateExploitability(const T &game, const std::vector<std::function<const double *(const T &)>> &strategies) {
    InfoSets infoSets;
    for (int p = 0; p < game.playerNum(); ++p) {
        auto game_cp(game);
        game_cp.reset(false);
        CreateInfoSets(game_cp, p, strategies, 1.0, infoSets);
    }

    double exploitability = 0.0;
    for (int p = 0; p < game.playerNum(); ++p) {
        auto game_cp(game);
        game_cp.reset(false);
        std::unordered_map<std::string, std::vector<double>> bestResponseStrategies;
        exploitability += CalculateBestResponseValue(game_cp, p, strategies, bestResponseStrategies, 1.0, infoSets);
    }
    return exploitability;
}

/// @brief Fill the ordered map that maps information sets to game nodes and reach probabilities
/// @param game game
/// @param playerIndex player whose strategy is updated in the current iteration
/// @param strategies list of strategies for each player
/// @param po the probability of reaching the current game node if the acting player always chooses actions leading to the current game node
/// @return exploitability of a given strategy profile
template <typename T>
void Trainer<T>::CreateInfoSets(const T &game, const int playerIndex, const std::vector<std::function<const double *(const T &)>> &strategies, const double po, InfoSets &infoSets) {
    // return at terminal states
    if (game.done()) {
        return;
    }

    // chance node turn
    const int actionNum = game.actionNum();
    if (game.isChanceNode()) {
        for (int a = 0; a < actionNum; ++a) {
            auto game_cp(game);
            game_cp.step(a);
            const double chanceProbability = game_cp.chanceProbability();
            CreateInfoSets(game_cp, playerIndex, strategies, po * chanceProbability, infoSets);
        }
        return;
    }

    const int player = game.currentPlayer();
    if (player == playerIndex) {
        std::string infoSet = game.infoSetStr();
        if (infoSets.count(infoSet) == 0) {
            infoSets[infoSet] = std::vector<std::tuple<T, double>>();
        }
        infoSets[infoSet].push_back(std::make_tuple(game, po));
    }

    for (int a = 0; a < actionNum; ++a) {
        auto game_cp(game);
        game_cp.step(a);
        if (player == playerIndex) {
            CreateInfoSets(game_cp, playerIndex, strategies, po, infoSets);
        } else {
            const double actionProb = strategies[player](game)[a];
            CreateInfoSets(game_cp, playerIndex, strategies, po * actionProb, infoSets);
        }
    }
}

/// @brief Calculate best response value for a given player
/// @param game game
/// @param playerIndex player whose strategy is updated in the current iteration
/// @param strategies list of strategies for each player
/// @param bestResponseStrategies best response strategy for a given player
/// @param po the probability of reaching the current game node if the acting player always chooses actions leading to the current game node
/// @param infoSets ordered map that maps information sets to game nodes and reach probabilities
/// @return best response value for a given player
template <typename T>
double Trainer<T>::CalculateBestResponseValue(const T &game, const int playerIndex,
                                             const std::vector<std::function<const double *(const T &)>> &strategies,
                                             std::unordered_map<std::string, std::vector<double>> &bestResponseStrategies,
                                             const double po,
                                             const InfoSets &infoSets) {
    // return payoff for terminal states
    if (game.done()) {
        return game.payoff(playerIndex);
    }

    // chance node turn
    const int actionNum = game.actionNum();
    if (game.isChanceNode()) {
        double nodeUtil = 0.0;
        for (int a = 0; a < actionNum; ++a) {
            auto game_cp(game);
            game_cp.step(a);
            const double chanceProbability = game_cp.chanceProbability();
            nodeUtil += chanceProbability * CalculateBestResponseValue(game_cp, playerIndex, strategies, bestResponseStrategies, po * chanceProbability, infoSets);
        }
        return nodeUtil;
    }

    const int player = game.currentPlayer();
    if (player == playerIndex) {
        // get information set string representation
        std::string infoSet = game.infoSetStr();
        if (bestResponseStrategies.count(infoSet) == 0) {
            // calculate action values
            double actionValues[actionNum];
            for (int a = 0; a < actionNum; ++a) {
                actionValues[a] = 0.0;
            }
            for (int i = 0; i < infoSets.at(infoSet).size(); ++i) {
                auto game_ = std::get<0>(infoSets.at(infoSet)[i]);
                auto po_ = std::get<1>(infoSets.at(infoSet)[i]);
                double brValues[actionNum];
                for (int a = 0; a < actionNum; ++a) {
                    auto game_cp(game_);
                    game_cp.step(a);
                    brValues[a] = CalculateBestResponseValue(game_cp, playerIndex, strategies, bestResponseStrategies, po_, infoSets);
                    actionValues[a] += po_ * brValues[a];
                }
            }
            // calculate best response strategy
            int brAction = 0;
            for (int a = 0; a < actionNum; ++a) {
                if (actionValues[a] > actionValues[brAction]) {
                    brAction = a;
                }
            }
            bestResponseStrategies[infoSet] = std::vector<double>(actionNum, 0.0);
            bestResponseStrategies[infoSet][brAction] = 1.0;
        }
        // calculate best response value
        double utils[actionNum];
        for (int a = 0; a < actionNum; ++a) {
            auto game_cp(game);
            game_cp.step(a);
            utils[a] = CalculateBestResponseValue(game_cp, playerIndex, strategies, bestResponseStrategies, po, infoSets);
        }
        double bestResponseValue = 0.0;
        for (int a = 0; a < actionNum; ++a) {
            bestResponseValue += utils[a] * bestResponseStrategies.at(infoSet)[a];
        }
        return bestResponseValue;
    } else {
        // for each action, recursively calculate payoff with additional history and probability
        double nodeUtil = 0.0;
        for (int a = 0; a < actionNum; ++a) {
            auto game_cp(game);
            game_cp.step(a);
            const double actionProb = strategies[player](game)[a];
            nodeUtil += actionProb * CalculateBestResponseValue(game_cp, playerIndex, strategies, bestResponseStrategies, po * actionProb, infoSets);
        }
        return nodeUtil;
    }
}

/// @brief Execute the CFR algorithm to compute an approximate Nash equilibrium
/// @param iterations number of iterations of CFR
template <typename T>
void Trainer<T>::train(const int iterations) {
    double utils[mGame->playerNum()];

    for (int i = 0; i < iterations; ++i) {
        for (int p = 0; p < mGame->playerNum(); ++p) {
            if (!mUpdate[p]) {
                continue;
            }
            if (mModeStr == "vanilla") {
                mGame->reset(false);
                utils[p] = CFR(*mGame, p, 1.0, 1.0);
                for (auto & itr : mNodeMap) {
                    itr.second->updateStrategy();
                }
            } else {
                mGame->reset();
                if (mModeStr == "chance") {
                    utils[p] = chanceSamplingCFR(*mGame, p, 1.0, 1.0);
                    for (auto & itr : mNodeMap) {
                        itr.second->updateStrategy();
                    }
                } else if (mModeStr == "external") {
                    utils[p] = externalSamplingCFR(*mGame, p);
                } else if (mModeStr == "outcome") {
                    utils[p] = std::get<0>(outcomeSamplingCFR(*mGame, p, i, 1.0, 1.0, 1.0));
                } else {
                    assert(false);
                }
            }
        }
        if (i % 1000 == 0) {
            std::cout << "iteration:" << i << ", cumulative nodes touched: " << mNodeTouchedCnt << ", infosets num: " << mNodeMap.size() << ", expected payoffs: (";
            for (int p = 0; p < mGame->playerNum(); ++p) {
                std::cout << utils[p] << ",";
            }
            std::cout << ")" << std::endl;
        }
        if (i != 0 && i % 10000000 == 0) {
            writeStrategyToBin(i);
        }
    }

    writeStrategyToBin();
}

/// @brief Main procedure of vanilla CFR
/// @param game game
/// @param playerIndex player whose strategy is updated in the current iteration
/// @param pi the probability of reaching the current game node if all players other than the acting player always choose actions leading to the current game node
/// @param po the probability of reaching the current game node if the acting player always chooses actions leading to the current game node
/// @return expected payoff of the specified player at the current game node
template <typename T>
double Trainer<T>::CFR(const T &game, const int playerIndex, const double pi, const double po) {
    ++mNodeTouchedCnt;

    // return payoff for terminal states
    if (game.done()) {
        return game.payoff(playerIndex);
    }

    // chance node turn
    const int actionNum = game.actionNum();
    if (game.isChanceNode()) {
        double nodeUtil = 0.0;
        for (int a = 0; a < actionNum; ++a) {
            auto game_cp(game);
            game_cp.step(a);
            const double chanceProbability = game_cp.chanceProbability();
            nodeUtil += chanceProbability * CFR(game_cp, playerIndex, pi, po * chanceProbability);
        }
        return nodeUtil;
    }

    // get information set string representation
    std::string infoSet = game.infoSetStr();

    // treat static player as chance node
    const int player = game.currentPlayer();
    if (!mUpdate[player]) {
        double nodeUtil = 0.0;
        for (int a = 0; a < actionNum; ++a) {
            auto game_cp(game);
            game_cp.step(a);
            const auto chanceProbability = double(mFixedStrategies[player].at(infoSet)->averageStrategy()[a]);
            nodeUtil += chanceProbability * CFR(game_cp, playerIndex, pi, po * chanceProbability);
        }
        return nodeUtil;
    }

    // get information set node or create it if nonexistant
    Node *node = mNodeMap[infoSet];
    if (node == nullptr) {
        node = new Node(actionNum);
        mNodeMap[infoSet] = node;
    }

    // get current strategy through regret-matching
    const double *strategy = node->strategy();

    // for each action, recursively call CFR with additional history and probability
    double utils[actionNum];
    double nodeUtil = 0;
    for (int a = 0; a < actionNum; ++a) {
        auto game_cp(game);
        game_cp.step(a);
        if (player == playerIndex) {
            utils[a] = CFR(game_cp, playerIndex, pi * strategy[a], po);
        } else {
            utils[a] = CFR(game_cp, playerIndex, pi, po * strategy[a]);
        }
        nodeUtil += strategy[a] * utils[a];
    }

    if (player == playerIndex) {
        // for each action, compute and accumulate counterfactual regret
        for (int a = 0; a < actionNum; ++a) {
            const double regret = utils[a] - nodeUtil;
            const double regretSum = node->regretSum(a) + po * regret;
            node->regretSum(a, regretSum);
        }
        // update average strategy across all training iterations
        node->strategySum(strategy, pi);
    }

    return nodeUtil;
}

/// @brief Main procedure of chance-sampling MCCFR
/// @param game game
/// @param playerIndex player whose strategy is updated in the current iteration
/// @param pi the probability of reaching the current game node if all players other than the acting player always choose actions leading to the current game node
/// @param po the probability of reaching the current game node if the acting player and the chance player always choose actions leading to the current game node
/// @return estimated expected payoff of the specified player at the current game node
template <typename T>
double Trainer<T>::chanceSamplingCFR(const T &game, const int playerIndex, const double pi, const double po) {
    ++mNodeTouchedCnt;

    // return payoff for terminal states
    if (game.done()) {
        return game.payoff(playerIndex);
    }

    // get information set string representation
    std::string infoSet = game.infoSetStr();

    // treat static player as chance node
    const int actionNum = game.actionNum();
    const int player = game.currentPlayer();
    if (!mUpdate[player]) {
        auto game_cp(game);
        auto strategy = mFixedStrategies[player].at(infoSet)->averageStrategy();
        std::discrete_distribution<int> dist(strategy, strategy + actionNum);
        game_cp.step(dist(mEngine));
        return chanceSamplingCFR(game_cp, playerIndex, pi, po);
    }

    // get information set node or create it if nonexistant
    Node *node = mNodeMap[infoSet];
    if (node == nullptr) {
        node = new Node(actionNum);
        mNodeMap[infoSet] = node;
    }

    // get current strategy through regret-matching
    const double *strategy = node->strategy();

    // for each action, recursively call cfr with additional history and probability
    double utils[actionNum];
    double nodeUtil = 0;
    for (int a = 0; a < actionNum; ++a) {
        auto game_cp(game);
        game_cp.step(a);
        if (player == playerIndex) {
            utils[a] = chanceSamplingCFR(game_cp, playerIndex, pi * strategy[a], po);
        } else {
            utils[a] = chanceSamplingCFR(game_cp, playerIndex, pi, po * strategy[a]);
        }
        nodeUtil += strategy[a] * utils[a];
    }

    if (player == playerIndex) {
        // for each action, compute and accumulate counterfactual regret
        for (int a = 0; a < actionNum; ++a) {
            const double regret = utils[a] - nodeUtil;
            const double regretSum = node->regretSum(a) + po * regret;
            node->regretSum(a, regretSum);
        }
        // update average strategy across all training iterations
        node->strategySum(strategy, pi);
    }

    return nodeUtil;
}

/// @brief Main procedure of external-sampling MCCFR
/// @param game game
/// @param playerIndex player whose strategy is updated in the current iteration
/// @return estimated expected payoff of the specified player at the current game node
template <typename T>
double Trainer<T>::externalSamplingCFR(const T &game, const int playerIndex) {
    ++mNodeTouchedCnt;

    // return payoff for terminal states
    if (game.done()) {
        return game.payoff(playerIndex);
    }

    // get information set string representation
    std::string infoSet = game.infoSetStr();

    // external sampling with stochastically-weighted averaging cannot treat static player
    const int actionNum = game.actionNum();
    const int player = game.currentPlayer();
    assert(mUpdate[player] && "External sampling with stochastically-weighted averaging cannot treat static player.");

    // get information set node or create it if nonexistant
    Node *node = mNodeMap[infoSet];
    if (node == nullptr) {
        node = new Node(actionNum);
        mNodeMap[infoSet] = node;
    }

    // get current strategy through regret-matching
    node->updateStrategy();
    const double *strategy = node->strategy();

    // if current player is not the target player, sample a single action and recursively call cfr
    if (player != playerIndex) {
        auto game_cp(game);
        std::discrete_distribution<int> dist(strategy, strategy + actionNum);
        game_cp.step(dist(mEngine));
        const double util = externalSamplingCFR(game_cp, playerIndex);
        // update average strategy
        node->strategySum(strategy, 1.0);
        return util;
    }

    // for each action, recursively call cfr with additional history and probability
    double utils[actionNum];
    double nodeUtil = 0;
    for (int a = 0; a < actionNum; ++a) {
        auto game_cp(game);
        game_cp.step(a);
        utils[a] = externalSamplingCFR(game_cp, playerIndex);
        nodeUtil += strategy[a] * utils[a];
    }

    // for each action, compute and accumulate counterfactual regret
    for (int a = 0; a < actionNum; ++a) {
        const double regret = utils[a] - nodeUtil;
        const double regretSum = node->regretSum(a) + regret;
        node->regretSum(a, regretSum);
    }

    return nodeUtil;
}

/// @brief Main procedure of outcome-sampling MCCFR
/// @param game game
/// @param playerIndex player whose strategy is updated in the current iteration
/// @param pi the probability of reaching the current game node if all players other than the acting player always choose actions leading to the current game node
/// @param po the probability of reaching the current game node if the acting player and the chance player always choose actions leading to the current game node
/// @param s the probability of reaching the current game node if the chance player always chooses actions leading to the current game node and the other players act according to the sample profile
/// @return estimated expected payoff of the specified player at the current game node, and the probability of reaching the terminal game node if the chance player always chooses actions leading to the terminal game node
template <typename T>
std::tuple<double, double> Trainer<T>::outcomeSamplingCFR(const T &game, const int playerIndex, const int iteration , const double pi, const double po, const double s) {
    ++mNodeTouchedCnt;

    // return payoff for terminal states
    if (game.done()) {
        return std::make_tuple(game.payoff(playerIndex) / s, 1.0);
    }

    // get information set string representation
    std::string infoSet = game.infoSetStr();

    // outcome sampling with stochastically-weighted averaging cannot treat static player
    const int actionNum = game.actionNum();
    const int player = game.currentPlayer();
    assert(mUpdate[player] && "Outcome sampling with stochastically-weighted averaging cannot treat static player.");

    // get information set node or create it if nonexistant
    Node *node = mNodeMap[infoSet];
    if (node == nullptr) {
        node = new Node(actionNum);
        mNodeMap[infoSet] = node;
    }

    // get current strategy through regret-matching
    node->updateStrategy();
    const double *strategy = node->strategy();

    // if current player is the target player, sample a single action according to epsilon-on-policy
    // otherwise, sample a single action according to the player's strategy
    const double epsilon = 0.6;
    double probability[actionNum];
    if (player == playerIndex) {
        for (int a = 0; a < actionNum; ++a) {
            probability[a] = (epsilon / (double) actionNum) + (1.0 - epsilon) * strategy[a];
        }
    } else {
        for (int a = 0; a < actionNum; ++a) {
            probability[a] = strategy[a];
        }
    }
    std::discrete_distribution<int> dist(probability, probability + actionNum);
    const int action = dist(mEngine);

    // for sampled action, recursively call cfr with additional history and probability
    double util, pTail;
    auto game_cp(game);
    game_cp.step(action);
    const double newPi = pi * (player == playerIndex ? strategy[action] : 1.0);
    const double newPo = po * (player == playerIndex ? 1.0 : strategy[action]);
    std::tuple<double, double> ret = outcomeSamplingCFR(game_cp, playerIndex, iteration, newPi, newPo, s * probability[action]);
    util = std::get<0>(ret);
    pTail = std::get<1>(ret);
    if (player == playerIndex) {
        // for each action, compute and accumulate counterfactual regret
        const double W = util * po;
        for (int a = 0; a < actionNum; ++a) {
            const double regret = a == action ? W * (1.0 - strategy[action]) * pTail : -W * pTail * strategy[action];
            const double regretSum = node->regretSum(a) + regret;
            node->regretSum(a, regretSum);
        }
    } else {
        // update average strategy
        node->strategySum(strategy, po / s);
    }
    return std::make_tuple(util, pTail * strategy[action]);
}

/// @brief Save the current average strategy as a binary file
/// @param iteration current iteration
template <typename T>
void Trainer<T>::writeStrategyToBin(const int iteration) const {
    for (auto &itr : mNodeMap) {
        for (char c : itr.first) {
            std::cout << int(c);
        }
        std::cout << ":";
        for (int i = 0; i < itr.second->actionNum(); ++i) {
            std::cout << itr.second->averageStrategy()[i] << ",";
        }
        std::cout << std::endl;
    }
    std::string path = iteration > 0 ? "strategy_" + std::to_string(iteration)
                                     : "strategy";
    path += "_" + mModeStr + ".bin";
    std::ofstream ofs(mFolderPath + "/" + path);
    boost::archive::binary_oarchive oa(ofs);
    oa << mNodeMap;
    ofs.close();
}

} // namespace

