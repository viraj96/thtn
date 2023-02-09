#include <cassert>
#include <iostream>

#include "cwa.hpp"
#include "domain.hpp"

general_formula *goal_formula = nullptr;
vector<ground_literal> init = vector<ground_literal>();
vector<ground_literal> goal = vector<ground_literal>();
vector<pair<ground_literal, int> > init_functions =
    vector<pair<ground_literal, int> >();

string ground_literal::to_string() const {
    string result = "Ground Literal: ";

    result += "\n\t Positive = ";
    if (positive)
        result += "true\n";
    else
        result += "false\n";

    result += "\t Timed = ";
    if (timed == START)
        result += "start\n";
    else if (timed == END)
        result += "end\n";
    else
        result += "all\n";

    result += "\t Temporal Qualifier = ";
    if (temp_qual == AT)
        result += "at\n";
    else
        result += "over\n";

    result += "\t Predicate = " + predicate + "\n";

    result += "\t Arguments = \n";
    for (string arg : args) result += "\t\t " + arg + "\n";

    return result;
}

bool operator<(const ground_literal &lhs, const ground_literal &rhs) {
    if (lhs.predicate < rhs.predicate) return true;
    if (lhs.predicate > rhs.predicate) return false;

    if (lhs.positive < rhs.positive) return true;
    if (lhs.positive > rhs.positive) return false;

    if (lhs.args < rhs.args) return true;
    if (lhs.args > rhs.args) return false;

    return false;
}

void flatten_goal() {
    if (goal_formula == nullptr) return;

    vector<pair<
        pair<vector<variant<literal, conditional_effect> >, vector<literal> >,
        additional_variables> >
        ex = goal_formula->expand(false);

    assert(ex.size() == 1);
    assert(ex[0].first.second.size() == 0);

    map<string, string> access = map<string, string>();
    for (auto x : ex[0].second) {
        string sort = x.second;
        // Must be an actual constant
        assert(sorts[sort].size() == 1);
        access[x.first] = *sorts[sort].begin();
    }

    for (variant<literal, conditional_effect> l : ex[0].first.first) {
        // Goal may not contain conditional effects
        if (holds_alternative<conditional_effect>(l)) assert(false);

        ground_literal gl = ground_literal();
        gl.positive = get<literal>(l).positive;
        gl.predicate = get<literal>(l).predicate;
        for (string v : get<literal>(l).arguments) gl.args.push_back(access[v]);

        goal.push_back(gl);
    }
}

vector<string> int_const = vector<string>();
map<string, int> const_int = map<string, int>();
map<string, vector<int> > int_sorts = map<string, vector<int> >();

// Fully instantiate predicate via recursion
void fully_instantiate(int *inst, unsigned int pos, vector<string> &arg_sorts,
                       vector<vector<int> > &result) {
    if (pos == arg_sorts.size()) {
        vector<int> instance(inst, inst + arg_sorts.size());
        result.push_back(instance);
        return;
    }

    for (int c : int_sorts[arg_sorts[pos]]) {
        inst[pos] = c;
        fully_instantiate(inst, pos + 1, arg_sorts, result);
    }
}

void compute_cwa() {
    // Transform sort representation to ints
    for (pair<string, set<string> > s : sorts) {
        for (string c : s.second) {
            int id = 0;
            auto it = const_int.find(c);

            if (it == const_int.end()) {
                id = const_int.size();
                const_int[c] = id;
                int_const.push_back(c);

            } else
                id = it->second;

            int_sorts[s.first].push_back(id);
        }
    }

    // Find predicates occurring negatively in preconditions and their types
    map<string, set<vector<string> > > neg_predicates_with_arg_sorts =
        map<string, set<vector<string> > >();

    for (task t : primitive_tasks) {
        vector<literal> literals = t.prec;
        for (conditional_effect ceff : t.ceff)
            for (literal l : ceff.condition) literals.push_back(l);

        for (literal l : literals)
            if (!l.positive) {
                vector<string> argSorts = vector<string>();
                for (string v : l.arguments)
                    for (arg_and_type x : t.vars) {
                        if (x.first == v)
                            argSorts.push_back(x.second);
                        else if (x.first == v.substr(0, v.find("."))) {
                            string ctype = v;
                            v.erase(0, v.find(".") + 1);
                            ctype = v;
                            bool found = false;
                            for (sort_definition s : sort_definitions) {
                                for (string d : s.declared_sorts) {
                                    if (d == x.second) {
                                        for (arg_and_type vt : s.vars.vars) {
                                            string type = vt.first;
                                            if (type.substr(1) == ctype) {
                                                argSorts.push_back(vt.second);
                                                found = true;
                                                break;
                                            }
                                        }
                                    }

                                    if (found) break;
                                }

                                if (found) break;
                            }
                        }
                    }

                assert(argSorts.size() == l.arguments.size());

                if (!l.functional)
                    neg_predicates_with_arg_sorts[l.predicate].insert(argSorts);
            }
    }

    // Negative predicates in goal
    for (ground_literal l : goal)
        if (!l.positive) {
            vector<string> args = vector<string>();
            for (string c : l.args) args.push_back(sort_for_const(c));
            neg_predicates_with_arg_sorts[l.predicate].insert(args);
        }

    map<string, set<vector<int> > > init_check =
        map<string, set<vector<int> > >();
    for (ground_literal l : init) {
        vector<int> args = vector<int>();
        for (string &arg : l.args) args.push_back(const_int[arg]);
        init_check[l.predicate].insert(args);
    }

    for (pair<string, set<vector<string> > > np :
         neg_predicates_with_arg_sorts) {
        set<vector<int> > instantiations = set<vector<int> >();
        for (vector<string> arg_sorts : np.second) {
            vector<vector<int> > inst = vector<vector<int> >();
            int *partial_instance = new int[arg_sorts.size()];
            fully_instantiate(partial_instance, 0, arg_sorts, inst);
            delete[] partial_instance;

            for (vector<int> &p : inst)
                if (instantiations.count(p) == 0) {
                    instantiations.insert(p);
                    if (init_check[np.first].count(p)) continue;

                    ground_literal lit = ground_literal();
                    lit.predicate = np.first;
                    for (int arg : p) lit.args.push_back(int_const[arg]);
                    lit.positive = false;
                    init.push_back(lit);
                }
        }
    }
}
