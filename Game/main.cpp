//
// Created by 阿部 拳之 on 2019/10/29.
//

#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "CFRAgent.hpp"
#include "Game.hpp"
#include "Trainer.hpp"
#include "Trainer.cpp"

int main() {
    std::mt19937 engine((std::random_device()()));
    Kuhn::Game game(engine);

    // initialize strategies
    std::vector<std::string> strategyPaths = {"../strategies/" + game.name() + "/strategy_cfr.bin",
                                              "../strategies/" + game.name() + "/strategy_cfr.bin"};
    std::vector<Agent::CFRAgent *> cfragents(strategyPaths.size());
    std::vector<std::function<const float *(const Kuhn::Game &)>> strategies(strategyPaths.size());
    for (int i = 0; i < strategyPaths.size(); ++i) {
        cfragents[i] = new Agent::CFRAgent(engine, strategyPaths[i]);
        const Agent::CFRAgent &agent = *cfragents[i];
        strategies[i] = [&agent](const Kuhn::Game &game) { return agent.strategy(game); };
    }

    // calculate expected payoffs
    Trainer::Trainer<Kuhn::Game> trainer("cfr");
    for (int p = 0; p < game.playerNum(); ++p) {
        // game reset
        game.reset(false);
        std::cout << trainer.calculatePayoff(game, p, strategies) << std::endl;
    }

    // finalize
    for (int i = 0; i < strategyPaths.size(); ++i) {
        delete cfragents[i];
    }
}
