//
// Created by 阿部 拳之 on 2019/10/29.
//

#include <iostream>
#include <random>
#include "CFRAgent.hpp"
#include "Game.hpp"
#include "Trainer.hpp"
#include "Trainer.cpp"

int main() {
    std::mt19937 engine((std::random_device()()));
    Kuhn::Game game(engine);

    Trainer::Trainer<Kuhn::Game> trainer("cfr");
    float ps[game.playerNum()];
    for (int i = 0; i < game.playerNum(); ++i) {
        ps[i] = 1.0;
    }
    for (int p = 0; p < game.playerNum(); ++p) {
        // game reset
        game.resetForCFR();
        std::cout << trainer.CFR(game, p, 1.0f, 1.0f) << std::endl;
    }
}
