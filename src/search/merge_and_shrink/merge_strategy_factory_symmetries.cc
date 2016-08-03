#include "merge_strategy_factory_symmetries.h"

#include "factored_transition_system.h"
#include "merge_scoring_function_dfp.h"
#include "merge_scoring_function_goal_relevance.h"
#include "merge_scoring_function_single_random.h"
#include "merge_scoring_function_total_order.h"
#include "merge_selector_score_based_filtering.h"
#include "merge_symmetries.h"
#include "merge_tree.h"
#include "merge_tree_factory_linear.h"
#include "merge_tree_factory_miasm.h"
#include "transition_system.h"

#include "miasm/merge_tree.h"

#include "symmetries/symmetry_group.h"

#include "../globals.h"
#include "../plugin.h"
#include "../variable_order_finder.h"

#include "../options/option_parser.h"
#include "../options/options.h"

#include "../utils/logging.h"
#include "../utils/system.h"

#include <algorithm>
#include <iomanip>

using namespace std;

namespace merge_and_shrink {
MergeStrategyFactorySymmetries::MergeStrategyFactorySymmetries(
    const options::Options &options)
    : MergeStrategyFactory(), options(options) {
}

void MergeStrategyFactorySymmetries::dump_strategy_specific_options() const {
    cout << "Options for merge symmetries:" << endl;
    cout << "    symmetries for shrinking: ";
    int symmetries_for_shrinking = options.get_enum("symmetries_for_shrinking");
    switch (symmetries_for_shrinking) {
        case 0: {
            cout << "none";
            break;
        }
        case 1: {
            cout << "atomic";
            break;
        }
        case 2: {
            cout << "local";
            break;
        }
    }
    cout << endl;
    cout << "    symmetries for merging: ";
    int symmetries_for_merging = options.get_enum("symmetries_for_merging");
    switch (symmetries_for_merging) {
        case 0: {
            cout << "none";
            break;
        }
        case 1: {
            cout << "smallest";
            break;
        }
        case 2: {
            cout << "largest";
            break;
        }
    }
    cout << endl;
    if (symmetries_for_merging) {
        cout << "    external merging: ";
        switch (options.get_enum("external_merging")) {
            case 0: {
                cout << "merge for atomic symmetry";
                break;
            }
            case 1: {
                cout << "merge for local symmetry";
                break;
            }
        }
        cout << endl;
        cout << "    internal merging: ";
        switch (options.get_enum("internal_merging")) {
            case 0: {
                cout << "linear";
                break;
            }
            case 1: {
                cout << "non linear";
                break;
            }
        }
        cout << endl;
    }
    cout << "    maxium number of m&s iterations with bliss: "
         << options.get<int>("max_bliss_iterations") << endl;
    cout << "    time limit for single bliss calls (0 means unlimited): "
         << options.get<int>("bliss_call_time_limit") << endl;
    cout << "    total time budget for bliss (0 means unlimited): "
         << options.get<int>("bliss_total_time_budget") << endl;
    cout << "    stop searching for symmetries once no symmetry was found: "
         << (options.get<bool>("stop_after_no_symmetries") ? "yes" : "no") << endl;
    cout << "    stabilize transition systems: "
         << (options.get<bool>("stabilize_transition_systems") ? "yes" : "no") << endl;
    cout << "    fallback merge strategy: ";
    switch (FallbackStrategy(options.get_enum("fallback_strategy"))) {
    case LINEAR:
        cout << "linear";
        break;
    case DFP:
        cout << "dfp";
        break;
    case MIASM:
        cout << "miasm";
        break;
    }
    cout << endl;
}

unique_ptr<MergeStrategy> MergeStrategyFactorySymmetries::compute_merge_strategy(
        shared_ptr<AbstractTask> task,
        FactoredTransitionSystem &fts) {
    TaskProxy task_proxy(*task);
    int num_vars = task_proxy.get_variables().size();
    vector<int> linear_merge_order;
    shared_ptr<MergeSelectorScoreBasedFiltering> dfp_selector = nullptr;
    unique_ptr<MergeTree> miasm_merge_tree = nullptr;

    FallbackStrategy fallback_strategy =
        FallbackStrategy(options.get_enum("fallback_strategy"));
    if (fallback_strategy == LINEAR) {
        VariableOrderFinder vof(
            task,
            VariableOrderType(options.get_enum("variable_order")));
        linear_merge_order.reserve(num_vars);
        while (!vof.done()) {
            linear_merge_order.push_back(vof.next());
        }
//        cout << "linear variable order: " << linear_merge_order << endl;
    } else if (fallback_strategy == DFP) {
        vector<shared_ptr<MergeScoringFunction>> scoring_functions;
        scoring_functions.push_back(make_shared<MergeScoringFunctionGoalRelevance>());
        scoring_functions.push_back(make_shared<MergeScoringFunctionDFP>());

        bool randomized_order = options.get<bool>("randomized_order");
        if (randomized_order) {
            shared_ptr<MergeScoringFunctionSingleRandom> scoring_random =
                make_shared<MergeScoringFunctionSingleRandom>(options);
            scoring_functions.push_back(scoring_random);
        } else {
            shared_ptr<MergeScoringFunctionTotalOrder> scoring_total_order =
                make_shared<MergeScoringFunctionTotalOrder>(options);
            scoring_functions.push_back(scoring_total_order);
        }
        dfp_selector = make_shared<MergeSelectorScoreBasedFiltering>(
            move(scoring_functions));
        dfp_selector->initialize(task);
    } else if (fallback_strategy == MIASM) {
        MergeTreeFactoryMiasm factory(options);
        miasm_merge_tree = move(factory.compute_merge_tree(task, fts));
    } else {
        ABORT("unknown fallback merge strategy");
    }
    int num_merges = num_vars - 1;
    return utils::make_unique_ptr<MergeSymmetries>(
        fts,
        options,
        num_merges,
        move(linear_merge_order),
        dfp_selector,
        move(miasm_merge_tree));
}

string MergeStrategyFactorySymmetries::name() const {
    return "symmetries";
}

static shared_ptr<MergeStrategyFactory> _parse(options::OptionParser &parser) {
    // Options for symmetries computation
    parser.add_option<int>("max_bliss_iterations", "maximum ms iteration until "
                           "which bliss is allowed to run.",
                           "infinity");
    parser.add_option<int>("bliss_call_time_limit", "time in seconds one bliss "
                           "run is allowed to last at most (0 means no limit)",
                           "0");
    parser.add_option<int>("bliss_total_time_budget", "time in seconds bliss is "
                           "allowed to run overall (0 means no limit)",
                           "0");
    parser.add_option<bool>("stop_after_no_symmetries", "stop calling bliss "
                            "after unsuccesfull previous bliss call.",
                           "False");
    vector<string> symmetries_for_shrinking;
    symmetries_for_shrinking.push_back("NO_SHRINKING");
    symmetries_for_shrinking.push_back("ATOMIC");
    symmetries_for_shrinking.push_back("LOCAL");
    parser.add_enum_option("symmetries_for_shrinking",
                           symmetries_for_shrinking,
                           "choose the type of symmetries used for shrinking: "
                           "no shrinking, "
                           "only atomic symmetries, "
                           "local symmetries.",
                           "NO_SHRINKING");
    vector<string> symmetries_for_merging;
    symmetries_for_merging.push_back("NO_MERGING");
    symmetries_for_merging.push_back("SMALLEST");
    symmetries_for_merging.push_back("LARGEST");
    parser.add_enum_option("symmetries_for_merging",
                           symmetries_for_merging,
                           "choose the type of symmetries that should determine "
                           "the set of transition systems to be merged: "
                           "the smallest or the largest",
                           "SMALLEST");
    vector<string> external_merging;
    external_merging.push_back("MERGE_FOR_ATOMIC");
    external_merging.push_back("MERGE_FOR_LOCAL");
    parser.add_enum_option("external_merging",
                           external_merging,
                           "choose the set of transition systems to be merged: "
                           "merge for atomic: merge all transition systems affected "
                           "by the chosen symmetry, or "
                           "merge for local: merge only the transition systems "
                           "mapped (in cycles) to others. only merge every "
                           "cycle separately.",
                           "MERGE_FOR_ATOMIC");
    vector<string> internal_merging;
    internal_merging.push_back("LINEAR");
    internal_merging.push_back("NON_LINEAR");
    parser.add_enum_option("internal_merging",
                           internal_merging,
                           "choose the order in which to merge the set of "
                           "transition systems to be merged (only useful with "
                           "MERGE_FOR_ATOMIC): "
                           "linear (obvious), "
                           "non linear, which means to first merge every cycle, "
                           "and then the resulting intermediate transition systems.",
                           "LINEAR");

    // Options for GraphCreator
    parser.add_option<bool>("stabilize_transition_systems", "compute symmetries that "
                            "stabilize transition systems, i.e. that are local.", "false");
    parser.add_option<bool>("debug_graph_creator", "produce dot readable output "
                            "from the graph generating methods", "false");

    // Options for fallback merge strategy
    vector<string> fallback_strategy;
    fallback_strategy.push_back("linear");
    fallback_strategy.push_back("dfp");
    fallback_strategy.push_back("miasm");
    parser.add_enum_option("fallback_strategy",
                           fallback_strategy,
                           "choose a merge strategy: linear (specify "
                           "variable_order), dfp, or miasm.",
                           "dfp");

    // linear
    MergeTreeFactoryLinear::add_options_to_parser(parser);

    // dfp
    MergeScoringFunctionTotalOrder::add_options_to_parser(parser);
    parser.add_option<bool>(
        "randomized_order",
        "If true, use a 'globally' randomized order, i.e. all transition "
        "systems are considered in an arbitrary order. This renders all other "
        "ordering options void.",
        "false");

    // miasm
    MergeTreeFactoryMiasm::add_options_to_parser(parser);

    options::Options options = parser.parse();
    if (options.get<int>("bliss_call_time_limit")
            && options.get<int>("bliss_total_time_budget")) {
        cerr << "Please only specify bliss_call_time_limit or "
                "bliss_total_time_budget but not both" << endl;
        utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
    }
    if (options.get_enum("symmetries_for_shrinking") == 0
            && options.get_enum("symmetries_for_merging") == 0) {
        cerr << "Please use symmetries at least for shrinking or merging." << endl;
        utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
    }
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<MergeStrategyFactorySymmetries>(options);
}

static options::PluginShared<MergeStrategyFactory> _plugin("merge_symmetries", _parse);
}
