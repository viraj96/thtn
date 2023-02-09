#include <cassert>
#include <iostream>
#include <algorithm>

#include "parsetree.hpp"
#include "properties.hpp"

using namespace std;

void liftedPropertyTopSortDFS(string cur, map<string, vector<string> > &adj,
                              map<string, int> &colour,
                              vector<string> &liftedTopSort) {
    assert(colour[cur] != 1);
    if (colour[cur]) return;

    colour[cur] = 1;
    for (string &nei : adj[cur])
        liftedPropertyTopSortDFS(nei, adj, colour, liftedTopSort);
    colour[cur] = 2;

    liftedTopSort.push_back(cur);
}

vector<string> liftedPropertyTopSort(parsed_task_network *tn) {
    vector<string> liftedTopSort = vector<string>();
    map<string, vector<string> > adj = map<string, vector<string> >();
    for (arg_and_type *nei : tn->ordering)
        adj[nei->first].push_back(nei->second);

    map<string, int> colour = map<string, int>();

    for (sub_task *t : tn->tasks)
        if (!colour[t->id])
            liftedPropertyTopSortDFS(t->id, adj, colour, liftedTopSort);

    reverse(liftedTopSort.begin(), liftedTopSort.end());

    return liftedTopSort;
}

bool isTopSortTotalOrder(vector<string> &liftedTopSort,
                         parsed_task_network *tn) {
    for (size_t i = 1; i < liftedTopSort.size(); i++) {
        bool orderEnforced = false;
        for (arg_and_type *nei : tn->ordering)
            if (nei->first == liftedTopSort[i - 1] &&
                nei->second == liftedTopSort[i]) {
                orderEnforced = true;
                break;
            }

        if (!orderEnforced) return false;
    }

    return true;
}

bool recursionFindingDFS(string cur, map<string, int> &colour) {
    if (colour[cur] == 1) return true;
    if (colour[cur]) return false;

    colour[cur] = 1;
    for (parsed_method &m : parsed_methods[cur])
        for (sub_task *sub : m.tn->tasks)
            if (recursionFindingDFS(sub->task, colour)) return true;
    colour[cur] = 2;

    return false;
}

bool isRecursiveParentSort(string current, string target) {
    if (current == target) return true;
    for (sort_definition &sd : sort_definitions) {
        for (string &ss : sd.declared_sorts)
            if (current == ss) {
                if (!sd.has_parent_sort) continue;
                if (isRecursiveParentSort(sd.parent_sort, target)) return true;
            }
    }

    return false;
}

void printProperties() {
    // Determine lifted instance properties and print them

    // 1. Total order
    bool totalOrder = true;
    for (auto &[_, ms] : parsed_methods) {
        (void)_;
        for (parsed_method &m : ms) {
            // Tasks for functional methods are not still defined. They will
            // be defined after the planner gets to it.
            if (!m.functional) {
                if (m.tn->tasks.size() < 2) continue;

                // Do topsort
                vector<string> liftedTopSort = liftedPropertyTopSort(m.tn);

                if (!isTopSortTotalOrder(liftedTopSort, m.tn)) {
                    cout << "Partially Ordered Method: " << m.name << endl;
                    totalOrder = false;
                }
            }
        }
    }

    cout << "Instance is totally-ordered: ";
    if (totalOrder) {
        cout << "yes" << endl;
    } else {
        cout << "no" << endl;
    }

    // 2. Recursion
    map<string, int> colour = map<string, int>();
    bool hasLiftedRecursion = recursionFindingDFS("__top", colour);

    cout << "Instance is acyclic:         ";
    if (!hasLiftedRecursion) {
        cout << "yes" << endl;
    } else {
        cout << "no" << endl;
    }

    // Requirements
    cout << "Requirements:" << endl;
    cout << "\t:hierarchy" << endl;
    if (sort_definitions.size()) cout << "\t:typing" << endl;
    bool hasEquals = false;
    for (auto &[_, ms] : parsed_methods) {
        (void)_;
        for (parsed_method &m : ms)
            hasEquals |= m.prec->hasEquals() || m.eff->hasEquals();
    }
    for (parsed_task &p : parsed_primitive)
        hasEquals |= p.prec->hasEquals() || p.eff->hasEquals();
    if (hasEquals) cout << "\t:equality" << endl;

    bool exists_prec = false;
    for (auto &[_, ms] : parsed_methods) {
        (void)_;
        for (parsed_method &m : ms) exists_prec |= m.prec->hasExists();
    }
    for (parsed_task &p : parsed_primitive) exists_prec |= p.prec->hasExists();
    if (exists_prec) cout << "\t:existential-preconditions" << endl;

    bool forall_prec = false;
    for (auto &[_, ms] : parsed_methods) {
        (void)_;
        for (parsed_method &m : ms) forall_prec |= m.prec->hasForall();
    }
    for (parsed_task &p : parsed_primitive) forall_prec |= p.prec->hasForall();
    if (forall_prec) cout << "\t:universal-preconditions" << endl;

    bool forall_eff = false;
    for (auto &[_, ms] : parsed_methods) {
        (void)_;
        for (parsed_method &m : ms) forall_eff |= m.eff->hasForall();
    }
    for (parsed_task &p : parsed_primitive) forall_eff |= p.eff->hasForall();
    if (forall_eff) cout << "\t:universal-effects" << endl;

    bool method_preconditon = false;
    for (auto &[_, ms] : parsed_methods) {
        (void)_;
        for (parsed_method &m : ms) method_preconditon |= !m.prec->isEmpty();
    }
    if (method_preconditon) cout << "\t:method-preconditions" << endl;

    for (auto &[_, ms] : parsed_methods) {
        (void)_;
        for (parsed_method &m : ms) {
            if (!m.functional) {
                for (sub_task *sub : m.tn->tasks) {
                    // Quadratic
                    for (parsed_task prim : parsed_primitive) {
                        if (sub->task != prim.name) continue;
                        for (size_t i = 0; i < prim.arguments->vars.size();
                             i++) {
                            string primArgType = prim.arguments->vars[i].second;
                            string method_var = sub->arguments->vars[i];

                            for (arg_and_type a : m.vars->vars) {
                                if (method_var == a.first) {
                                    if (primArgType != a.second) {
                                        if (!isRecursiveParentSort(
                                                a.second, primArgType)) {
                                            cout << "Method: " << m.name
                                                 << endl;
                                            cout << "\tVariable: " << method_var
                                                 << endl;
                                            cout << "\tSubtask: " << sub->id
                                                 << endl;
                                            cout << "\t\tTask argument: "
                                                 << primArgType << endl;
                                            cout << "\t\tvariable: " << a.second
                                                 << endl;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    for (parsed_task prim : parsed_abstract) {
                        if (sub->task != prim.name) continue;
                        for (size_t i = 0; i < prim.arguments->vars.size();
                             i++) {
                            string primArgType = prim.arguments->vars[i].second;
                            string method_var = sub->arguments->vars[i];

                            for (pair<string, string> a : m.vars->vars) {
                                if (method_var == a.first) {
                                    if (primArgType != a.second) {
                                        if (!isRecursiveParentSort(
                                                a.second, primArgType)) {
                                            cout << "Method: " << m.name
                                                 << endl;
                                            cout << "\tVariable: " << method_var
                                                 << endl;
                                            cout << "\tSubtask: " << sub->id
                                                 << endl;
                                            cout << "\t\tTask argument: "
                                                 << primArgType << endl;
                                            cout << "\t\tvariable: " << a.second
                                                 << endl;
                                        }
                                    }

                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
