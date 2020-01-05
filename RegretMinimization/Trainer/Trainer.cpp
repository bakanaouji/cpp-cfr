//
// Created by 阿部 拳之 on 2019-08-05.
//

#include "Trainer.hpp"

#include <fstream>
#include <iostream>
#include <vector>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/serialization/unordered_map.hpp>
#include "Node.hpp"

namespace Trainer {

template <typename T>
Trainer<T>::Trainer(const std::string &mode) : mEngine((std::random_device()())), mNodeTouchedCnt(0), mModeStr(mode) {
    mFolderPath = "../strategies/kuhn";
    boost::filesystem::create_directories(mFolderPath);
    mGame = new T(mEngine);
}

template <typename T>
void Trainer<T>::train(const int iterations) {
    float utils[mGame->playerNum()];

    for (int i = 0; i < iterations; ++i) {
        for (int p = 0; p < 2; ++p) {
            // game reset
            mGame->reset();
            if (mModeStr == "cfr") {
                mGame->resetForCFR();
                utils[p] = CFR(*mGame, p, 1.0f, 1.0f);
            } else if (mModeStr == "chance") {
                utils[p] = chanceSamplingCFR(*mGame, p, 1.0f, 1.0f);
            } else if (mModeStr == "external") {
                utils[p] = externalSamplingCFR(*mGame, p);
            } else if (mModeStr == "outcome") {
                utils[p] = std::get<0>(outcomeSamplingCFR(*mGame, p, i, 1.0f, 1.0f, 1.0f));
            } else {
                assert(false);
            }
        }
        if (i % 1000 == 0) {
            std::cout << i << "," << mNodeMap.size() << "," << mNodeTouchedCnt << ",";
            for (int p = 0; p < mGame->playerNum(); ++p) {
                std::cout << utils[p] << ",";
            }
            std::cout << std::endl;
        }
        if (i != 0 && i % 10000000 == 0) {
            writeStrategyToJson(i);
        }
    }

    writeStrategyToJson();
}

template <typename T>
Trainer<T>::~Trainer() {
    for (auto &itr : mNodeMap) {
        delete itr.second;
    }
    delete mGame;
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

    // get information set node or create it if nonexistant
    Node *node = mNodeMap[infoSet];
    if (node == nullptr) {
        node = new Node(actionNum);
        mNodeMap[infoSet] = node;
    }

    // get current strategy through regret-matching
    const float *strategy = node->strategy();

    // for each action, recursively call CFR with additional history and probability
    const int player = game.currentPlayer();
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

    // get information set node or create it if nonexistant
    const int actionNum = game.actionNum();
    std::string infoSet = game.infoSetStr();
    Node *node = mNodeMap[infoSet];
    if (node == nullptr) {
        node = new Node(actionNum);
        mNodeMap[infoSet] = node;
    }

    // get current strategy through regret-matching
    const float *strategy = node->strategy();

    // for each action, recursively call cfr with additional history and probability
    const int player = game.currentPlayer();
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

    // get information set node or create it if nonexistant
    const int actionNum = game.actionNum();
    std::string infoSet = game.infoSetStr();
    Node *node = mNodeMap[infoSet];
    if (node == nullptr) {
        node = new Node(actionNum);
        mNodeMap[infoSet] = node;
    }

    // get current strategy through regret-matching
    const float *strategy = node->strategy();

    // if current player is not the target player, sample a single action and recursively call cfr
    const int player = game.currentPlayer();
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

    // get information set node or create it if nonexistant
    const int actionNum = game.actionNum();
    std::string infoSet = game.infoSetStr();
    Node *node = mNodeMap[infoSet];
    if (node == nullptr) {
        node = new Node(actionNum);
        mNodeMap[infoSet] = node;
    }

    // get current strategy through regret-matching
    const float *strategy = node->strategy();

    // if current player is the target player, sample a single action according to epsilon-on-policy
    // otherwise, sample a single action according to the player's strategy
    const int player = game.currentPlayer();
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

