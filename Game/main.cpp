//
// Copyright (c) 2020 Kenshi Abe
//

#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "CFRAgent.hpp"
#include "CFRAgent.cpp"
#include "Game.hpp"
#include "Trainer.hpp"
#include "Trainer.cpp"

// Specify game class here
#define GAME Kuhn::Game

int main() {
    std::mt19937 engine((std::random_device()()));
    GAME game(engine);

    // initialize strategies
    std::vector<std::string> strategyPaths = {"../strategies/" + game.name() + "/strategy_cfr.bin",
                                              "../strategies/" + game.name() + "/strategy_cfr.bin"};
    std::vector<Agent::CFRAgent<GAME> *> cfragents(strategyPaths.size());
    std::vector<std::function<const float *(const GAME &)>> strategies(strategyPaths.size());
    for (int i = 0; i < strategyPaths.size(); ++i) {
        cfragents[i] = new Agent::CFRAgent<GAME>(engine, strategyPaths[i]);
        const Agent::CFRAgent<GAME> &agent = *cfragents[i];
        strategies[i] = [&agent](const GAME &game) { return agent.strategy(game); };
    }

    // calculate expected payoffs
    game.reset(false);
    std::vector<float> payoffs = Trainer::Trainer<GAME>::CalculatePayoff(game, strategies);
    std::cout << "expected payoffs: (";
    for (int i = 0; i < game.playerNum(); ++i) {
        std::cout << payoffs[i] << ",";
    }
    std::cout << ")" << std::endl;

    // finalize
    for (int i = 0; i < strategyPaths.size(); ++i) {
        delete cfragents[i];
    }
}
