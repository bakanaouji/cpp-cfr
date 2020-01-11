//
// Copyright (c) 2020 Kenshi Abe
//

#include "Node.hpp"

namespace Trainer {

/// @param actionNum Number of available actions in this node
Node::Node(const int actionNum) : mActionNum(actionNum), mAlreadyCalculated(false) {
    mRegretSum = new float[actionNum];
    mStrategy = new float[actionNum];
    mStrategySum = new float[actionNum];
    mAverageStrategy = new float[actionNum];
    for (int a = 0; a < actionNum; ++a) {
        mRegretSum[a] = 0.0f;
        mStrategy[a] = 0.0f;
        mStrategySum[a] = 0.0f;
        mAverageStrategy[a] = 0.0f;
    }
}

Node::~Node() {
    delete[] mRegretSum;
    delete[] mStrategy;
    delete[] mStrategySum;
    delete[] mAverageStrategy;
}

/// @brief Get the current strategy at this node through Regret-Matching
/// @return mixed strategy
const float *Node::strategy() {
    float normalizingSum = 0.0f;
    for (int a = 0; a < mActionNum; ++a) {
        mStrategy[a] = mRegretSum[a] > 0 ? mRegretSum[a] : 0;
        normalizingSum += mStrategy[a];
    }
    for (int a = 0; a < mActionNum; ++a) {
        if (normalizingSum > 0) {
            mStrategy[a] /= normalizingSum;
        } else {
            mStrategy[a] = 1.0f / (float) mActionNum;
        }
    }
    return mStrategy;
}

/// @brief Get the average strategy across all training iterations
/// @return average mixed strategy
const float *Node::averageStrategy() {
    if (!mAlreadyCalculated) {
        calcAverageStrategy();
    }
    return mAverageStrategy;
}

/// @brief Update the average strategy by doing addition the current strategy weighted by the contribution of the acting player at the this node.
///        The contribution is the probability of reaching this node if all players other than the acting player always choose actions leading to this node.
/// @param strategy current strategy
/// @param realizationWeight contribution of the acting player at this node
void Node::strategySum(const float *strategy, const float realizationWeight) {
    for (int a = 0; a < mActionNum; ++a) {
        mStrategySum[a] += realizationWeight * strategy[a];
    }
    mAlreadyCalculated = false;
}

/// @brief Get the cumulative counterfactual regret of the specified action
/// @param action action
/// @return cumulative counterfactual regret
float Node::regretSum(const int action) const {
    return mRegretSum[action];
}

/// @brief Update the cumulative counterfactual regret by doing addition the counterfactual regret weighted by the contribution of the all players other than the acting player at this node.
/// @param action action
/// @param value counterfactual regret
void Node::regretSum(const int action, const float value) {
    mRegretSum[action] = value;
}

/// @brief Get the number of available actions at this node.
/// @return number of available actions
uint8_t Node::actionNum() const {
    return mActionNum;
}

/// @brief Calculate the average strategy across all training iterations
void Node::calcAverageStrategy() {
    // if average strategy has already been calculated, do nothing to reduce the calculation time
    if (mAlreadyCalculated) {
        return;
    }

    // calculate average strategy
    for (int a = 0; a < mActionNum; ++a) {
        mAverageStrategy[a] = 0.0f;
    }
    float normalizingSum = 0.0f;
    for (int a = 0; a < mActionNum; ++a) {
        normalizingSum += mStrategySum[a];
    }
    for (int a = 0; a < mActionNum; ++a) {
        if (normalizingSum > 0) {
            mAverageStrategy[a] = mStrategySum[a] / normalizingSum;
        } else {
            mAverageStrategy[a] = 1.0f / (float) mActionNum;
        }
    }
    mAlreadyCalculated = true;
}

} // namespace
