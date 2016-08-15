#ifndef MERGE_AND_SHRINK_MERGE_TREE_FACTORY_MIASM_H
#define MERGE_AND_SHRINK_MERGE_TREE_FACTORY_MIASM_H

#include "merge_tree_factory.h"

#include "miasm/merge_tree.h" // for MiasmInternal and MiasmExternal

#include "../options/options.h"

#include <vector>
#include <set>

namespace merge_and_shrink {
class MiasmMergeTree;

/**
 * @brief The MIASM merging strategy
 * \nosubgrouping
 */
class MergeTreeFactoryMiasm : public MergeTreeFactory {
private:
    /**
     * The greedy method for computing the maximal weighted packing of
     * the family of subsets
     */
    void greedy_max_set_packing();
    /** @name Protected: Options */
    options::Options options;
    //@{
    /** @brief The enum option that specifies the internal merging strategy */
    const MiasmInternal miasm_internal;
    /** @brief The enum option that specifies the external merging strategy */
    const MiasmExternal miasm_external;
    //@}
    /** @name Protected: Local Computation  */
    //@{
    /** @name Protected: Result Data */
    //@{
    /** @brief the sink sets */
    std::vector<std::set<int> > sink_sets;
    /** @brief The mutually disjoint subsets which together minimize the
     * ratio of R&R states */
    std::vector<std::set<int> > max_packing;
    //@}

    MiasmMergeTree *compute_miasm_merge_tree(
        std::shared_ptr<AbstractTask> task);
protected:
    virtual std::string name() const override;
    virtual void dump_tree_specific_options() const override;
public:
    /** @brief The option-based constructor */
    explicit MergeTreeFactoryMiasm(const options::Options &opts);
    virtual ~MergeTreeFactoryMiasm() override = default;
    virtual std::unique_ptr<MergeTree> compute_merge_tree(
            std::shared_ptr<AbstractTask> task) override;
    static void add_options_to_parser(options::OptionParser &parser);
};
}

#endif
