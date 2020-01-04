//
// Created by 阿部 拳之 on 2019-08-05.
//

#ifndef REGRETMINIMIZATION_NODE_HPP
#define REGRETMINIMIZATION_NODE_HPP

#include <string>
#include <vector>
#include <iostream>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/vector.hpp>

namespace Trainer {

/// Information set node class definition
class Node {
public:
    explicit Node(const int actionNum = 0);

    ~Node();

    /// get current information set mixed strategy through regret-matching
    const float *strategy();

    /// calculate average information set mixed strategy across all training iterations
    const float *averageStrategy() const;

    void strategySum(const float *strategy, const float realizationWeight);

    /// calculate average information set mixed strategy across all training iterations
    void calcAverageStrategy();

    float regretSum(const int action) const;

    void regretSum(const int action, const float value);

    uint8_t actionNum() const;

private:
    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive &ar, const unsigned int version) const {
        std::vector<float> vec(mAverageStrategy, mAverageStrategy + mActionNum);
        ar & vec;
    }

    template<class Archive>
    void load(Archive &ar, const unsigned int version) {
        std::vector<float> vec;
        ar & vec;
        mActionNum = vec.size();
        if (mAverageStrategy != nullptr) {
            delete[] mAverageStrategy;
        }
        mAverageStrategy = new float[vec.size()];
        for (int i = 0; i < vec.size(); ++i) {
            mAverageStrategy[i] = vec[i];
        }
        mAlreadyCalculated = true;
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    uint8_t mActionNum;
    float *mRegretSum;
    float *mStrategy;
    float *mStrategySum;
    float *mAverageStrategy;
    bool mAlreadyCalculated;
};

} // namespace

#endif //REGRETMINIMIZATION_NODE_HPP
