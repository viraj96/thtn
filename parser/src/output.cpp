#include <map>
#include <cstdio>
#include <vector>
#include <cassert>
#include <iostream>

#include "cwa.hpp"
#include "domain.hpp"
#include "output.hpp"
#include "parsetree.hpp"

#include "hddl_header.hpp"

using namespace std;

void verbose_output(int verbosity) {
    cout << "number of sorts: " << sorts.size() << endl;
    if (verbosity > 0) {
        for (pair<string, set<string> > s : sorts) {
            cout << "\t" << s.first << ":";
            for (string e : s.second) cout << " " << e;
            cout << endl;
        }

        for (pair<string, set<map<string, var_declaration> > > c : csorts) {
            cout << "\t" << c.first << " - ";
            for (map<string, var_declaration> s : c.second) {
                for (pair<string, var_declaration> m : s) {
                    cout << m.first << " { ";
                    for (arg_and_type v : m.second.vars) {
                        cout << v.first << " = " << v.second << " ";
                    }
                    cout << " }\n";
                }
            }
        }
    }

    cout << "number of predicates: " << predicate_definitions.size() << endl;
    if (verbosity > 0) {
        for (predicate_definition def : predicate_definitions) {
            cout << "\t" << def.name;
            for (string arg : def.argument_sorts) cout << " " << arg;
            cout << endl;
        }
    }

    cout << "number of primitives: " << primitive_tasks.size() << endl;
    if (verbosity > 0) {
        for (task t : primitive_tasks) {
            cout << "\t" << t.name << endl;
            if (verbosity == 1) continue;
            cout << "\t\tvars:" << endl;
            for (arg_and_type v : t.vars)
                cout << "\t\t     " << v.first << " - " << v.second << endl;
            if (verbosity == 2) continue;
            cout << "\t\tprec:" << endl;
            for (literal l : t.prec) {
                cout << "\t\t     " << (l.positive ? "+" : "-") << " "
                     << l.predicate;
                for (string v : l.arguments) cout << " " << v;
                cout << endl;
            }
            cout << "\t\teff:" << endl;
            for (literal l : t.eff) {
                cout << "\t\t     " << (l.positive ? "+" : "-") << " "
                     << l.predicate;
                for (string v : l.arguments) cout << " " << v;
                cout << endl;
            }
            cout << "\t\tconstraints:" << endl;
            for (literal l : t.constraints) {
                cout << "\t\t     " << (l.positive ? "+" : "-") << " "
                     << l.predicate;
                for (string v : l.arguments) cout << " " << v;
                cout << endl;
            }
        }
    }

    cout << "number of abstracts: " << abstract_tasks.size() << endl;
    if (verbosity > 0) {
        for (task t : abstract_tasks) {
            cout << "\t" << t.name << endl;
            if (verbosity == 1) continue;
            cout << "\t\tvars:" << endl;
            for (arg_and_type v : t.vars)
                cout << "\t\t     " << v.first << " - " << v.second << endl;
        }
    }

    cout << "number of methods: " << methods.size() << endl;
    if (verbosity > 0) {
        for (method m : methods) {
            cout << "\t" << m.name << endl;
            cout << "\t\tabstract task: " << m.at;
            for (string v : m.atargs) cout << " " << v;
            cout << endl;
            if (verbosity == 1) continue;
            cout << "\t\tvars:" << endl;
            for (arg_and_type v : m.vars)
                cout << "\t\t     " << v.first << " - " << v.second << endl;
            if (verbosity == 2) continue;
            cout << "\t\tsubtasks:" << endl;
            for (plan_step ps : m.ps) {
                cout << "\t\t     " << ps.id << " " << ps.task;
                for (string v : ps.args) cout << " " << v;
                cout << endl;
            }
            if (verbosity == 3) continue;
            cout << "\t\tordering:" << endl;
            for (arg_and_type o : m.ordering)
                cout << "\t\t     " << o.first << " < " << o.second << endl;
            cout << "\t\tconstraints:" << endl;
            for (literal l : m.constraints) {
                cout << "\t\t     " << (l.positive ? "+" : "-") << " "
                     << l.predicate;
                for (string v : l.arguments) cout << " " << v;
                cout << endl;
            }
        }
    }

    cout << "Initial state: " << init.size() << endl;
    if (verbosity > 0) {
        for (ground_literal l : init) {
            cout << "\t" << (l.positive ? "+" : "-") << l.predicate;
            for (string c : l.args) cout << " " << c;
            cout << endl;
        }
    }

    cout << "Goal state: " << goal.size() << endl;
    if (verbosity > 0) {
        for (ground_literal l : goal) {
            cout << "\t" << (l.positive ? "+" : "-") << l.predicate;
            for (string c : l.args) cout << " " << c;
            cout << endl;
        }
    }
}

void simple_hddl_output(ostream& dout) {
    // Prepare indices
    map<string, int> constants = map<string, int>();
    vector<string> constants_out = vector<string>();
    for (pair<string, set<string> > x : sorts) {
        for (string s : x.second) {
            if (constants.count(s) == 0)
                constants[s] = constants.size(), constants_out.push_back(s);
        }
    }

    map<string, int> sort_id = map<string, int>();
    vector<pair<string, set<string> > > sort_out =
        vector<pair<string, set<string> > >();
    for (pair<string, set<string> > x : sorts)
        if (!sort_id.count(x.first))
            sort_id[x.first] = sort_id.size(), sort_out.push_back(x);

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

    map<string, int> predicates = map<string, int>();
    vector<pair<string, predicate_definition> > predicate_out =
        vector<pair<string, predicate_definition> >();
    vector<arg_and_type> mutexPredicates = vector<arg_and_type>();
    for (predicate_definition p : predicate_definitions) {
        predicates["+" + p.name] = predicates.size();
        predicate_out.push_back(make_pair("+" + p.name, p));

        if (neg_pred.count(p.name)) {
            predicates["-" + p.name] = predicates.size();
            predicate_out.push_back(make_pair("-" + p.name, p));

            // + and - predicates are known mutexes
            mutexPredicates.push_back(make_pair("+" + p.name, "-" + p.name));
        }
    }

    map<string, int> function_declarations = map<string, int>();
    vector<predicate_definition> functions_out = vector<predicate_definition>();
    for (pair<predicate_definition, string> p : parsed_functions) {
        if (p.second != numeric_funtion_type) {
            cerr << "The parser currently supports only numeric \
                (type \"number\") functions."
                 << endl;
            exit(1);
        }

        // Don't output the metric target, we don't need it
        if (p.first.name == metric_target) continue;

        function_declarations[p.first.name] = function_declarations.size();
        functions_out.push_back(p.first);
    }

    // Determine whether the instance actually has action costs. If not, we
    // insert in the output that every action has cost 1
    bool instance_has_action_costs = metric_target != dummy_function_type;

    bool instance_is_classical = true;
    for (task t : abstract_tasks)
        if (t.name == "__top") instance_is_classical = false;

    // If we are in a classical domain remove everything HTN
    if (instance_is_classical) {
        abstract_tasks.clear();
        methods.clear();
    }

    map<string, int> task_id = map<string, int>();
    vector<pair<task, bool> > task_out = vector<pair<task, bool> >();
    for (task t : primitive_tasks) {
        if (task_id.count(t.name) != 0)
            cerr << "Duplicate primitive task " << t.name << endl;

        assert(task_id.count(t.name) == 0);
        task_id[t.name] = task_id.size();
        task_out.push_back(make_pair(t, true));
    }

    for (task t : abstract_tasks) {
        if (task_id.count(t.name) != 0)
            cerr << "Duplicate abstract task " << t.name << endl;

        assert(task_id.count(t.name) == 0);
        task_id[t.name] = task_id.size();
        task_out.push_back(make_pair(t, false));
    }

    // Write domain to std out
    dout << "#number_constants_number_sorts" << endl;
    dout << constants.size() << " " << sorts.size() << endl;
    dout << "#constants" << endl;
    for (string c : constants_out) dout << c << endl;
    dout << "#end_constants" << endl;
    dout << "#sorts_each_with_number_of_members_and_members" << endl;
    for (pair<string, set<string> > s : sort_out) {
        dout << s.first << " " << s.second.size();
        for (string c : s.second) dout << " " << constants[c];
        dout << endl;
    }
    dout << "#end_sorts" << endl;
    dout << "#number_of_predicates" << endl;
    dout << predicate_out.size() << endl;
    dout << "#predicates_each_with_number_of_arguments_and_argument_sorts"
         << endl;
    for (pair<string, predicate_definition> p : predicate_out) {
        dout << p.first << " " << p.second.argument_sorts.size();
        for (string s : p.second.argument_sorts)
            assert(sort_id.count(s)), dout << " " << s << " " << sort_id[s];
        dout << endl;
    }

    dout << "#end_predicates" << endl;
    dout << "#begin_predicate_mutexes" << endl;
    dout << mutexPredicates.size() << endl;

    for (auto [one, two] : mutexPredicates)
        dout << predicates[one] << " " << predicates[two] << endl;
    dout << "#end_predicate_mutexes" << endl;

    dout << "#number_of_functions" << endl;
    dout << function_declarations.size() << endl;
    dout << "#function_declarations_with_number_of_arguments_and_argument_sorts"
         << endl;

    for (predicate_definition f : functions_out) {
        dout << f.name << " " << f.argument_sorts.size();
        for (string s : f.argument_sorts)
            assert(sort_id.count(s)), dout << " " << sort_id[s];
        dout << endl;
    }

    dout << "#number_primitive_tasks_and_number_abstract_tasks" << endl;
    dout << primitive_tasks.size() << " " << abstract_tasks.size() << endl;

    for (pair<task, bool> tt : task_out) {
        task t = tt.first;
        dout << "#begin_task_name_number_of_original_variables_and_number_of_"
                "variables"
             << endl;
        assert(int(t.vars.size()) >= t.number_of_original_vars);
        dout << t.name << " " << t.number_of_original_vars << " "
             << t.vars.size() << endl;
        dout << "#sorts_of_variables" << endl;

        map<string, int> v_id = map<string, int>();
        for (arg_and_type v : t.vars)
            assert(sort_id.count(v.second)), dout << sort_id[v.second] << " ",
                v_id[v.first] = v_id.size();

        dout << endl;
        dout << "#end_variables" << endl;

        if (tt.second) {
            dout << "#number_of_cost_statements" << endl;
            if (instance_has_action_costs)
                dout << t.costExpression.size() << endl;
            else
                dout << 1 << endl;
            dout << "#begin_cost_statements" << endl;
            if (instance_has_action_costs) {
                for (literal c : t.costExpression) {
                    if (c.isConstantCostExpression)
                        dout << "const " << c.costValue << endl;
                    else {
                        dout << "var " << function_declarations[c.predicate];
                        for (string v : c.arguments) dout << " " << v_id[v];
                        dout << endl;
                    }
                }

            } else
                dout << "const 1" << endl;

            dout << "#end_cost_statements" << endl;
            dout << "#preconditions_each_predicate_and_argument_variables"
                 << endl;
            dout << t.prec.size() << endl;
            for (literal l : t.prec) {
                string p = (l.positive ? "+" : "-") + l.predicate;
                dout << predicates[p];
                for (string v : l.arguments) dout << " " << v_id[v];
                dout << endl;
            }

            // Determine number of add and delete effects
            int add = 0, del = 0;
            for (literal l : t.eff) {
                if (neg_pred.count(l.predicate))
                    add++, del++;
                else if (l.positive)
                    add++;
                else
                    del++;
            }

            // Count conditional add and delete effects
            int cadd = 0, cdel = 0;
            for (conditional_effect ceff : t.ceff) {
                literal l = ceff.effect;
                if (neg_pred.count(l.predicate))
                    cadd++, cdel++;
                else if (l.positive)
                    cadd++;
                else
                    cdel++;
            }

            dout << "#add_each_predicate_and_argument_variables" << endl;
            dout << add << endl;
            for (literal l : t.eff) {
                if (!neg_pred.count(l.predicate) && !l.positive) continue;
                string p = (l.positive ? "+" : "-") + l.predicate;
                dout << predicates[p];
                for (string v : l.arguments) dout << " " << v_id[v];
                dout << endl;
            }

            dout << "#conditional_add_each_with_conditions_and_effect" << endl;
            dout << cadd << endl;
            for (conditional_effect ceff : t.ceff) {
                // If this is a delete effect and the "-" predicates is not
                // necessary
                if (!neg_pred.count(ceff.effect.predicate) &&
                    !ceff.effect.positive)
                    continue;

                // Number of conditions
                dout << ceff.condition.size();
                for (literal l : ceff.condition) {
                    string p = (l.positive ? "+" : "-") + l.predicate;
                    // Two spaces for better human readability
                    dout << "  " << predicates[p];
                    for (string v : l.arguments) dout << " " << v_id[v];
                }

                // Effect
                string p =
                    (ceff.effect.positive ? "+" : "-") + ceff.effect.predicate;
                // Two spaces for better human readability
                dout << "  " << predicates[p];
                for (string v : ceff.effect.arguments) dout << " " << v_id[v];
                dout << endl;
            }

            dout << "#del_each_predicate_and_argument_variables" << endl;
            dout << del << endl;
            for (literal l : t.eff) {
                if (!neg_pred.count(l.predicate) && l.positive) continue;
                string p = (l.positive ? "-" : "+") + l.predicate;
                dout << predicates[p];
                for (string v : l.arguments) dout << " " << v_id[v];
                dout << endl;
            }

            dout << "#conditional_del_each_with_conditions_and_effect" << endl;
            dout << cdel << endl;
            for (conditional_effect ceff : t.ceff) {
                // If this is an add effect and the "+" predicates is not
                // necessary
                if (!neg_pred.count(ceff.effect.predicate) &&
                    ceff.effect.positive)
                    continue;

                // Number of conditions
                dout << ceff.condition.size();
                for (literal l : ceff.condition) {
                    string p = (l.positive ? "+" : "-") + l.predicate;
                    // Two spaces for better human readability
                    dout << "  " << predicates[p];
                    for (string v : l.arguments) dout << " " << v_id[v];
                }

                // Effect
                string p =
                    (ceff.effect.positive ? "-" : "+") + ceff.effect.predicate;
                // Two spaces for better human readability
                dout << "  " << predicates[p];
                for (string v : ceff.effect.arguments) dout << " " << v_id[v];
                dout << endl;
            }

            dout << "#variable_constraints_first_number_then_individual_"
                    "constraints"
                 << endl;
            dout << t.constraints.size() << endl;
            for (literal l : t.constraints) {
                if (!l.positive) dout << "!";
                dout << "= " << v_id[l.arguments[0]] << " "
                     << v_id[l.arguments[1]] << endl;
                // Cannot be a constant
                assert(l.arguments[0][0] == '?');
                // Cannot be a constant
                assert(l.arguments[1][0] == '?');
            }
        }

        dout << "#end_of_task" << endl;
    }

    dout << "#number_of_methods" << endl;
    dout << methods.size() << endl;

    for (method m : methods) {
        dout << "#begin_method_name_abstract_task_number_of_variables" << endl;
        dout << m.name << " " << task_id[m.at] << " " << m.vars.size() << endl;
        dout << "#variable_sorts" << endl;

        map<string, int> v_id = map<string, int>();
        for (arg_and_type v : m.vars)
            assert(sort_id.count(v.second)), dout << sort_id[v.second] << " ",
                v_id[v.first] = v_id.size();
        dout << endl;
        dout << "#parameter_of_abstract_task" << endl;
        for (string v : m.atargs) dout << v_id[v] << " ";
        dout << endl;
        dout << "#number_of_subtasks" << endl;
        dout << m.ps.size() << endl;
        dout << "#subtasks_each_with_task_id_and_parameter_variables" << endl;
        map<string, int> ps_id = map<string, int>();
        for (plan_step ps : m.ps) {
            ps_id[ps.id] = ps_id.size();
            dout << task_id[ps.task];
            for (string v : ps.args) dout << " " << v_id[v];
            dout << endl;
        }
        dout << "#number_of_ordering_constraints_and_ordering" << endl;
        dout << m.ordering.size() << endl;
        for (arg_and_type o : m.ordering)
            dout << ps_id[o.first] << " " << ps_id[o.second] << endl;

        dout << "#variable_constraints" << endl;
        dout << m.constraints.size() << endl;
        for (literal l : m.constraints) {
            if (!l.positive) dout << "!";
            dout << "= " << v_id[l.arguments[0]] << " " << v_id[l.arguments[1]]
                 << endl;
            // Cannot be a constant
            assert(l.arguments[1][0] == '?');
        }

        dout << "#end_of_method" << endl;
    }

    dout << "#init_and_goal_facts" << endl;
    dout << init.size() << " " << goal.size() << endl;
    for (ground_literal gl : init) {
        string pn = (gl.positive ? "+" : "-") + gl.predicate;
        assert(predicates.count(pn) != 0);
        dout << predicates[pn];
        for (string c : gl.args) dout << " " << constants[c];
        dout << endl;
    }
    dout << "#end_init" << endl;
    for (ground_literal gl : goal) {
        string pn = (gl.positive ? "+" : "-") + gl.predicate;
        assert(predicates.count(pn) != 0);
        dout << predicates[pn];
        for (string c : gl.args) dout << " " << constants[c];
        dout << endl;
    }
    dout << "#end_goal" << endl;
    dout << "#init_function_facts" << endl;
    vector<string> function_lines = vector<string>();
    for (pair<ground_literal, int> f : init_functions) {
        if (f.first.predicate == metric_target) {
            cerr << "Ignoring initialisation of metric target \""
                 << metric_target << "\"" << endl;
            continue;
        }
        string line = to_string(function_declarations[f.first.predicate]);
        for (string c : f.first.args) line += " " + to_string(constants[c]);
        line += " " + to_string(f.second);
        function_lines.push_back(line);
    }
    dout << function_lines.size() << endl;
    for (string l : function_lines) dout << l << endl;

    dout << "#initial_task" << endl;
    if (instance_is_classical)
        dout << "-1" << endl;
    else
        dout << task_id["__top"] << endl;
}
