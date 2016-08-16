#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_GOAL_VARIABLE_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_GOAL_VARIABLE_H

#include "merge_scoring_function.h"

namespace merge_and_shrink {
class MergeScoringFunctionGoalVariable : public MergeScoringFunction {
    std::vector<bool> is_goal_variable;
protected:
    virtual std::string name() const override;
public:
    MergeScoringFunctionGoalVariable() = default;
    virtual ~MergeScoringFunctionGoalVariable() override = default;
    virtual std::vector<double> compute_scores(
        FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) override;
    virtual void initialize(std::shared_ptr<AbstractTask> task) override;
};
}

#endif
