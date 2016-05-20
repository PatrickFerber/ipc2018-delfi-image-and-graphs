#ifndef MERGE_AND_SHRINK_MERGE_DFP_H
#define MERGE_AND_SHRINK_MERGE_DFP_H

#include "merge_strategy.h"

#include <vector>

namespace merge_and_shrink {
class TransitionSystem;

class MergeDFP : public MergeStrategy {
    // Store the "DFP" ordering in which transition systems should be considered.
    std::vector<int> transition_system_order;
    bool is_goal_relevant(const TransitionSystem &ts) const;
    void compute_label_ranks(
        int index,
        std::vector<int> &label_ranks) const;
    std::pair<int, int> compute_next_pair(
        const std::vector<int> &sorted_active_ts_indices) const;
public:
    MergeDFP(FactoredTransitionSystem &fts,
             std::vector<int> &&transition_system_order);
    virtual ~MergeDFP() override = default;
    virtual std::pair<int, int> get_next() override;
    std::pair<int, int> get_next(const std::vector<int> &sorted_indices);
};
}

#endif
