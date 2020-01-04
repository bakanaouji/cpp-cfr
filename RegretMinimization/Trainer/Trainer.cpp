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
#include "Game.hpp"
#include "Node.hpp"

namespace Trainer {

Trainer::Trainer(const std::string &mode) : mEngine((std::random_device()())), mNodeTouchedCnt(0), mModeStr(mode) {
    mFolderPath = "../strategies/kuhn";
    boost::filesystem::create_directories(mFolderPath);
    mGame = new Kuhn::Game(mEngine);
}

void Trainer::train(const int iterations) {
    float initialPs[mGame->playerNum()];
    for (int i = 0; i < mGame->playerNum(); ++i) {
        initialPs[i] = 1.0f;
    }
    float utils[mGame->playerNum()];

    for (int i = 0; i < iterations; ++i) {
        for (int p = 0; p < 2; ++p) {
            // game reset
            mGame->reset();
            if (mModeStr == "cfr") {
                mGame->resetForCFR();
                utils[p] = CFR(*mGame, p, initialPs);
            } else if (mModeStr == "chance") {
                utils[p] = chanceSamplingCFR(*mGame, p, initialPs);
            } else if (mModeStr == "external") {
                utils[p] = externalSamplingCFR(*mGame, p);
            } else if (mModeStr == "outcome") {
                utils[p] = std::get<0>(outcomeSamplingCFR(*mGame, p, i, 1, 1, 1));
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

Trainer::~Trainer() {
    for (auto &itr : mNodeMap) {
        delete itr.second;
    }
    delete mGame;
}

float Trainer::CFR(const Kuhn::Game &game, const int playerIndex, const float *ps) {
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
            Kuhn::Game game_cp(game);
            game_cp.step(a);
            const float chanceProbability = game_cp.chanceProbability();
            float pis[game_cp.playerNum()];
            for (int i = 0; i < game_cp.playerNum(); ++i) {
                pis[i] = playerIndex == i ? ps[i] : chanceProbability * ps[i];
            }
            nodeUtil += chanceProbability * CFR(game_cp, playerIndex, pis);
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
        Kuhn::Game game_cp(game);
        game_cp.step(a);
        float pis[game_cp.playerNum()];
        std::memcpy(pis, ps, sizeof(ps[0]) * game_cp.playerNum());
        pis[player] *= strategy[a];
        utils[a] = CFR(game_cp, playerIndex, pis);
        nodeUtil += strategy[a] * utils[a];
    }

    if (player == playerIndex) {
        // for each action, compute and accumulate counterfactual regret
        float psProd = 1.0f;
        for (int i = 0; i < game.playerNum(); ++i) {
            if (i != player) {
                psProd *= ps[i];
            }
        }
        for (int a = 0; a < actionNum; ++a) {
            const float regret = utils[a] - nodeUtil;
            const float regretSum = node->regretSum(a) + psProd * regret;
            node->regretSum(a, regretSum);
        }
        // update average strategy across all training iterations
        node->strategySum(strategy, ps[player]);
    }

    return nodeUtil;
}

float Trainer::chanceSamplingCFR(const Kuhn::Game &game, const int playerIndex, const float *ps) {
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
        Kuhn::Game game_cp(game);
        game_cp.step(a);
        float pis[game_cp.playerNum()];
        std::memcpy(pis, ps, sizeof(ps[0]) * game_cp.playerNum());
        pis[player] *= strategy[a];
        utils[a] = chanceSamplingCFR(game_cp, playerIndex, pis);
        nodeUtil += strategy[a] * utils[a];
    }

    if (player == playerIndex) {
        // for each action, compute and accumulate counterfactual regret
        float psProd = 1.0f;
        for (int i = 0; i < game.playerNum(); ++i) {
            if (i != player) {
                psProd *= ps[i];
            }
        }
        for (int a = 0; a < actionNum; ++a) {
            const float regret = utils[a] - nodeUtil;
            const float regretSum = node->regretSum(a) + psProd * regret;
            node->regretSum(a, regretSum);
        }
        // update average strategy across all training iterations
        node->strategySum(strategy, ps[player]);
    }

    return nodeUtil;
}

float Trainer::externalSamplingCFR(const Kuhn::Game &game, const int playerIndex) {
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
        Kuhn::Game game_cp(game);
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
        Kuhn::Game game_cp(game);
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

std::tuple<float, float> Trainer::outcomeSamplingCFR(const Kuhn::Game &game, const int playerIndex, const int iteration , const float p0, const float p1, const float s) {
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

    float util, pTail;
    if (player == playerIndex) {
        // for sampled action, recursively call cfr with additional history and probability
        Kuhn::Game game_cp(game);
        game_cp.step(action);
        std::tuple<float, float> ret = outcomeSamplingCFR(game_cp, playerIndex, iteration, p0 * strategy[action], p1,
                                                            s * probability[action]);
        util = std::get<0>(ret);
        pTail = std::get<1>(ret);
        const float W = util * p1;

        // for each action, compute and accumulate counterfactual regret
        for (int a = 0; a < actionNum; ++a) {
            const float regret = a == action ? W * (1.0f - strategy[action]) * pTail : -W * pTail * strategy[action];
            const float regretSum = node->regretSum(a) + regret;
            node->regretSum(a, regretSum);
        }
    } else {
        // for sampled action, recursively call cfr with additional history and probability
        Kuhn::Game game_cp(game);
        game_cp.step(action);
        std::tuple<float, float> ret = outcomeSamplingCFR(game_cp, playerIndex, iteration, p0, p1 * strategy[action],
                                                            s * probability[action]);
        util = std::get<0>(ret);
        pTail = std::get<1>(ret);
        // update average strategy
        node->strategySum(strategy, p1 / s);
    }
    return std::make_tuple(util, pTail * strategy[action]);
}

void Trainer::writeStrategyToJson(const int iteration) const {
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

