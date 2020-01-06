//
// Copyright (c) 2020 Kenshi Abe
//

#include "CFRAgent.hpp"

#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>

namespace Agent {

/// constructor
template <typename T>
CFRAgent<T>::CFRAgent(std::mt19937 &engine, const std::string &path) : mEngine(engine) {
    std::ifstream ifs(path);
    boost::archive::binary_iarchive ia(ifs);
    ia >> mStrategy;
    ifs.close();
}

/// destructor
template <typename T>
CFRAgent<T>::~CFRAgent() {
    for (auto &itr : mStrategy) {
        delete itr.second;
    }
}

/// choose action
template <typename T>
int CFRAgent<T>::action(const T &game) const {
    if (game.actionNum() == 1) {
        return 0;
    }
    const std::string infoSetStr = game.infoSetStr();
    const float *probability = mStrategy.at(infoSetStr)->averageStrategy();
    std::discrete_distribution<int> dist(probability, probability + game.actionNum());
    return dist(mEngine);
}

/// get probability of choosing each action
template <typename T>
const float *CFRAgent<T>::strategy(const T &game) const {
    return mStrategy.at(game.infoSetStr())->averageStrategy();
}

} // namespace
