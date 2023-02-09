#include <map>
#include <cstdio>
#include <vector>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <unordered_set>

#include "cwa.hpp"
#include "domain.hpp"
#include "parsetree.hpp"
#include "properties.hpp"
#include "hddlWriter.hpp"

#include "hddl_header.hpp"

using namespace std;

bool replacement_type_dfs(int cur, int end, set<int> &visited,
                          vector<set<int>> &parents) {
    if (cur == end) return true;
    if (!parents[cur].size()) return false;
    if (visited.count(cur)) return true;

    visited.insert(cur);

    for (int p : parents[cur])
        if (!replacement_type_dfs(p, end, visited, parents)) return false;

    return true;
}

pair<int, set<int>> get_replacement_type(int type_to_replace,
                                         vector<set<int>> &parents) {
    // Try all possible replacement sorts
    set<int> best_visited = set<int>();
    int best_replacement = -1;
    for (size_t s = 0; s < parents.size(); s++) {
        if (int(s) != type_to_replace) {
            set<int> visited = set<int>();
            if (replacement_type_dfs(type_to_replace, s, visited, parents)) {
                if (best_replacement == -1 ||
                    best_visited.size() > visited.size()) {
                    best_replacement = s;
                    best_visited = visited;
                }
            }
        }
    }

    return make_pair(best_replacement, best_visited);
}

tuple<vector<string>, vector<int>, map<string, string>, map<int, int>>
compute_local_type_hierarchy() {
    // Find subset relations between sorts
    // [i][j] = true means that j is a subset of i
    vector<string> sortsInOrder = vector<string>();
    for (auto &[s, _] : sorts) {
        (void)_;
        sortsInOrder.push_back(s);
    }

    vector<vector<bool>> subset(sortsInOrder.size());

    for (size_t s1I = 0; s1I < sortsInOrder.size(); s1I++) {
        string s1 = sortsInOrder[s1I];

        if (!sorts[s1].size()) {
            for (size_t s2 = 0; s2 < sortsInOrder.size(); s2++)
                subset[s1I].push_back(false);
            continue;
        }

        for (size_t s2I = 0; s2I < sortsInOrder.size(); s2I++) {
            string s2 = sortsInOrder[s2I];
            if (s1I != s2I && sorts[s2].size()) {
                if (includes(sorts[s1].begin(), sorts[s1].end(),
                             sorts[s2].begin(), sorts[s2].end())) {
                    // Here we know that s2 is a subset of s1
                    subset[s1I].push_back(true);

                } else
                    subset[s1I].push_back(false);

            } else
                subset[s1I].push_back(false);
        }
    }

    // Transitive reduction
    for (size_t s1 = 0; s1 < sortsInOrder.size(); s1++)
        for (size_t s2 = 0; s2 < sortsInOrder.size(); s2++)
            for (size_t s3 = 0; s3 < sortsInOrder.size(); s3++)
                if (subset[s2][s1] && subset[s1][s3]) subset[s2][s3] = false;

    // Determine parents
    vector<set<int>> parents(sortsInOrder.size());
    vector<int> parent(sortsInOrder.size());
    for (size_t s1 = 0; s1 < sortsInOrder.size(); s1++) parent[s1] = -1;

    for (size_t s1 = 0; s1 < sortsInOrder.size(); s1++)
        for (size_t s2 = 0; s2 < sortsInOrder.size(); s2++)
            if (subset[s1][s2]) {
                parents[s2].insert(s1);
                if (parent[s2] != -1) {
                    if (parent[s2] >= 0) {
                        cout << "Type hierarchy is not a tree. I can't write \
                            this in standard conform HDDL."
                             << endl;
                        cout << "\tThe sort " << s2 << " " << sortsInOrder[s2]
                             << " has multiple parents." << endl;
                        for (int ss : parents[s2])
                            cout << "\t\t" << sortsInOrder[ss] << endl;
                    }

                    parent[s2] = -2;

                } else
                    parent[s2] = s1;
            }

    // Sorts with multiple parents need to be replaced by parent sorts
    map<int, int> replacedSorts = map<int, int>();

    for (size_t s = 0; s < sortsInOrder.size(); s++) {
        if (parent[s] == -2) {
            auto [replacement, all_covered] = get_replacement_type(s, parents);
            // There is always the object type
            assert(replacement != -1);

            for (int covered : all_covered) {
                replacedSorts[covered] = replacement;
                parent[covered] = -2;
            }
        }
    }

    // Update parent relation
    for (size_t s = 0; s < sortsInOrder.size(); s++)
        if (parent[s] >= 0)
            if (replacedSorts.count(parent[s]))
                parent[s] = replacedSorts[parent[s]];

    // Who is whose direct subset, with handling the replaced sorts
    vector<unordered_set<int>> directsubset(sortsInOrder.size());
    for (size_t s1 = 0; s1 < sortsInOrder.size(); s1++)
        for (size_t s2 = 0; s2 < sortsInOrder.size(); s2++)
            if (parent[s2] >= 0)
                if (subset[s1][s2]) {
                    if (replacedSorts.count(s1))
                        directsubset[replacedSorts[s1]].insert(s2);
                    else
                        directsubset[s1].insert(s2);
                }

    // Assign elements to sorts
    vector<set<string>> directElements(sortsInOrder.size());

    for (size_t s1 = 0; s1 < sortsInOrder.size(); s1++) {
        if (parent[s1] != -2) {
            for (string elem : sorts[sortsInOrder[s1]]) {
                bool in_sub_sort = false;
                for (int s2 : directsubset[s1]) {
                    if (sorts[sortsInOrder[s2]].count(elem)) {
                        in_sub_sort = true;
                        break;
                    }
                }

                if (in_sub_sort) continue;
                directElements[s1].insert(elem);
            }
        }
    }

    map<string, string> sortOfElement = map<string, string>();

    for (size_t s1 = 0; s1 < sortsInOrder.size(); s1++)
        for (string elem : directElements[s1])
            sortOfElement[elem] = sortsInOrder[s1];

    return make_tuple(sortsInOrder, parent, sortOfElement, replacedSorts);
}

void print_indent(ostream &out, int indent, bool end) {
    if (indent == -1) {
        if (end) out << "\t\t";
        return;
    }

    out << "\t\t";
    for (int i = 0; i < indent + 1; i++) out << "\t";
}

void print_var_and_const(ostream &out, var_and_const &vars) {
    map<string, string> constants = map<string, string>();
    for (auto [v, s] : vars.newVar) constants[v] = *sorts[s].begin();
    for (string v : vars.vars) {
        if (constants.count(v)) v = constants[v];
        out << " " << v;
    }
}

void print_formula(ostream &out, general_formula *f, int indent) {
    if (f == 0) return;
    if (f->type == EMPTY) return;
    if (f->type == AND || f->type == OR) {
        print_indent(out, indent);
        out << "(";
        if (f->type == AND)
            out << "and";
        else
            out << "or";
        out << endl;
        for (general_formula *s : f->subformulae)
            print_formula(out, s, indent + 1);

        print_indent(out, indent, true);
        out << ")" << endl;
    }

    if (f->type == FORALL || f->type == EXISTS) {
        print_indent(out, indent);
        out << "(";
        if (f->type == FORALL)
            out << "forall";
        else
            out << "exists";
        out << " (";
        bool first = true;
        for (auto [v, s] : f->qvariables.vars) {
            if (!first) out << " ";
            out << v << " - " << s;
            first = false;
        }
        out << ")" << endl;
        for (general_formula *s : f->subformulae)
            print_formula(out, s, indent + 1);

        print_indent(out, indent, true);
        out << ")" << endl;
    }

    if (f->type == ATOM || f->type == NOTATOM) {
        print_indent(out, indent);
        out << "(";
        if (f->temp_qual == AT)
            out << "at ";
        else if (f->temp_qual == OVER)
            out << "over ";

        if (f->timed == START)
            out << "start ";
        else if (f->timed == END)
            out << "end ";
        else if (f->timed == ALL)
            out << "all ";

        if (f->type == NOTATOM) out << "(not ";
        out << "(" << f->predicate;
        print_var_and_const(out, f->arguments);
        if (f->type == NOTATOM) out << ")";
        out << ")"
            << ")" << endl;
    }

    if (f->type == WHEN) {
        print_indent(out, indent);
        out << "(when" << endl;
        print_formula(out, f->subformulae[0], indent + 1);
        print_formula(out, f->subformulae[1], indent + 1);
        print_indent(out, indent);
        out << ")" << endl;
    }

    if (f->type == EQUAL || f->type == NOTEQUAL) {
        print_indent(out, indent);
        out << "(";
        if (f->temp_qual == AT)
            out << "at ";
        else if (f->temp_qual == OVER)
            out << "over ";

        if (f->timed == START)
            out << "start ";
        else if (f->timed == END)
            out << "end ";
        else if (f->timed == ALL)
            out << "all ";

        if (f->type == NOTEQUAL) out << "(not ";
        out << "(= " << f->arg1 << " " << f->arg2;
        if (f->type == NOTEQUAL) out << ")";
        out << ")"
            << ")" << endl;
    }
}

void print_formula_for(ostream &out, general_formula *f, string topic) {
    general_formula *pf = f;
    if (pf->type != AND) {
        pf = new general_formula();
        pf->type = AND;
        pf->subformulae.push_back(f);
    }
    out << topic << " ";
    print_formula(out, pf, -1);
}

void hddl_output(ostream &dout, ostream &pout, bool problem) {
    auto sanitise = [&](string s) {
        string result = "";
        if (s == "__equal") {
            result = "=";
            return result;
        }
        for (size_t i = 0; i < s.size(); i++) {
            char c = s[i];

            if (c == '_' && !i) result += "US";

            if (c == '<')
                result += "LA_";
            else if (c == '>')
                result += "RA_";
            else if (c == '[')
                result += "LB_";
            else if (c == ']')
                result += "RB_";
            else if (c == '|')
                result += "BAR_";
            else if (c == ';')
                result += "SEM_";
            else if (c == ',')
                result += "COM_";
            else if (c == '+')
                result += "PLUS_";
            else if (c == '-')
                result += "MINUS_";
            else
                result += c;
        }

        return result;
    };

    set<string> neg_pred = set<string>();
    for (task t : primitive_tasks)
        for (literal l : t.prec)
            if (!l.positive) neg_pred.insert(l.predicate);

    for (task t : primitive_tasks)
        for (conditional_effect ceff : t.ceff)
            for (literal l : ceff.condition)
                if (!l.positive) neg_pred.insert(l.predicate);

    for (ground_literal l : goal)
        if (!l.positive) neg_pred.insert(l.predicate);

    if (!problem) {
        // TODO: Do this more intelligently
        dout << "(define (domain " << domain_name << ")" << endl;
        dout << "\t(:requirements :typing :equality :hierarchy";
        dout << " :method-preconditions :negative-preconditions";
        dout << ")" << endl;
        dout << endl;

        // TODO: Identical types are not recognised and are treated as a non-dag
        // structure, which is not necessary
        dout << "\t(:types" << endl;
    }

    set<string> declaredSorts = set<string>();
    vector<string> sortsInOrder = vector<string>();
    map<int, int> replacedSorts = map<int, int>();
    map<string, string> sortOfElement = map<string, string>();

    for (sort_definition s : sort_definitions) {
        bool first = true;
        for (string ss : s.declared_sorts) {
            if (!problem) {
                if (s.has_parent_sort) {
                    if (first) dout << "\t\t";
                    first = false;
                    dout << " " << ss;

                } else
                    dout << "\t\t" << ss << endl;
            }

            declaredSorts.insert(ss);
        }

        if (!problem) {
            if (s.vars.vars.size()) {
                dout << " { ";
            }
            for (arg_and_type p : s.vars.vars) {
                dout << p.first << " - " << p.second << " ";
            }
            if (s.vars.vars.size()) {
                dout << "}";
            }
            if (s.has_parent_sort) {
                dout << " - " << s.parent_sort,
                    declaredSorts.insert(s.parent_sort);
                dout << endl;
            }
        }
    }

    if (!problem) {
        dout << "\t)" << endl;
        dout << endl;
    }

    if (problem) {
        pout << "(define ";
        pout << "(problem " << problem_name << ")" << endl;
        pout << "\t(:domain " << domain_name << ")" << endl;
    }

    // Determine which constants need to be declared in the domain
    set<string> constants_in_domain = compute_constants_in_domain();

    if (!problem)
        if (constants.size()) {
            dout << "\t(:constants" << endl;
            for (pair<string, set<string>> m : constants) {
                dout << "\t\t";
                for (string s : m.second) dout << s << " ";
                dout << "- " << m.first << endl;
            }
            dout << "\t)\n\n";
        }

    if (problem) {
        // Objects
        pout << "\t(:objects" << endl;

        for (pair<string, set<string>> x : sorts) {
            if (!declaredSorts.count(x.first)) continue;
            pout << "\t\t";
            for (string s : x.second)
                if (!constants_in_domain.count(s)) pout << sanitise(s) << " ";
            pout << "- " << sanitise(x.first) << endl;
        }

        pout << "\t)" << endl;

        // Complex objects
        pout << "\t(:object-instants" << endl;

        for (pair<string, set<map<string, var_declaration>>> c : csorts) {
            for (map<string, var_declaration> s : c.second) {
                for (pair<string, var_declaration> m : s) {
                    unsigned int ct = 0;
                    pout << "\t\t";
                    pout << m.first << " { ";
                    for (arg_and_type v : m.second.vars) {
                        pout << v.first << " = " << v.second;
                        if (ct < m.second.vars.size() - 1) pout << ", ";
                        ct++;
                    }

                    pout << " }\n";
                }
            }
        }

        pout << "\t)" << endl;
    }

    // Predicate definitions
    if (!problem) dout << "\t(:predicates" << endl;

    map<string, string> sortReplace = map<string, string>();
    map<string, string> typeConstraint = map<string, string>();
    for (auto &[s, _] : sorts) {
        (void)_;
        sortReplace[s] = s;
    }

    for (auto [originalSort, replacement] : replacedSorts) {
        string oldSort = sortsInOrder[originalSort];
        string newSort = sortsInOrder[replacement];
        sortReplace[oldSort] = newSort;
        string predicateName = "p_sort_member_" + sanitise(oldSort);
        typeConstraint[oldSort] = predicateName;
        if (!problem)
            dout << "    (" << predicateName << " ?x - " << sanitise(newSort)
                 << ")" << endl;
    }

    if (!problem) {
        for (predicate_definition p : predicate_definitions) {
            if (!p.functional && p.name != dummy_equal_literal) {
                for (int negative = 0; negative < 2; negative++) {
                    // Compile only for internal output
                    if (negative) continue;
                    if (negative && !neg_pred.count(p.name)) continue;
                    dout << "\t\t(";
                    if (negative) dout << "not_";
                    dout << sanitise(p.name);

                    // Arguments
                    for (size_t arg = 0; arg < p.argument_sorts.size(); arg++)
                        dout << " ?var" << arg << " - "
                             << sanitise(sortReplace[p.argument_sorts[arg]]);

                    dout << ")" << endl;
                }
            }
        }

        dout << "\t)" << endl;
        dout << endl;

        // Functional predicate definitions
        dout << "\t(:f-predicates" << endl;

        for (predicate_definition p : predicate_definitions) {
            if (p.functional) {
                for (int negative = 0; negative < 2; negative++) {
                    // Compile only for internal output
                    if (negative) continue;
                    if (negative && !neg_pred.count(p.name)) continue;
                    dout << "\t\t(";
                    if (negative) dout << "not_";
                    dout << sanitise(p.name);

                    // Arguments
                    for (size_t arg = 0; arg < p.argument_sorts.size(); arg++)
                        dout << " ?var" << arg << " - "
                             << sanitise(sortReplace[p.argument_sorts[arg]]);

                    dout << ")" << endl;
                }
            }
        }

        dout << "\t)" << endl;
        dout << endl;
    }

    bool hasActionCosts = metric_target != dummy_function_type;

    if (!problem) {
        // Functions (for cost expressions)
        if (parsed_functions.size() && hasActionCosts) {
            dout << "\t(:functions" << endl;
            for (pair<predicate_definition, string> f : parsed_functions) {
                dout << "\t\t(" << sanitise(f.first.name);
                for (size_t arg = 0; arg < f.first.argument_sorts.size(); arg++)
                    dout << " ?var" << arg << " - "
                         << sanitise(f.first.argument_sorts[arg]);
                dout << ") - " << f.second << endl;
            }

            dout << "\t)" << endl;
            dout << endl;
        }

        // Abstract tasks
        for (task t : abstract_tasks) {
            if (!t.artificial) {
                if (t.name == "__top") continue;

                dout << "\t(:task " << sanitise(t.name) << endl;
                dout << "\t\t:parameters (";
                bool first = true;
                for (arg_and_type v : t.vars) {
                    if (!first) dout << " ";
                    first = false;
                    dout << sanitise(v.first) << " - "
                         << sanitise(sortReplace[v.second]);
                }

                dout << ")" << endl << "\t)" << endl;
                dout << endl;
            }
        }

        // Decomposition methods
        for (auto [atname, ms] : parsed_methods)
            for (parsed_method m : ms) {
                if (m.name == "__top_method") continue;
                if (m.functional) continue;
                dout << "\t(:method ";
                dout << sanitise(m.name) << endl;
                dout << "\t\t:parameters (";
                bool first = true;
                for (auto [v, s] : m.vars->vars) {
                    if (!first) dout << " ";
                    first = false;
                    dout << sanitise(v) << " - " << sanitise(sortReplace[s]);
                }

                dout << ")" << endl;

                // AT
                dout << "\t\t:task (";
                dout << sanitise(atname);
                map<string, string> atConstants;
                for (auto [v, s] : m.newVarForAT)
                    atConstants[v] = *sorts[s].begin();

                for (string v : m.atArguments) {
                    if (atConstants.count(v)) v = atConstants[v];
                    dout << " " << sanitise(v);
                }

                dout << ")" << endl;

                if (!m.prec->isEmpty())
                    print_formula_for(dout, m.prec, "\t\t:precondition");

                if (!m.eff->isEmpty())
                    print_formula_for(dout, m.eff, "\t\t:effect");

                // Subtasks
                if (!m.functional) {
                    vector<string> liftedTopSort = liftedPropertyTopSort(m.tn);
                    if (isTopSortTotalOrder(liftedTopSort, m.tn)) {
                        dout << "\t\t:ordered-subtasks (and" << endl;
                        map<string, sub_task *> idMap =
                            map<string, sub_task *>();
                        for (sub_task *t : m.tn->tasks) idMap[t->id] = t;
                        for (string id : liftedTopSort) {
                            dout << "\t\t\t(" << id << " ("
                                 << sanitise(idMap[id]->task);
                            print_var_and_const(dout, *idMap[id]->arguments);
                            dout << "))" << endl;
                        }

                        dout << "\t\t)" << endl;

                    } else {
                        dout << "\t\t:subtasks (and" << endl;
                        for (sub_task *task : m.tn->tasks) {
                            dout << "\t\t\t(" << task->id << " ("
                                 << sanitise(task->task);
                            print_var_and_const(dout, *task->arguments);
                            dout << "))" << endl;
                        }
                        dout << "\t\t)" << endl;
                        if (m.tn->ordering.size()) {
                            // Ordering of subtasks
                            dout << "\t\t:ordering (and" << endl;
                            for (arg_and_type *p : m.tn->ordering)
                                dout << "\t\t\t(< " << p->first << " "
                                     << p->second << ")" << endl;
                            dout << "\t\t)" << endl;
                        }
                    }

                    if (m.tn->synchronize_constraints.size()) {
                        dout << "\t\t:sync-constraints (and" << endl;
                        for (general_formula *sc :
                             m.tn->synchronize_constraints) {
                            dout << "\t\t\t(" << sc->task1;
                            if (sc->lb == sc->ub && sc->lb == 0 &&
                                !sc->contiguous)
                                dout << " meets " << sc->task2 << ")" << endl;
                            else if (sc->lb == sc->ub && sc->lb == 0 &&
                                     sc->contiguous)
                                dout << " contiguous " << sc->task2 << ")"
                                     << endl;
                            else {
                                dout << " before {" << sc->lb << " ";
                                if (sc->ub == inf_int)
                                    dout << "+inf";
                                else if (sc->ub == -inf_int)
                                    dout << "-inf";
                                else
                                    dout << sc->ub;
                                dout << "} " << sc->task2 << ")" << endl;
                            }
                        }

                        dout << "\t\t)" << endl;
                    }

                    if (!m.tn->constraint->isEmpty())
                        print_formula_for(dout, m.tn->constraint,
                                          ":constraints");
                }

                dout << "\t)" << endl << endl;
            }

        // Functional decomposition methods
        for (auto [atname, ms] : parsed_methods)
            for (parsed_method m : ms) {
                if (m.name == "__top_method") continue;
                if (!m.functional) continue;
                dout << "\t(:f-method ";
                dout << sanitise(m.name) << endl;
                dout << "\t\t:parameters (";
                bool first = true;
                for (auto [v, s] : m.vars->vars) {
                    if (!first) dout << " ";
                    first = false;
                    dout << sanitise(v) << " - " << sanitise(sortReplace[s]);
                }

                dout << ")" << endl;

                // AT
                dout << "\t\t:task (";
                dout << sanitise(atname);
                map<string, string> atConstants = map<string, string>();
                for (auto [v, s] : m.newVarForAT)
                    atConstants[v] = *sorts[s].begin();

                for (string v : m.atArguments) {
                    if (atConstants.count(v)) v = atConstants[v];
                    dout << " " << sanitise(v);
                }

                dout << ")" << endl;

                if (!m.prec->isEmpty())
                    print_formula_for(dout, m.prec, "\t\t:precondition");

                if (!m.eff->isEmpty())
                    print_formula_for(dout, m.eff, "\t\t:effect");

                // Subtasks
                if (!m.functional) {
                    vector<string> liftedTopSort = liftedPropertyTopSort(m.tn);
                    if (isTopSortTotalOrder(liftedTopSort, m.tn)) {
                        dout << "\t\t:ordered-subtasks (and" << endl;
                        map<string, sub_task *> idMap =
                            map<string, sub_task *>();
                        for (sub_task *t : m.tn->tasks) idMap[t->id] = t;
                        for (string id : liftedTopSort) {
                            dout << "\t\t\t(" << id << " ("
                                 << sanitise(idMap[id]->task);
                            print_var_and_const(dout, *idMap[id]->arguments);
                            dout << "))" << endl;
                        }

                        dout << "\t\t)" << endl;

                    } else {
                        dout << "\t\t:subtasks (and" << endl;
                        for (sub_task *task : m.tn->tasks) {
                            dout << "\t\t\t(" << task->id << " ("
                                 << sanitise(task->task);
                            print_var_and_const(dout, *task->arguments);
                            dout << "))" << endl;
                        }
                        dout << "\t\t)" << endl;
                        if (m.tn->ordering.size()) {
                            // Ordering of subtasks
                            dout << "\t\t:ordering (and" << endl;
                            for (arg_and_type *p : m.tn->ordering)
                                dout << "\t\t\t(< " << p->first << " "
                                     << p->second << ")" << endl;
                            dout << "\t\t)" << endl;
                        }
                    }

                    if (!m.tn->constraint->isEmpty())
                        print_formula_for(dout, m.tn->constraint,
                                          ":constraints");
                }

                dout << "\t)" << endl << endl;
            }

        // Actions
        for (task t : primitive_tasks) {
            if (t.artificial) continue;
            dout << "\t(:action " << sanitise(t.name) << endl;
            dout << "\t\t:parameters (";
            bool first = true;
            vector<arg_and_type> variablesToConstrain = vector<arg_and_type>();
            for (arg_and_type v : t.vars) {
                if (!first) dout << " ";
                first = false;
                dout << sanitise(v.first) << " - "
                     << sanitise(sortReplace[v.second]);
                if (typeConstraint.count(v.second))
                    variablesToConstrain.push_back(
                        make_pair(v.first, typeConstraint[v.second]));
            }

            dout << ")" << endl;

            dout << "\t\t:duration (= ?duration " << t.duration << ")" << endl;

            if (variablesToConstrain.size() || t.prec.size() ||
                t.constraints.size()) {
                // Precondition
                dout << "\t\t:precondition (and" << endl;

                for (auto &[v, pred] : variablesToConstrain) {
                    dout << "\t\t\t(" << sanitise(pred) << " " << sanitise(v)
                         << ")" << endl;
                }

                for (literal l : t.constraints) {
                    dout << "\t\t\t";
                    if (!l.positive) dout << "(not ";
                    dout << "(= " << sanitise(l.arguments[0]) << " "
                         << sanitise(l.arguments[1]) << ")";
                    if (!l.positive) dout << ")";
                    dout << endl;
                }

                for (literal l : t.prec) {
                    dout << "\t\t\t(";

                    if (l.temp_qual == AT)
                        dout << "at ";
                    else if (l.temp_qual == OVER)
                        dout << "over ";

                    if (l.timed == START)
                        dout << "start ";
                    else if (l.timed == END)
                        dout << "end ";
                    else if (l.timed == ALL)
                        dout << "all ";

                    string p =
                        (l.positive ? "" : "not (") + sanitise(l.predicate);

                    dout << "(" << p;
                    for (string v : l.arguments) dout << " " << sanitise(v);
                    if (!l.positive) dout << ")";
                    dout << "))" << endl;
                }

                dout << "\t\t)" << endl;
            }

            if (t.eff.size() || t.ceff.size() ||
                (hasActionCosts && t.costExpression.size())) {
                // Effect
                dout << "\t\t:effect (and" << endl;

                for (literal l : t.eff) {
                    dout << "\t\t\t(";

                    if (l.temp_qual == AT)
                        dout << "at ";
                    else if (l.temp_qual == OVER)
                        dout << "over ";

                    if (l.timed == START)
                        dout << "start ";
                    else if (l.timed == END)
                        dout << "end ";
                    else if (l.timed == ALL)
                        dout << "all ";

                    string p =
                        (l.positive ? "" : "not (") + sanitise(l.predicate);
                    dout << "(" << p;

                    for (string v : l.arguments) dout << " " << sanitise(v);
                    if (!l.positive) dout << ")";
                    dout << "))" << endl;
                }

                for (conditional_effect ceff : t.ceff) {
                    for (int positive = 0; positive < 2; positive++) {
                        if ((neg_pred.count(ceff.effect.predicate)) ||
                            (ceff.effect.positive == positive)) {
                            dout << "\t\t\t(when (and";

                            for (literal l : ceff.condition) {
                                dout << " (";
                                if (!l.positive) dout << "not (";
                                dout << sanitise(l.predicate);
                                for (string v : l.arguments)
                                    dout << " " << sanitise(v);
                                if (!l.positive) dout << ")";
                                dout << ")";
                            }

                            dout << ") (";

                            // Actual effect
                            if (!positive) dout << "not (";
                            dout
                                << ((ceff.effect.positive == positive) ? ""
                                                                       : "not_")
                                << sanitise(ceff.effect.predicate);
                            for (string v : ceff.effect.arguments)
                                dout << " " << sanitise(v);
                            if (!positive) dout << ")";

                            dout << "))" << endl;
                        }
                    }
                }

                dout << "\t\t)" << endl;
            }

            dout << "\t)" << endl << endl;
        }

        dout << ")" << endl;
    }

    if (problem) {
        bool instance_is_classical = true;
        for (task t : abstract_tasks)
            if (t.name == "__top") instance_is_classical = false;

        for (parsed_task t : parsed_abstract)
            if (t.name == "__top") instance_is_classical = false;

        if (!instance_is_classical) {
            pout << "\t(:htn" << endl;
            pout << "\t\t:parameters ()" << endl;

            if (requests.size()) {
                pout << "\t\t:requests (and" << endl;
                for (request r : requests)
                    pout << "\t\t\t(" << r.id << ")" << endl;
                pout << "\t\t)" << endl;
            }

            pout << "\t)" << endl;
        }

        pout << "\t(:init" << endl;
        for (ground_literal gl : init) {
            // Don't output negatives in normal mode
            if (!gl.positive) continue;

            pout << "\t\t(";

            if (gl.temp_qual == OVER && gl.timed == ALL) pout << "over all (";

            if (!gl.positive) pout << "not (";

            pout << sanitise(gl.predicate);
            for (string c : gl.args) pout << " " << sanitise(c);
            if (!gl.positive) pout << ")";

            if (gl.temp_qual == OVER && gl.timed == ALL) pout << ")";

            pout << ")" << endl;
        }

        for (auto [oldType, predicate] : typeConstraint) {
            for (string c : sorts[oldType])
                pout << "\t\t(" << sanitise(predicate) << " " << sanitise(c)
                     << ")" << endl;
        }

        // Metric
        if (hasActionCosts) {
            for (pair<ground_literal, int> f : init_functions) {
                pout << "\t\t(= (" << sanitise(f.first.predicate);
                for (string c : f.first.args) pout << " " << sanitise(c);
                pout << ") " << f.second << ")" << endl;
            }
        }

        pout << "\t)" << endl;

        if (goal_formula != nullptr && !goal_formula->isEmpty()) {
            print_formula_for(pout, goal_formula, "(:goal");
            pout << "  )" << endl;
        }

        pout << ")" << endl;
    }
}
