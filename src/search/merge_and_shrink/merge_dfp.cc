#include "merge_dfp.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "transition_system.h"

#include "../option_parser.h"
#include "../option_parser_util.h"
#include "../plugin.h"

#include <algorithm>
#include <cassert>
#include <iostream>

using namespace std;


MergeDFP::MergeDFP(const Options &options)
    : MergeStrategy(), order(Order(options.get_enum("order"))) {
}

void MergeDFP::initialize(const shared_ptr<AbstractTask> task) {
    MergeStrategy::initialize(task);
    /*
      n := remaining_merges + 1 is the number of variables of the planning task
      and thus the number of atomic transition systems. These will be stored at
      indices 0 to n-1 and thus n is the index at which the first composite
      transition system will be stored at.
    */
    border_atomics_composites = remaining_merges + 1;
}

int MergeDFP::get_corrected_index(int index) const {
    assert(order != REGULAR);
    if (order == INVERSE) {
        return index;
    }
    /*
      This method assumes that we iterate over the vector of all
      transition systems in inverted order (from back to front). It returns the
      unmodified index as long as we are in the range of composite
      transition systems (these are thus traversed in order from the last one
      to the first one) and modifies the index otherwise so that the order
      in which atomic transition systems are considered is from the first to the
      last one (from front to back). This is to emulate the previous behavior
      when new transition systems were not inserted after existing transition systems,
      but rather replaced arbitrarily one of the two original transition systems.
    */
    assert(index >= 0);
    if (index >= border_atomics_composites)
        return index;
    return border_atomics_composites - 1 - index;
}

void MergeDFP::compute_label_ranks(shared_ptr<FactoredTransitionSystem> fts,
                                   int index,
                                   vector<int> &label_ranks) const {
    const TransitionSystem &ts = fts->get_ts(index);
    const Distances &distances = fts->get_dist(index);
    int num_labels = fts->get_num_labels();
    // Irrelevant (and inactive, i.e. reduced) labels have a dummy rank of -1
    label_ranks.resize(num_labels, -1);

    for (TSConstIterator group_it = ts.begin();
         group_it != ts.end(); ++group_it) {
        // Relevant labels with no transitions have a rank of infinity.
        int label_rank = INF;
        const vector<Transition> &transitions = group_it.get_transitions();
        bool group_relevant = false;
        if (static_cast<int>(transitions.size()) == ts.get_size()) {
            /*
              A label group is irrelevant in the earlier notion if it has
              exactly a self loop transition for every state.
            */
            for (size_t i = 0; i < transitions.size(); ++i) {
                if (transitions[i].target != transitions[i].src) {
                    group_relevant = true;
                    break;
                }
            }
        } else {
            group_relevant = true;
        }
        if (!group_relevant) {
            label_rank = -1;
        } else {
            for (size_t i = 0; i < transitions.size(); ++i) {
                const Transition &t = transitions[i];
                label_rank = min(label_rank, distances.get_goal_distance(t.target));
            }
        }
        for (LabelConstIter label_it = group_it.begin();
             label_it != group_it.end(); ++label_it) {
            int label_no = *label_it;
            label_ranks[label_no] = label_rank;
        }
    }
}

pair<int, int> MergeDFP::get_next(shared_ptr<FactoredTransitionSystem> fts) {
    assert(initialized());
    assert(!done());

    /*
      Precompute a vector sorted_active_ts_indices which contains all exisiting
      transition systems in the desired order and compute label ranks.
    */
    vector<int> sorted_active_ts_indices;
    vector<vector<int>> transition_system_label_ranks;
    int num_transition_systems = fts->get_size();
    if (order == REGULAR) {
        // TODO: fix this
        for (int ts_index = 0; ts_index < num_transition_systems; ++ts_index) {
            if (fts->is_active(ts_index)) {
                sorted_active_ts_indices.push_back(ts_index);
                transition_system_label_ranks.push_back(vector<int>());
                vector<int> &label_ranks = transition_system_label_ranks.back();
                compute_label_ranks(fts, ts_index, label_ranks);
            }
        }
    } else {
        for (int i = num_transition_systems - 1; i >= 0; --i) {
            /*
              We iterate from back to front, considering the composite
              transition systems in the order from "most recently added" (= at the back
              of the vector) to "first added" (= at border_atomics_composites).
              Afterwards, we consider the atomic transition systems in the "regular"
              order from the first one until the last one. See also explanation
              at get_corrected_index().
            */
            int ts_index = get_corrected_index(i);
            if (fts->is_active(ts_index)) {
                sorted_active_ts_indices.push_back(ts_index);
                transition_system_label_ranks.push_back(vector<int>());
                vector<int> &label_ranks = transition_system_label_ranks.back();
                compute_label_ranks(fts, ts_index, label_ranks);
            }
        }
    }

    int next_index1 = -1;
    int next_index2 = -1;
    int first_valid_pair_index1 = -1;
    int first_valid_pair_index2 = -1;
    int minimum_weight = INF;
    // Go over all pairs of transition systems and compute their weight.
    for (size_t i = 0; i < sorted_active_ts_indices.size(); ++i) {
        int ts_index1 = sorted_active_ts_indices[i];
        const vector<int> &label_ranks1 = transition_system_label_ranks[i];
        assert(!label_ranks1.empty());
        for (size_t j = i + 1; j < sorted_active_ts_indices.size(); ++j) {
            int ts_index2 = sorted_active_ts_indices[j];

            if (fts->get_ts(ts_index1).is_goal_relevant()
                || fts->get_ts(ts_index2).is_goal_relevant()) {
                // Only consider pairs where at least one component is goal relevant.

                // TODO: the 'old' code that took the 'first' pair in case of
                // no finite pair weight could be found, actually took the last
                // one, so we do the same here for the moment.
//                if (first_valid_pair_index1 == -1) {
                // Remember the first such pair
//                    assert(first_valid_pair_index2 == -1);
                first_valid_pair_index1 = ts_index1;
                first_valid_pair_index2 = ts_index2;
//                }

                // Compute the weight associated with this pair
                vector<int> &label_ranks2 = transition_system_label_ranks[j];
                assert(!label_ranks2.empty());
                assert(label_ranks1.size() == label_ranks2.size());
                int pair_weight = INF;
                for (size_t k = 0; k < label_ranks1.size(); ++k) {
                    if (label_ranks1[k] != -1 && label_ranks2[k] != -1) {
                        // label is relevant in both transition_systems
                        int max_label_rank = max(label_ranks1[k], label_ranks2[k]);
                        pair_weight = min(pair_weight, max_label_rank);
                    }
                }
                if (pair_weight < minimum_weight) {
                    minimum_weight = pair_weight;
                    next_index1 = ts_index1;
                    next_index2 = ts_index2;
                }
            }
        }
    }

    if (next_index1 == -1) {
        /*
          TODO: this is not correct (see above)! we take the *last* pair.
          We should eventually change this to be a random ordering.

          No pair with finite weight has been found. In this case, we simply
          take the first pair according to our ordering consisting of at
          least one goal relevant transition system. (We computed that in the
          loop before.)
        */
        assert(next_index2 == -1);
        assert(minimum_weight == INF);
        assert(first_valid_pair_index1 != -1);
        assert(first_valid_pair_index2 != -1);
        next_index1 = first_valid_pair_index1;
        next_index2 = first_valid_pair_index2;
    }

    /*
      There always exists at least one goal relevant transition system,
      assuming that the global goal specification is non-empty. Hence at
      this point, we must have found a pair of transition systems to merge.
    */
    // NOT true if used on a subset of transitions!
    if (next_index1 == -1 || next_index2 == -1) {
        assert(next_index1 == -1 && next_index2 == -1);
        assert(minimum_weight == INF);
        next_index1 = sorted_active_ts_indices[0];
        next_index2 = sorted_active_ts_indices[1];
    }
    assert(next_index1 != -1);
    assert(next_index2 != -1);
    cout << "Next pair of indices: (" << next_index1 << ", " << next_index2 << ")" << endl;
    --remaining_merges;
    return make_pair(next_index1, next_index2);
}

string MergeDFP::name() const {
    return "dfp";
}

static shared_ptr<MergeStrategy>_parse(OptionParser &parser) {
    vector<string> order;
    order.push_back("DFP");
    order.push_back("REGULAR");
    order.push_back("INVERSE");
    parser.add_enum_option("order", order, "order of transition systems", "DFP");
    Options options = parser.parse();
    parser.document_synopsis(
        "Merge strategy DFP",
        "This merge strategy implements the algorithm originally described in the "
        "paper \"Directed model checking with distance-preserving abstractions\" "
        "by Draeger, Finkbeiner and Podelski (SPIN 2006), adapted to planning in "
        "the following paper:\n\n"
        " * Silvan Sievers, Martin Wehrle, and Malte Helmert.<<BR>>\n"
        " [Generalized Label Reduction for Merge-and-Shrink Heuristics "
        "http://ai.cs.unibas.ch/papers/sievers-et-al-aaai2014.pdf].<<BR>>\n "
        "In //Proceedings of the 28th AAAI Conference on Artificial "
        "Intelligence (AAAI 2014)//, pp. 2358-2366. AAAI Press 2014.");
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeDFP>(options);
}

static PluginShared<MergeStrategy> _plugin("merge_dfp", _parse);
