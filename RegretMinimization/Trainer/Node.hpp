//
// Copyright (c) 2020 Kenshi Abe
//

#ifndef REGRETMINIMIZATION_NODE_HPP
#define REGRETMINIMIZATION_NODE_HPP

#include <vector>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>

namespace Trainer {

/// @class Node
/// @brief Information set node class definition
class Node {
public:
    /// @param actionNum Number of available actions in this node
    explicit Node(int actionNum = 0);

    ~Node();

    /// @brief Get the current strategy at this node through Regret-Matching
    /// @return mixed strategy
    const double *strategy();

    /// @brief Get the average strategy across all training iterations
    /// @return average mixed strategy
    const double *averageStrategy();

    /// @brief Update the average strategy by doing addition the current strategy weighted by the contribution of the acting player at the this node.
    ///        The contribution is the probability of reaching this node if all players other than the acting player always choose actions leading to this node.
    /// @param strategy current strategy
    /// @param realizationWeight contribution of the acting player at this node
    void strategySum(const double *strategy, double realizationWeight);

    /// @brief Update current strategy
    void updateStrategy();

    /// @brief Get the cumulative counterfactual regret of the specified action
    /// @param action action
    /// @return cumulative counterfactual regret
    double regretSum(int action) const;

    /// @brief Update the cumulative counterfactual regret by doing addition the counterfactual regret weighted by the contribution of the all players other than the acting player at this node.
    /// @param action action
    /// @param value counterfactual regret
    void regretSum(int action, double value);

    /// @brief Get the number of available actions at this node.
    /// @return number of available actions
    uint8_t actionNum() const;

private:
    friend class boost::serialization::access;

    /// @brief Calculate the average strategy across all training iterations
    void calcAverageStrategy();

    template<class Archive>
    void save(Archive &ar, const unsigned int version) const {
        std::vector<double> vec(mAverageStrategy, mAverageStrategy + mActionNum);
        ar & vec;
    }

    template<class Archive>
    void load(Archive &ar, const unsigned int version) {
        std::vector<double> vec;
        ar & vec;
        mActionNum = vec.size();
        delete[] mAverageStrategy;
        mAverageStrategy = new double[vec.size()];
        for (int i = 0; i < vec.size(); ++i) {
            mAverageStrategy[i] = vec[i];
        }
        mAlreadyCalculated = true;
        mNeedToUpdateStrategy = false;
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    uint8_t mActionNum;
    double *mRegretSum;
    double *mStrategy;
    double *mStrategySum;
    double *mAverageStrategy;
    bool mAlreadyCalculated;
    bool mNeedToUpdateStrategy;
};

} // namespace

#endif //REGRETMINIMIZATION_NODE_HPP
