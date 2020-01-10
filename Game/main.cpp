//
// Copyright (c) 2020 Kenshi Abe
//

#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include "cmdline.h"
#include "CFRAgent.hpp"
#include "CFRAgent.cpp"
#include "Game.hpp"
#include "Trainer.hpp"
#include "Trainer.cpp"

// Specify game class here
#define GAME Kuhn::Game

int main(int argc, char *argv[]) {
    // parse arguments
    cmdline::parser p;
    p.add<uint32_t>("seed", 's', "Random seed used to initialize the random generator", false);
    for (int i = 0; i < GAME::playerNum(); ++i) {
        p.add<std::string>("strategy-path-" + std::to_string(i), 0,
                           "Path to the binary file that represents the average strategy for player " + std::to_string(i), true);
    }
    p.parse_check(argc, argv);

    // create game
    std::mt19937 engine(p.exist("seed") ? p.get<uint32_t>("seed") : std::random_device()());
    GAME game(engine);

    // initialize strategies
    std::vector<Agent::CFRAgent<GAME> *> cfrAgents(GAME::playerNum());
    std::vector<std::function<const float *(const GAME &)>> strategies(GAME::playerNum());
    for (int i = 0; i < GAME::playerNum(); ++i) {
        cfrAgents[i] = new Agent::CFRAgent<GAME>(engine, p.get<std::string>("strategy-path-" + std::to_string(i)));
        const Agent::CFRAgent<GAME> &agent = *cfrAgents[i];
        strategies[i] = [&agent](const GAME &game) { return agent.strategy(game); };
    }

    // calculate expected payoffs
    game.reset(false);
    std::vector<float> payoffs = Trainer::Trainer<GAME>::CalculatePayoff(game, strategies);
    std::cout << "expected payoffs: (";
    for (int i = 0; i < GAME::playerNum(); ++i) {
        std::cout << payoffs[i] << ",";
    }
    std::cout << ")" << std::endl;

    // finalize
    for (int i = 0; i < GAME::playerNum(); ++i) {
        delete cfrAgents[i];
    }
}
