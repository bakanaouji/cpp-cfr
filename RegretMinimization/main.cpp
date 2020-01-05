//
// Copyright (c) 2020 Kenshi Abe
//

#include "Trainer.hpp"
#include "Trainer.cpp"
#include "Game.hpp"

int main() {
    Trainer::Trainer<Kuhn::Game> trainer("cfr");
    trainer.train(1000000);
}