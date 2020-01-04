//
// Created by 阿部 拳之 on 2019/10/29.
//

#include "CFRAgent.hpp"

#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>
#include "Game.hpp"

namespace Agent {

/// コンストラクタ
CFRAgent::CFRAgent(std::mt19937 &engine, const std::string &path) : mEngine(engine) {
    std::ifstream ifs(path);
    boost::archive::binary_iarchive ia(ifs);
    ia >> mStrategy;
    ifs.close();
}

/// デストラクタ
CFRAgent::~CFRAgent() {
    for (auto &itr : mStrategy) {
        delete itr.second;
    }
}

/// 行動を決定
int CFRAgent::action(const Kuhn::Game &game) const {
    if (game.actionNum() == 1) {
        return 0;
    }
    const std::string infoSetStr = game.infoSetStr();
    const float *probability = mStrategy.at(infoSetStr)->averageStrategy();
    std::discrete_distribution<int> dist(probability, probability + game.actionNum());
    return dist(mEngine);
}

} // namespace
