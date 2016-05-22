#ifndef MERGE_AND_SHRINK_MERGE_SCCS_H
#define MERGE_AND_SHRINK_MERGE_SCCS_H

#include "merge_strategy.h"

#include <memory>
#include <vector>

namespace merge_and_shrink {
class MergeDFP;

enum class OrderOfSCCs {
    TOPOLOGICAL,
    REVERSE_TOPOLOGICAL,
    DECREASING,
    INCREASING
};

enum class InternalMergeOrder {
    LINEAR,
    DFP
};

class MergeSCCs : public MergeStrategy {

    InternalMergeOrder internal_merge_order;
    std::vector<int> linear_variable_order;
    std::unique_ptr<MergeDFP> merge_dfp;
    std::vector<std::vector<int>> non_singleton_cg_sccs;
    std::vector<int> indices_of_merged_sccs;
    std::vector<int> current_ts_indices;
    std::pair<int, int> get_next_linear(
        const std::vector<int> available_indices,
        int most_recent_index,
        bool two_indices) const;
public:
    MergeSCCs(FactoredTransitionSystem &fts,
        InternalMergeOrder internal_merge_order,
        std::vector<int> &&linear_variable_order,
        std::unique_ptr<MergeDFP> merge_dfp,
        std::vector<std::vector<int>> &&non_singleton_cg_sccs,
        std::vector<int> &&indices_of_merged_sccs);
    virtual ~MergeSCCs() override = default;
    virtual std::pair<int, int> get_next() override;
    virtual int get_iterations_with_tiebreaking() const override;
    virtual int get_total_tiebreaking_pair_count() const override;
};
}

#endif
