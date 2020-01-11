//
// Copyright (c) 2020 Kenshi Abe
//

#ifndef REGRETMINIMIZATION_CFRAGENT_HPP
#define REGRETMINIMIZATION_CFRAGENT_HPP

#include <random>
#include <string>
#include <unordered_map>
#include "Node.hpp"

namespace Agent {

/// @param Agent that acts according to average strategy obtained by CFR
/// @param T Type of Game
template <typename T>
class CFRAgent {
public:
    /// @param engine Mersenne Twister pseudo-random generator
    /// @param path path to the binary file that represents the average strategy
    explicit CFRAgent(std::mt19937 &engine, const std::string &path);

    ~CFRAgent();

    /// @brief Choose an action at the current game node
    /// @param game game
    /// @return chosen action
    int action(const T &game) const;

    /// @brief Get the probability of choosing each action
    /// @param game game
    /// @return list of the probabilities
    const float *strategy(const T &game) const;

private:
    std::mt19937 &mEngine;
    std::unordered_map<std::string, Trainer::Node *> mStrategy;
};

} // namespace

#endif //REGRETMINIMIZATION_CFRAGENT_HPP
