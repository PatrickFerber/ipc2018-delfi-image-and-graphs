#ifndef MERGE_AND_SHRINK_MERGE_STRATEGY_H
#define MERGE_AND_SHRINK_MERGE_STRATEGY_H

#include <utility>

namespace merge_and_shrink {
class FactoredTransitionSystem;

class MergeStrategy {
protected:
    FactoredTransitionSystem &fts;
public:
    explicit MergeStrategy(FactoredTransitionSystem &fts);
    virtual ~MergeStrategy() = default;
    virtual std::pair<int, int> get_next() = 0;

    // tiebreaking statistics
    virtual std::pair<int, int> get_dfp_tiebreaking_statistics() const {
        return std::make_pair(0, 0);
    }
};
}

#endif
