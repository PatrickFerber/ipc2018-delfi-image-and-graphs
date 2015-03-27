#ifndef CEGAR_UTILS_H
#define CEGAR_UTILS_H

#include "../additive_heuristic.h"
#include "../causal_graph.h"
#include "../global_operator.h"
#include "../global_state.h"
#include "../landmarks/landmark_graph.h"

#include <boost/dynamic_bitset.hpp>

#include <limits>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace cegar {
extern bool DEBUG;

const int UNDEFINED = -1;
// The name INFINITY is reserved in C++11.
const int INF = std::numeric_limits<int>::max();

// See additive_heuristic.h.
const int MAX_COST_VALUE = 100000000;

typedef boost::dynamic_bitset<> Bitset;
typedef std::pair<int, std::vector<int> > Split;
typedef std::vector<Split> Splits;
typedef std::pair<int, int> Fact;
typedef std::unordered_map<int, std::unordered_set<int> > VariableToValues;

bool is_not_marked(GlobalOperator &op);

int get_pre(const GlobalOperator &op, int var);
int get_eff(const GlobalOperator &op, int var);
int get_post(const GlobalOperator &op, int var);

void get_unmet_preconditions(const GlobalOperator &op, const GlobalState &state, Splits *splits);

void get_unmet_goals(GoalsProxy goals, const GlobalState &state, Splits *splits);

struct hash_state_id {
    size_t operator()(const StateID &id) const {
        return id.hash();
    }
};

struct hash_fact {
    size_t operator()(const Fact &fact) const {
        return fact.first * 10000 + fact.second;
    }
};

Fact get_fact(const LandmarkNode *node);

void write_landmark_graph(const LandmarkGraph &graph);
void write_causal_graph();

void reserve_memory_padding();
void release_memory_padding();
bool memory_padding_is_reserved();

// Debugging.
std::string to_string(int i);
std::string to_string(Fact fact);
std::string to_string(const std::vector<int> &v);
std::string to_string(const std::set<int> &s);
std::string to_string(const std::unordered_set<int> &s);

std::ostream &operator<<(std::ostream &os, const Fact &fact);
}

#endif
