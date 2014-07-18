#include "graph_creator.h"

#include "../abstraction.h"

#include "../../globals.h"
#include "../../option_parser.h"
#include "../../timer.h"
#include "../../utilities.h"

#include <cassert>
#include <iostream>

using namespace std;

void add_automorphism(void* param, unsigned int, const unsigned int * automorphism) {
    GraphCreator *gc = (GraphCreator*) param;
    gc->create_symmetry_generator(automorphism);
}

GraphCreator::GraphCreator(const Options &options)
    : debug(options.get<bool>("debug_graph_creator")),
      build_stabilized_pdg(options.get<bool>("build_stabilized_pdg")),
      time_limit(options.get<int>("bliss_time_limit")),
      num_identity_generators(0),
      bliss_limit_reached(false) {
    /*generators_bound(1000), stop_after_false_generated(10000)*/
}

GraphCreator::~GraphCreator() {
    delete_generators();
}

void GraphCreator::delete_generators() {
    for (int i = 0; i < symmetry_generators.size(); i++){
        if (symmetry_generators[i])
            delete symmetry_generators[i];
    }

    symmetry_generators.clear();
    symmetry_generator_info.reset();
    num_identity_generators = 0;
}

void GraphCreator::create_symmetry_generator(const unsigned int *automorphism) {
    SymmetryGenerator* symmetry_generator = new SymmetryGenerator(symmetry_generator_info, automorphism, build_stabilized_pdg);
    //Only if we have non-identity generator we need to save it into the list of generators
//  cout << "Checking for identity" << endl;
    if(!symmetry_generator->identity()) {
        symmetry_generators.push_back(symmetry_generator);
    } else {
        delete symmetry_generator;
        ++num_identity_generators;
//        if (num_identity_generators > stop_after_false_generated) {
//            cout << endl << "Problems with generating symmetry group! Too many false generators." << endl;
//            cout<<"Number of generators: 0"<<endl;
//            exit_with(EXIT_CRITICAL_ERROR);
//        }
    }
}

double GraphCreator::compute_generators(const vector<Abstraction *>& abstractions) {
    // Find (non) abstraction stabilized symmetries for abstractions depending
    // on the chosen option.

    cout << "Computing generators for " << (build_stabilized_pdg? "" : "non ")
         << "abstraction stabilized symmetries" << endl;

    // Deleting previously computed generators
    // TODO: we never need to do this, do we?
    delete_generators();

    Timer timer;
    cout << "Starting initializing symmetries." << endl;

    bliss::Digraph* graph = create_bliss_graph(abstractions);
//    graph->set_splitting_heuristic(bliss::Digraph::shs_flm);
    graph->set_splitting_heuristic(bliss::Digraph::shs_fs);

    graph->set_time_limit(time_limit);
//    graph->set_generators_bound(generators_bound);

    bliss::Stats stats1;
//    FILE *f = fopen("bliss.log", "w");
//    FILE *stats_file = fopen("bliss.stats", "w");
//    graph->set_verbose_file(f);
//    graph->set_verbose_level(10);

    try {
        graph->find_automorphisms(stats1,&(add_automorphism), this);
    } catch (bliss::BlissException &e) {
        e.dump();
        delete_generators();
        delete graph;
        bliss_limit_reached = true;
        return timer();
    }

//    stats1.print(stats_file);
//    fclose(stats_file);
//    fclose(f);

    cout << "Got " << symmetry_generators.size() << " group generators" << endl; //, time step: [t=" << g_timer << "]" << endl;
    cout << "Got " << num_identity_generators << " identity generators" << endl;

    // Deleting the graph
    delete graph;

    cout << "Done initializing symmetries: " << timer << endl;
    return timer();
}

bliss::Digraph* GraphCreator::create_bliss_graph(const vector<Abstraction *>& abstractions) {
    //cout << stabilize_abstractions << endl;

    cout << "Creating the bliss graph object" << endl;
    bliss::Digraph* g = new bliss::Digraph();
    int idx = 0;

    int num_of_nodes = abstractions.size();

    // Setting the number of abstractions
    symmetry_generator_info.num_abstractions = num_of_nodes;
    int node_color_added_val = 0;

    if (debug) {
        cout << "digraph pdg";
        cout << " {" << endl;
        cout << "    node [shape = none] start;" << endl;
    }

    for (int abs_ind = 0; abs_ind < abstractions.size(); abs_ind++){
        // Add vertex for each abstraction
        if (build_stabilized_pdg|| abstractions[abs_ind] == 0) {
            // Either the abstraction is empty or all abstractions are stabilized.
            node_color_added_val++;
            // NOTE: we need to add an abstraction vertex for every abstraction, even the unused ones,
            // because we want to use the abstraction indices as vertex-IDs and vertex IDs in a bliss graph
            // are numbered from 0 to n-1. TODO: get rid of this fact!
            // We further add an extra color for each empty abstraction even when
            // when not stabilizing abstractions in order to ensure that no trivial
            // symmetries that map two empty abstractions to each other are found.
            idx = g->add_vertex(ABSTRACTION_VERTEX + node_color_added_val);
        } else {
          idx = g->add_vertex(ABSTRACTION_VERTEX);
        }
 //     cout << "Adding abstraction vertex: " << idx << endl;
        assert(abs_ind == idx);
        if (debug) {
            cout << "    node" << idx << " [shape=circle, label=abs" << abs_ind << "]; // color: " << node_color_added_val << endl;
        }

        // Setting the indices for connections between abstract states and their abstractions
        symmetry_generator_info.dom_sum_by_var.push_back(num_of_nodes);

        int abs_states = 0;
        if (abstractions[abs_ind]) //In case the abstraction is not empty
            abs_states = abstractions[abs_ind]->size();
        num_of_nodes += abs_states;
        for(int num_of_value = 0; num_of_value < abs_states; num_of_value++){
            symmetry_generator_info.var_by_val.push_back(abs_ind);
        }

    }
    // Setting the total number of abstract states and abstractions
    symmetry_generator_info.num_abs_and_states = num_of_nodes;

    // TODO: Validate that this is what we want here - no connection between actions (probably not!).
    symmetry_generator_info.length = num_of_nodes;

    // We need an arbitrary valid abstraction to get access to the number of
    // labels and their costs (we do not have access to the labels object).
    const Abstraction *some_abs = 0;

    for (int abs_ind = 0; abs_ind < abstractions.size(); abs_ind++){
        if (abstractions[abs_ind] == 0)  //In case the abstraction is empty
            continue;
        if (!some_abs)
            some_abs = abstractions[abs_ind];

        int abs_states = abstractions[abs_ind]->size();

        // Now add abs states for each abstraction
        for(AbstractStateRef state = 0; state < abs_states; state++){

            idx = g->add_vertex(ABS_STATE_VERTEX + node_color_added_val);
//          cout << "Added abstract state vertex: " << idx << " for abstraction " << abs_ind << endl;

            // Edge from abstraction nodes to all the abstraction's states
            g->add_edge(abs_ind, idx);

            if (debug) {
                cout << "    node" << idx << " [shape=circle, label=abs" << abs_ind << "_state" << state << "];" << endl;
                cout << "    node" << abs_ind << " -> node" << idx << ";" << endl;
            }
        }
    }

    // Now we add vertices for operators
    int num_labels = some_abs->get_num_labels();
    // TODO: we probably should skip reduced labels
    for (int label_no = 0; label_no < num_labels; ++label_no){
        if (some_abs->is_label_reduced(label_no))
            continue;
//      int label_op_by_cost = 3 * g_operators[op_no].get_cost();
        // Changed to one node per transition - two colors per operator
        unsigned int label_cost = 2 * some_abs->get_label_cost_by_index(label_no); // was label_op_by_cost

        // For each operator we have one label node
        int label_idx = g->add_vertex(LABEL_VERTEX + label_cost + node_color_added_val);
//      cout << "Added label vertex: " << label_idx << " with color " << LABEL_VERTEX + label_op_by_cost <<" for operator " << op_no << endl;

        if (debug) {
            cout << "    node" << label_idx << " [shape=circle, label=label_no" << label_no /*<< "_" << g_operators[label_no].get_name()*/ << "];" << endl;
        }

        for (int abs_ind = 0; abs_ind < abstractions.size(); abs_ind++){
            if (abstractions[abs_ind] == 0)  //In case the abstraction is empty
                continue;

            const std::vector<AbstractTransition>& transitions = abstractions[abs_ind]->get_transitions_for_label(label_no);
            for (size_t i = 0; i < transitions.size(); ++i) {
                const AbstractTransition &trans = transitions[i];

                // For each abstract transition we have a pair of nodes - pre and eff, both connected to their label node
                int pre_idx = g->add_vertex(LABEL_VERTEX + label_cost + 1 + node_color_added_val);

//              cout << "Added pre vertex: " << pre_idx << " with color " << LABEL_VERTEX + label_op_by_cost + 1 <<" for operator " << op_no << " in abstraction " << abs_ind << endl;
//                int eff_idx = g->add_vertex(LABEL_VERTEX + label_op_by_cost + 2);
                int eff_idx = pre_idx;
//              cout << "Added eff vertex: " << eff_idx << " with color " << LABEL_VERTEX + label_op_by_cost + 2 <<" for operator " << op_no << " in abstraction " << abs_ind << endl;

                unsigned int src_idx = symmetry_generator_info.get_index_by_var_val_pair(abs_ind, trans.src);
                unsigned int target_idx = symmetry_generator_info.get_index_by_var_val_pair(abs_ind, trans.target);
                // Edges from abstract state source over pre=eff=transition-node to target
                g->add_edge(src_idx, pre_idx);
                g->add_edge(eff_idx, target_idx);

//                g->add_edge(pre_idx, eff_idx);

                // Edge from operator-label to transitions (pre=eff=transition-node) induced by that operator
                g->add_edge(label_idx, pre_idx);

//                g->add_edge(label_idx, eff_idx);

                if (debug) {
                    cout << "    node" << pre_idx << " [shape=circle, label=transition];" << endl;
                    cout << "    node" << src_idx << " -> node" << pre_idx << ";" << endl;
                    cout << "    node" << eff_idx << " -> node" << target_idx << ";" << endl;
                    cout << "    node" << label_idx << " -> node" << pre_idx << ";" << endl;
                }
            }
        }
    }

    // Finally, adding the goal node
    idx = g->add_vertex(GOAL_VERTEX + node_color_added_val);

    if (debug) {
        cout << "    node [shape = doublecircle] node" << idx << " [label = goal];" << endl;
    }

//  cout << "Added goal vertex: " << idx << " with color " << GOAL_VERTEX << endl;

    for (int abs_ind = 0; abs_ind < abstractions.size(); abs_ind++){
        if (abstractions[abs_ind] == 0)  //In case the abstraction is empty
            continue;

        int abs_states = abstractions[abs_ind]->size();

        for(AbstractStateRef state = 0; state < abs_states; state++){
            if (!abstractions[abs_ind]->is_goal_state(state))
                continue;

            unsigned int val_idx = symmetry_generator_info.get_index_by_var_val_pair(abs_ind, state);

            // Edges from goal states to the goal node
            g->add_edge(val_idx, idx);

            if (debug) {
                cout << "    node" << val_idx << " -> node" << idx << ";" << endl;
            }
        }
    }

    if (debug) {
        cout << "}" << endl;
    }

    return g;
}
