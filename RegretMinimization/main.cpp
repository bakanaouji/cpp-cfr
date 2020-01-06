//
// Copyright (c) 2020 Kenshi Abe
//

#include "Game.hpp"
#include "Trainer.hpp"
#include "Trainer.cpp"

int main() {
    Trainer::Trainer<Kuhn::Game> trainer("cfr");
    trainer.train(1000000);
}