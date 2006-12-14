#ifndef OPERATOR_H
#define OPERATOR_H

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "globals.h"
#include "state.h"
#include "partially_relaxed_state.h"

class Variable;

struct Prevail {
    int var;
    int prev;
    Prevail(std::istream &in);
    Prevail(int v, int p) : var(v), prev(p) {}

    bool is_applicable(const State &state) const {
	assert(var >= 0 && var < g_variable_name.size());
	assert(prev >= 0 && prev < g_variable_domain[var]);
	return state[var] == prev;
    }
    bool is_applicable(const PartiallyRelaxedState &state) const {
	assert(var >= 0 && var < g_variable_name.size());
	assert(prev >= 0 && prev < g_variable_domain[var]);
	return state.has_value(var, prev);
    }

    void dump() const;
};

struct PrePost {
    int var;
    int pre, post;
    std::vector<Prevail> cond;
    PrePost() {} // Needed for axiom file-reading constructor, unfortunately.
    PrePost(std::istream &in);
    PrePost(int v, int pr, int po, const std::vector<Prevail> &co)
	: var(v), pre(pr), post(po), cond(co) {}

    bool is_applicable(const State &state) const {
	assert(var >= 0 && var < g_variable_name.size());
	assert(pre == -1 || pre >= 0 && pre < g_variable_domain[var]);
	return pre == -1 || state[var] == pre;
    }
    bool is_applicable(const PartiallyRelaxedState &state) const {
	assert(var >= 0 && var < g_variable_name.size());
	assert(pre == -1 || pre >= 0 && pre < g_variable_domain[var]);
	return pre == -1 || state.has_value(var, pre);
    }

    bool does_fire(const State &state) const {
	for(int i = 0; i < cond.size(); i++)
	    if(!cond[i].is_applicable(state))
		return false;
	return true;
    }
    bool does_fire(const PartiallyRelaxedState &state) const {
	for(int i = 0; i < cond.size(); i++)
	    if(!cond[i].is_applicable(state))
		return false;
	return true;
    }

    void dump() const;
};

class Operator {
    bool is_an_axiom;
    std::vector<Prevail> prevail;      // var, val
    std::vector<PrePost> pre_post;     // var, old-val, new-val, effect conditions
    std::string name;
public:
    Operator(std::istream &in, bool is_axiom);
    void dump() const;
    std::string get_name() const {return name;}

    bool is_axiom() const {return is_an_axiom;}

    const std::vector<Prevail> &get_prevail() const {return prevail;}
    const std::vector<PrePost> &get_pre_post() const {return pre_post;}

    bool is_applicable(const State &state) const {
	for(int i = 0; i < prevail.size(); i++)
	    if(!prevail[i].is_applicable(state))
		return false;
	for(int i = 0; i < pre_post.size(); i++)
	    if(!pre_post[i].is_applicable(state))
		return false;
	return true;
    }
    bool is_applicable(const PartiallyRelaxedState &state) const {
	for(int i = 0; i < prevail.size(); i++)
	    if(!prevail[i].is_applicable(state))
		return false;
	for(int i = 0; i < pre_post.size(); i++)
	    if(!pre_post[i].is_applicable(state))
		return false;
	return true;
    }
};

#endif
