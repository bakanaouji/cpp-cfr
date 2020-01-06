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

/// Agent that acts according to average strategy obtained by CFR
template <typename T>
class CFRAgent {
public:
    /// constructor
    explicit CFRAgent(std::mt19937 &engine, const std::string &path);

    /// destructor
    ~CFRAgent();

    /// choose action
    int action(const T &game) const;

    /// get probability of choosing each action
    const float *strategy(const T &game) const;

private:
    std::mt19937 &mEngine;
    std::unordered_map<std::string, Trainer::Node *> mStrategy;
};

} // namespace

#endif //REGRETMINIMIZATION_CFRAGENT_HPP
