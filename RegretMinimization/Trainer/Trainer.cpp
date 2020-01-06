//
// Copyright (c) 2020 Kenshi Abe
//

#include "Trainer.hpp"

#include <fstream>
#include <iostream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/unordered_map.hpp>
#include "Node.hpp"

namespace Trainer {

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

template <typename T>
std::vector<float> Trainer<T>::CalculatePayoff(const T &game, const std::vector<std::function<const float *(const T &)>> &strategies) {
    // return payoff for terminal states
    if (game.done()) {
        std::vector<float> payoffs(game.playerNum());
        for (int i = 0; i < game.playerNum(); ++i) {
            payoffs[i] = game.payoff(i);
        }
        return payoffs;
    }

    // chance node turn
    const int actionNum = game.actionNum();
    if (game.isChanceNode()) {
        std::vector<float> nodeUtils(game.playerNum());
        for (int i = 0; i < game.playerNum(); ++i) {
            nodeUtils[i] = 0.0f;
        }
        for (int a = 0; a < actionNum; ++a) {
            auto game_cp(game);
            game_cp.step(a);
            const float chanceProbability = game_cp.chanceProbability();
            std::vector<float> utils = CalculatePayoff(game_cp, strategies);
            for (int i = 0; i < game.playerNum(); ++i) {
                nodeUtils[i] += chanceProbability * utils[i];
            }
        }
        return nodeUtils;
    }

    // for each action, recursively calculate payoff with additional history and probability
    const int player = game.currentPlayer();
    std::vector<float> nodeUtils(game.playerNum());
    for (int i = 0; i < game.playerNum(); ++i) {
        nodeUtils[i] = 0.0f;
    }
    for (int a = 0; a < actionNum; ++a) {
        auto game_cp(game);
        game_cp.step(a);
        std::vector<float> utils = CalculatePayoff(game_cp, strategies);
        for (int i = 0; i < game.playerNum(); ++i) {
            nodeUtils[i] += strategies[player](game)[a] * utils[i];
        }
    }
    return nodeUtils;
}

template <typename T>
void Trainer<T>::train(const int iterations) {
    float utils[mGame->playerNum()];

    for (int i = 0; i < iterations; ++i) {
        for (int p = 0; p < mGame->playerNum(); ++p) {
            if (!mUpdate[p]) {
                continue;
            }
            if (mModeStr == "vanilla") {
                mGame->reset(false);
                utils[p] = CFR(*mGame, p, 1.0f, 1.0f);
            } else {
                mGame->reset();
                if (mModeStr == "chance") {
                    utils[p] = chanceSamplingCFR(*mGame, p, 1.0f, 1.0f);
                } else if (mModeStr == "external") {
                    utils[p] = externalSamplingCFR(*mGame, p);
                } else if (mModeStr == "outcome") {
                    utils[p] = std::get<0>(outcomeSamplingCFR(*mGame, p, i, 1.0f, 1.0f, 1.0f));
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
            writeStrategyToJson(i);
        }
    }

    writeStrategyToJson();
}

template <typename T>
float Trainer<T>::CFR(const T &game, const int playerIndex, const float pi, const float po) {
    ++mNodeTouchedCnt;

    // return payoff for terminal states
    if (game.done()) {
        return game.payoff(playerIndex);
    }

    // chance node turn
    const int actionNum = game.actionNum();
    if (game.isChanceNode()) {
        float nodeUtil = 0.0f;
        for (int a = 0; a < actionNum; ++a) {
            auto game_cp(game);
            game_cp.step(a);
            const float chanceProbability = game_cp.chanceProbability();
            nodeUtil += chanceProbability * CFR(game_cp, playerIndex, pi, po * chanceProbability);
        }
        return nodeUtil;
    }

    // get information set string representation
    std::string infoSet = game.infoSetStr();

    // treat static player as chance node
    const int player = game.currentPlayer();
    if (!mUpdate[player]) {
        float nodeUtil = 0.0f;
        for (int a = 0; a < actionNum; ++a) {
            auto game_cp(game);
            game_cp.step(a);
            const float chanceProbability = float(mFixedStrategies[player].at(infoSet)->averageStrategy()[a]);
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
    const float *strategy = node->strategy();

    // for each action, recursively call CFR with additional history and probability
    float utils[actionNum];
    float nodeUtil = 0;
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
            const float regret = utils[a] - nodeUtil;
            const float regretSum = node->regretSum(a) + po * regret;
            node->regretSum(a, regretSum);
        }
        // update average strategy across all training iterations
        node->strategySum(strategy, pi);
    }

    return nodeUtil;
}

template <typename T>
float Trainer<T>::chanceSamplingCFR(const T &game, const int playerIndex, const float pi, const float po) {
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
    const float *strategy = node->strategy();

    // for each action, recursively call cfr with additional history and probability
    float utils[actionNum];
    float nodeUtil = 0;
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
            const float regret = utils[a] - nodeUtil;
            const float regretSum = node->regretSum(a) + po * regret;
            node->regretSum(a, regretSum);
        }
        // update average strategy across all training iterations
        node->strategySum(strategy, pi);
    }

    return nodeUtil;
}

template <typename T>
float Trainer<T>::externalSamplingCFR(const T &game, const int playerIndex) {
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
    const float *strategy = node->strategy();

    // if current player is not the target player, sample a single action and recursively call cfr
    if (player != playerIndex) {
        auto game_cp(game);
        std::discrete_distribution<int> dist(strategy, strategy + actionNum);
        game_cp.step(dist(mEngine));
        const float util = externalSamplingCFR(game_cp, playerIndex);
        // update average strategy
        node->strategySum(strategy, 1.0f);
        return util;
    }

    // for each action, recursively call cfr with additional history and probability
    float utils[actionNum];
    float nodeUtil = 0;
    for (int a = 0; a < actionNum; ++a) {
        auto game_cp(game);
        game_cp.step(a);
        utils[a] = externalSamplingCFR(game_cp, playerIndex);
        nodeUtil += strategy[a] * utils[a];
    }

    // for each action, compute and accumulate counterfactual regret
    for (int a = 0; a < actionNum; ++a) {
        const float regret = utils[a] - nodeUtil;
        const float regretSum = node->regretSum(a) + regret;
        node->regretSum(a, regretSum);
    }

    return nodeUtil;
}

template <typename T>
std::tuple<float, float> Trainer<T>::outcomeSamplingCFR(const T &game, const int playerIndex, const int iteration , const float pi, const float po, const float s) {
    ++mNodeTouchedCnt;

    // return payoff for terminal states
    if (game.done()) {
        return std::make_tuple(game.payoff(playerIndex) / s, 1.0f);
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
    const float *strategy = node->strategy();

    // if current player is the target player, sample a single action according to epsilon-on-policy
    // otherwise, sample a single action according to the player's strategy
    const float epsilon = 0.6;
    float probability[actionNum];
    if (player == playerIndex) {
        for (int a = 0; a < actionNum; ++a) {
            probability[a] = (epsilon / (float) actionNum) + (1.0f - epsilon) * strategy[a];
        }
    } else {
        for (int a = 0; a < actionNum; ++a) {
            probability[a] = strategy[a];
        }
    }
    std::discrete_distribution<int> dist(probability, probability + actionNum);
    const int action = dist(mEngine);

    // for sampled action, recursively call cfr with additional history and probability
    float util, pTail;
    auto game_cp(game);
    game_cp.step(action);
    const float newPi = pi * (player == playerIndex ? strategy[action] : 1.0f);
    const float newPo = po * (player == playerIndex ? 1.0f : strategy[action]);
    std::tuple<float, float> ret = outcomeSamplingCFR(game_cp, playerIndex, iteration, newPi, newPo, s * probability[action]);
    util = std::get<0>(ret);
    pTail = std::get<1>(ret);
    if (player == playerIndex) {
        // for each action, compute and accumulate counterfactual regret
        const float W = util * po;
        for (int a = 0; a < actionNum; ++a) {
            const float regret = a == action ? W * (1.0f - strategy[action]) * pTail : -W * pTail * strategy[action];
            const float regretSum = node->regretSum(a) + regret;
            node->regretSum(a, regretSum);
        }
    } else {
        // update average strategy
        node->strategySum(strategy, po / s);
    }
    return std::make_tuple(util, pTail * strategy[action]);
}

template <typename T>
void Trainer<T>::writeStrategyToJson(const int iteration) const {
    for (auto &itr : mNodeMap) {
        itr.second->calcAverageStrategy();
//        std::cout << itr.first << ":";
        for (int i = 0; i < itr.first.size(); ++i) {
            std::cout << int(itr.first[i]);
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

