#include "planner.hpp"

#include <boost/range/adaptor/indexed.hpp>
#include <filesystem>
#include <tuple>

#include "domain.hpp"
#include "utils.hpp"

string
slot::to_string() const
{
    string result = "Slot: ";
    result += "\t Token = " + tk.to_string() + "\n";
    result += "\t Previous Token = " + prev.to_string() + "\n";
    result += "\t Next Token = " + next.to_string() + "\n";
    result += "\t Timeline Id = " + tl_id + "\n";
    result += "\t Dependent End Timepoints = \n\t\t";
    for (string dep_end : dependent_ends)
        result += dep_end + "  ";
    result += "\n";
    return result;
}

string
primitive_solution::to_string() const
{
    string result = "Primitive Solution: \n";
    result += "\t Primitive Token = " + primitive_token.to_string() + "\n";
    result += "\t Slots = \n";
    for (slot s : token_slots)
        result += "\t\t" + s.to_string() + "\n";
    result += "\n";
    return result;
}

string
tasknetwork_solution::to_string() const
{
    string result = "TaskNetwork Solution: \n";
    result += "\t Plan Id =  " + std::to_string(plan_id) + "\n";
    result += "\t Request Id = " + request_id + "\n";
    result += "\t Robot Assignment = " + robot_assignment + "\n";
    result += "\t Metric Value = " + std::to_string(metric_value) + "\n";
    result += "\t Solution =  \n";
    for (primitive_solution ps : solution)
        result += "\t\t" + ps.to_string() + "\n";
    result += "\n";
    return result;
}

bool
operator==(const slot& lhs, const slot& rhs)
{
    string l_tk_start = lhs.tk.get_start();
    string l_tk_end = lhs.tk.get_end();
    string l_prev_start = lhs.prev.get_start();
    string l_prev_end = lhs.prev.get_end();
    string l_next_start = lhs.next.get_start();
    string l_next_end = lhs.next.get_end();

    string r_tk_start = rhs.tk.get_start();
    string r_tk_end = rhs.tk.get_end();
    string r_prev_start = rhs.prev.get_start();
    string r_prev_end = rhs.prev.get_end();
    string r_next_start = rhs.next.get_start();
    string r_next_end = rhs.next.get_end();

    return tie(
             l_tk_start, l_tk_end, l_prev_start, l_prev_end, l_next_start, l_next_end, lhs.tl_id) ==
           tie(r_tk_start, r_tk_end, r_prev_start, r_prev_end, r_next_start, r_next_end, rhs.tl_id);
}

bool
operator!=(const slot& lhs, const slot& rhs)
{
    return !(lhs == rhs);
}

string
evaluate(string arg, set<arg_and_type>* knowns)
{
    // Evaluate arguments without attributes
    string arg_value = "";
    for (pair<string, set<string>> c : constants)
        for (string a : c.second)
            if (a == arg)
                arg_value = a;

    for (arg_and_type at : (*knowns))
        if (at.first == arg)
            arg_value = at.second;

    return arg_value;
}

arg_and_type
evaluate(arg_and_type argument, set<arg_and_type>* knowns, world_state* current_state)
{
    string arg = argument.first;
    string attr = argument.second;

    string arg_value = evaluate(arg, knowns);
    assert(arg_value.length() != 0);

    for (pair<string, object_state> obj : (*current_state))
        if (obj.first == arg_value) {
            // Easier case when need to just query the object state
            string attr_value = "";
            for (arg_and_type at : obj.second.attribute_states.vars)
                if (at.first == attr) {
                    attr_value = at.second;
                    return make_pair(arg_value, attr_value);
                }
        }

    return arg_and_type();
}

bool
check_functional_predicate(literal* precondition,
                           set<arg_and_type>* knowns,
                           world_state* current_state)
{
    vector<string> args = vector<string>();

    for (string arg : precondition->arguments) {
        arg_and_type arg_and_attr = arg_and_type();
        if (arg.find(".") != string::npos)
            arg_and_attr = get_arg_and_attr(arg);

        if (arg_and_attr == arg_and_type()) {
            for (arg_and_type var : *(knowns))
                if (arg == var.first)
                    args.push_back(var.second);

        } else {
            for (arg_and_type var : (*knowns))
                if (arg_and_attr.first == var.first)
                    for (const arg_and_type& v : (*current_state)[var.second].attribute_states.vars)
                        if (arg_and_attr.second == v.first)
                            args.push_back(v.second);
        }
    }

    bool lit_pass = precondition->func_literal(args, current_state);

    if (precondition->positive)
        return lit_pass;
    else
        return !lit_pass;
}

bool
check_equal_predicate(literal* precondition, set<arg_and_type>* knowns, world_state* current_state)
{
    bool lit_pass = false;

    assert(precondition->arguments.size() == 2);
    string arg1 = precondition->arguments[0];
    string arg2 = precondition->arguments[1];

    arg_and_type arg1_n_attr = arg_and_type(), arg2_n_attr = arg_and_type();

    size_t arg1_has_attr = arg1.find(".");
    size_t arg2_has_attr = arg2.find(".");

    if (arg1_has_attr != string::npos)
        arg1_n_attr = get_arg_and_attr(arg1);
    if (arg2_has_attr != string::npos)
        arg2_n_attr = get_arg_and_attr(arg2);

    if (arg1_has_attr == string::npos && arg2_has_attr == string::npos) {
        string value1 = evaluate(arg1, knowns);
        string value2 = evaluate(arg2, knowns);
        assert(value1.length() != 0 && value2.length() != 0);
        lit_pass = (value1 == value2);

    } else if (arg1_has_attr != string::npos && arg2_has_attr == string::npos) {
        string value2 = evaluate(arg2, knowns);
        arg_and_type arg1_n_attr_value = evaluate(arg1_n_attr, knowns, current_state);
        assert(arg1_n_attr_value != arg_and_type());
        lit_pass = (arg1_n_attr_value.second == value2);

    } else if (arg1_has_attr == string::npos && arg2_has_attr != string::npos) {
        string value1 = evaluate(arg1, knowns);
        arg_and_type arg2_n_attr_value = evaluate(arg2_n_attr, knowns, current_state);
        assert(arg2_n_attr_value != arg_and_type());
        lit_pass = (arg2_n_attr_value.second == value1);

    } else {
        arg_and_type arg1_n_attr_value = evaluate(arg1_n_attr, knowns, current_state);
        arg_and_type arg2_n_attr_value = evaluate(arg2_n_attr, knowns, current_state);
        assert(arg1_n_attr_value != arg_and_type());
        assert(arg2_n_attr_value != arg_and_type());
        lit_pass = (arg1_n_attr_value.second == arg2_n_attr_value.second);
    }

    if (precondition->positive)
        return lit_pass;
    else
        return !lit_pass;
}

bool
check_init(literal* precondition,
           set<arg_and_type>* knowns,
           world_state* current_state,
           ground_literal* lit)
{
    bool lit_pass = false;

    if (lit->temp_qual == OVER && lit->timed == ALL)
        if (lit->predicate == precondition->predicate) {
            int arg_num = 0;
            bool arg_pass = true;
            for (string s : precondition->arguments) {
                arg_pass = true;
                if (s.find(".") != string::npos) {
                    arg_and_type arg_n_attr = get_arg_and_attr(s);
                    arg_and_type arg_n_attr_value = evaluate(arg_n_attr, knowns, current_state);
                    assert(arg_n_attr_value != arg_and_type());

                    if (lit->args[arg_num] != arg_n_attr_value.second)
                        arg_pass = false;

                } else {
                    string value = evaluate(s, knowns);
                    assert(value.length() != 0);
                    if (lit->args[arg_num] != value)
                        arg_pass = false;
                }

                if (!arg_pass)
                    break;

                arg_num++;
            }

            lit_pass = arg_pass;
        }

    if (precondition->positive)
        return lit_pass;
    else
        return !lit_pass;
}

bool
check_precondition(literal* precondition,
                   set<arg_and_type>* knowns,
                   world_state* current_state,
                   vector<ground_literal>* init)
{
    if (precondition->functional)
        return check_functional_predicate(precondition, knowns, current_state);

    if (precondition->predicate == dummy_equal_literal)
        return check_equal_predicate(precondition, knowns, current_state);

    for (ground_literal lit : (*init)) {
        bool lit_pass = check_init(precondition, knowns, current_state, &lit);
        if (lit_pass)
            return true;
    }

    return false;
}

arg_and_type
get_arg_and_attr(string argument)
{
    string arg = argument.substr(0, argument.find("."));
    argument.erase(0, argument.find(".") + 1);
    string attr = argument;
    return make_pair(arg, attr);
}

void
update_object_state(object_state* obj, Token* prev)
{
    // If the previous token was the head token then you only need to extract
    // the initial state
    if (prev->get_name() == "head")
        for (pair<string, object_state> os : initial_state)
            if (os.second.object_name == obj->object_name) {
                *obj = os.second;
                return;
            }

    // If it wasnt then you need to apply the effects of the previous token
    for (literal eff : prev->get_effects())
        if (eff.predicate == dummy_equal_literal) {
            string arg1 = eff.arguments[0];
            string arg2 = eff.arguments[1];
            arg_and_type arg1_n_attr = arg_and_type();

            // If second argument is a variable then ground it
            if (arg2[0] == '?')
                for (arg_and_type k : prev->get_knowns())
                    if (k.first == arg2)
                        arg2 = k.second;

            // Case when the predicate was pushed via the task network but the
            // argument does not exist within its local context.
            if (arg2[0] == '?')
                continue;

            // Only first argument can have attributes as the second one is
            // going to be assigned to the first
            if (arg1.find(".") != string::npos)
                arg1_n_attr = get_arg_and_attr(arg1);

            // First argument always has attributes. Otherwise no point in
            // changing state
            if (arg1_n_attr != arg_and_type())
                for (auto it = obj->attribute_states.vars.begin();
                     it != obj->attribute_states.vars.end();
                     it++)
                    if (it->first == arg1_n_attr.second)
                        it->second = arg2;
        }
}

// Slightly depends on the domain model
Token
gen_token(arg_and_type argument,
          Token* causal_token,
          stack<STN_operation>* search_operation_history,
          STN* stn)
{
    string name = "";
    for (arg_and_type arg : causal_token->get_arguments()) {
        if (causal_token->get_name() == "grasp" && arg.second == "robot") {
            for (arg_and_type k : causal_token->get_knowns())
                if (k.first == arg.first)
                    name = k.second;

        } else if (causal_token->get_name() == "release" && arg.second == "location") {
            for (arg_and_type k : causal_token->get_knowns())
                if (k.first == arg.first)
                    name = k.second;

        } else if (causal_token->get_name() == "rail_move" && arg.second == "robot") {
            for (arg_and_type k : causal_token->get_knowns())
                if (k.first == arg.first)
                    name = k.second;
        }
    }

    if (argument.second == "item") {
        if (causal_token->get_name() == "grasp")
            name += "_grasped";
        else
            name += "_released";

    } else if (argument.second == "rail_block")
        name += "_occupied";

    Token tk = instantiate_token(
      name, argument.second, stn, causal_token->get_request_id(), 0.0, search_operation_history);

    assert(tk != Token());

    for (arg_and_type k : causal_token->get_knowns())
        tk.add_knowns(k);
    for (arg_and_type arg : causal_token->get_arguments())
        tk.add_arguments(arg);
    for (literal eff : causal_token->get_effects())
        tk.add_effects(eff);
    for (literal prec : causal_token->get_preconditions())
        tk.add_preconditions(prec);

    return tk;
}

set<string>
get_literal_args_and_types(literal* lit, set<arg_and_type>* arguments)
{
    set<string> lit_arg_types = set<string>();

    for (string arg : lit->arguments) {
        string arg_to_check = arg;
        arg_and_type arg_and_attr = arg_and_type();
        if (arg.find(".") != string::npos)
            arg_and_attr = get_arg_and_attr(arg);

        if (arg_and_attr != arg_and_type())
            arg_to_check = arg_and_attr.first;

        for (arg_and_type a : (*arguments))
            if (arg_to_check == a.first) {
                string type = a.second;
                if (arg_and_attr != arg_and_type()) {
                    string attr = arg_and_attr.second;
                    for (sort_definition sd : sort_definitions)
                        for (string s : sd.declared_sorts)
                            if (s == a.second)
                                for (arg_and_type v : sd.vars.vars)
                                    if (v.first.substr(1) == attr)
                                        type += "-" + v.second;
                }
                lit_arg_types.insert(type);

                break;
            }
    }

    return lit_arg_types;
}

pair<bool, task>
find_satisfying_task(literal* precondition, set<string>* prec_arg_types)
{
    bool found_task = false;
    task satisfying_task = task();

    for (task pt : primitive_tasks) {
        for (literal eff : pt.eff)
            if (eff.predicate == precondition->predicate &&
                eff.positive == precondition->positive) {
                bool same = true;
                set<arg_and_type> vars = set<arg_and_type>();
                for (arg_and_type v : pt.vars)
                    vars.insert(v);
                set<string> eff_arg_types = get_literal_args_and_types(&eff, &vars);
                for (string prec_type : (*prec_arg_types)) {
                    bool found = false;
                    string prec_type_copy = prec_type, prec_param_type = prec_type,
                           prec_attr_type = "";
                    if (prec_type_copy.find("-") != string::npos) {
                        prec_param_type = prec_type_copy.substr(0, prec_type_copy.find("-"));
                        prec_type_copy.erase(0, prec_type_copy.find("-") + 1);
                        prec_attr_type = prec_type_copy.substr(0, prec_type_copy.find("-"));
                    }
                    for (string eff_type : eff_arg_types) {
                        string eff_type_copy = eff_type, eff_param_type = eff_type,
                               eff_attr_type = "";
                        if (eff_type_copy.find("-") != string::npos) {
                            eff_param_type = eff_type_copy.substr(0, eff_type_copy.find("-"));
                            eff_type_copy.erase(0, eff_type_copy.find("-") + 1);
                            eff_attr_type = eff_type_copy.substr(0, eff_type_copy.find("-"));
                        }
                        if (prec_attr_type != "" && eff_attr_type != "") {
                            if (prec_attr_type == eff_attr_type) {
                                found = true;
                                break;
                            }
                        } else if (prec_attr_type != "") {
                            if (prec_attr_type == eff_param_type) {
                                found = true;
                                break;
                            }
                        } else if (eff_attr_type != "") {
                            if (eff_attr_type == prec_param_type) {
                                found = true;
                                break;
                            }
                        } else if (prec_param_type == eff_param_type) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        same = false;
                        break;
                    }
                }

                if (same) {
                    found_task = true;
                    satisfying_task = pt;
                    break;
                }
            }
    }
    return make_pair(found_task, satisfying_task);
}

pair<set<arg_and_type>, set<arg_and_type>>
get_satisfying_knowns_and_args(task* satisfying_task,
                               set<arg_and_type>* arguments,
                               set<arg_and_type>* knowns,
                               world_state* current_state,
                               set<string>* prec_arg_types,
                               bool functional)
{
    /* Arguments:
     *          satisfying_task - The primitive task whose effect was found
     * to satisfy a failing  precondition of a failing task.
     *          arguments       - The list of argument (variable, type) of the
     * failing task.
     *          knowns          - The list of grounded argument (variable,
     * grounding) of the failing task.
     *          current_state     - The state of of all the
     * complex object including their attributes.
     *          prec_arg_types  - The failing precondition argument types
     *          functional      - Indicator variable to signify whether the
     * satisfying task is a functional method or a primitive action
     * */

    // If the satisfying task is a functional method then we first need to
    // ground the correspoding abstract task parameters since they get inherited
    // directly from the failing task. After than we can ground the other open
    // parameters based on trying to pass the method preconditions.

    map<string, bool> assigned = map<string, bool>();
    set<arg_and_type> satisfying_knowns = set<arg_and_type>();
    set<arg_and_type> satisfying_arguments = set<arg_and_type>();
    task abstract_task = task();

    if (functional) {
        // Find the corresponding abstract task of the functional method
        for (method m : methods)
            if (satisfying_task->name.substr(method_precondition_action_name.length()) == m.name)
                for (task at : abstract_tasks)
                    if (at.name == m.at) {
                        abstract_task = at;
                        break;
                    }
    }

    for (arg_and_type arg : satisfying_task->vars)
        satisfying_arguments.insert(arg);

    for (arg_and_type s_arg : satisfying_arguments) {
        // Check is the current argument is part of the abstract task
        bool s_at_arg = false;
        assigned.insert(make_pair(s_arg.first, false));
        if (functional) {
            for (arg_and_type at_arg : abstract_task.vars)
                if (s_arg.second == at_arg.second) {
                    s_at_arg = true;
                    break;
                }
        } else
            s_at_arg = true;
        if (s_at_arg) {
            // Check if the failing token's arguments fit the functional
            // method's corresponding abtract task parameter
            for (arg_and_type arg : (*arguments)) {
                if (arg.second == s_arg.second)
                    for (arg_and_type k : (*knowns))
                        if (k.first == arg.first) {
                            satisfying_knowns.insert(make_pair(s_arg.first, k.second));
                            assigned[s_arg.first] = true;
                            break;
                        }
                if (assigned[s_arg.first])
                    break;
            }
            // If not found in the failing token's arguments then it must be
            // in the failing precondition's arguments. So check them and
            // fit the functional method's corresponding abstract task
            // parameter
            if (!assigned[s_arg.first]) {
                for (string prec_type : (*prec_arg_types)) {
                    // prec_arg_types consists of <param_type>-<attr_type>
                    // if we are working with attributes of complex object
                    // or just <param_type> if we are working with the
                    // object itself
                    string prec_type_copy = prec_type, prec_param_type = prec_type,
                           prec_attr_type = "";
                    if (prec_type_copy.find("-") != string::npos) {
                        prec_param_type = prec_type_copy.substr(0, prec_type_copy.find("-"));
                        prec_type_copy.erase(0, prec_type_copy.find("-") + 1);
                        prec_attr_type = prec_type_copy.substr(0, prec_type_copy.find("-"));
                    }
                    // Here we only need to check for those cases where we
                    // are dealing with the complex objects since the other
                    // case should be handled by the previous check
                    if (prec_attr_type == s_arg.second) {
                        string ws_attr = string();
                        // Find the attribute we are looking for by going
                        // back to the sort definition of the parameter and
                        // type checking each of its attributes.
                        for (sort_definition sd : sort_definitions)
                            if (find(sd.declared_sorts.begin(),
                                     sd.declared_sorts.end(),
                                     prec_param_type) != sd.declared_sorts.end())
                                for (arg_and_type var : sd.vars.vars)
                                    if (var.second == prec_attr_type) {
                                        ws_attr = var.first.substr(1);
                                        break;
                                    }
                        for (arg_and_type arg : (*arguments)) {
                            for (arg_and_type k : (*knowns)) {
                                if (k.first == arg.first && arg.second == prec_param_type) {
                                    for (arg_and_type ws_arg_type :
                                         (*current_state)[k.second].attribute_states.vars) {
                                        if (ws_arg_type.first == ws_attr) {
                                            satisfying_knowns.insert(
                                              make_pair(s_arg.first, ws_arg_type.second));
                                            assigned[s_arg.first] = true;
                                            break;
                                        }
                                    }
                                }
                                if (assigned[s_arg.first])
                                    break;
                            }
                            if (assigned[s_arg.first])
                                break;
                        }
                    }
                    if (assigned[s_arg.first])
                        break;
                }
            }
        }
    }

    for (arg_and_type s_arg : satisfying_arguments) {
        // At this point we are dealing with a parameter that is not
        // part of the functional method's abstract task parameter list.
        // For them we need to find an assignment that makes the
        // functional method's precondition true
        //
        if (!assigned[s_arg.first]) {
            for (literal prec : satisfying_task->prec) {
                // Limit the search to those preconditions which contain the
                // current unresolved argument.
                if (find(prec.arguments.begin(), prec.arguments.end(), s_arg.first) !=
                    prec.arguments.end())
                    for (ground_literal lit : init) {
                        // Check the list of constant predicates that match
                        // the precondition signature
                        if (prec.positive == lit.positive && prec.predicate == lit.predicate &&
                            lit.timed == timed_type::ALL &&
                            lit.temp_qual == temporal_qualifier_type::OVER) {
                            bool violate = false;
                            int s_arg_pos = -1;
                            for (int i = 0; i < (int)lit.args.size(); i++) {
                                if (prec.arguments[i] != s_arg.first &&
                                    find(satisfying_knowns.begin(),
                                         satisfying_knowns.end(),
                                         make_pair(prec.arguments[i], lit.args[i])) ==
                                      satisfying_knowns.end()) {
                                    violate = true;
                                    break;
                                }
                                if (prec.arguments[i] == s_arg.first)
                                    s_arg_pos = i;
                            }
                            if (!violate) {
                                satisfying_knowns.insert(
                                  make_pair(s_arg.first, lit.args[s_arg_pos]));
                                assigned[s_arg.first] = true;
                                break;
                            }
                        }
                        if (assigned[s_arg.first])
                            break;
                    }
                if (assigned[s_arg.first])
                    break;
            }
        }
    }

    return make_pair(satisfying_knowns, satisfying_arguments);
}

bool
add_meets_constraint(Token* first,
                     Token* second,
                     STN* stn,
                     stack<STN_operation>* search_operation_history,
                     bool submit)
{
    string meets_name = first->get_end() + meets_constraint + second->get_start();
    constraint meets = make_tuple(first->get_end(), second->get_start(), zero, zero);

    if (submit) {
        assert(stn->add_constraint(first->get_end_timepoint(),
                                   second->get_start_timepoint(),
                                   meets_name,
                                   make_pair(zero, zero)));
        // assert(stn->add_constraint(meets_name, meets));
        return true;
    } else {
        bool succ = stn->add_constraint(first->get_end_timepoint(),
                                        second->get_start_timepoint(),
                                        meets_name,
                                        make_pair(zero, zero));
        if (succ && search_operation_history != nullptr)
            search_operation_history->push(STN_operation(first->get_end(),
                                                         second->get_start(),
                                                         first->get_end_timepoint(),
                                                         second->get_start_timepoint(),
                                                         STN_operation_type::ADD_CONSTRAINT,
                                                         STN_constraint_type::MEETS,
                                                         make_pair(zero, zero)));
        // bool succ = stn->add_constraint(meets_name, meets, search_history);
        return succ;
    }
}

tuple<bool, bool, bool>
del_and_add_sequencing_constraint(Token* prev,
                                  Token* curr,
                                  Token* next,
                                  STN* stn,
                                  stack<STN_operation>* search_operation_history,
                                  bool submit)
{
    string to_del = prev->get_end() + sequencing_constraint + next->get_start();
    constraint del = stn->get_constraint(to_del);
    bool succ0 =
      stn->del_constraint(prev->get_end_timepoint(), next->get_start_timepoint(), to_del);

    if (succ0)
        search_operation_history->push(STN_operation(prev->get_end(),
                                                     next->get_start(),
                                                     prev->get_end_timepoint(),
                                                     next->get_start_timepoint(),
                                                     STN_operation_type::DEL_CONSTRAINT,
                                                     STN_constraint_type::SEQUENCE,
                                                     make_pair(get<2>(del), get<3>(del))));
    // bool succ0 = stn->del_constraint(to_del, search_history);

    string c1_name = prev->get_end() + sequencing_constraint + curr->get_start();
    constraint c1 = make_tuple(prev->get_end(), curr->get_start(), zero, inf);

    string c2_name = curr->get_end() + sequencing_constraint + next->get_start();
    constraint c2 = make_tuple(curr->get_end(), next->get_start(), zero, inf);

    if (submit) {
        assert(succ0);
        assert(stn->add_constraint(
          prev->get_end_timepoint(), curr->get_start_timepoint(), c1_name, make_pair(zero, inf)));
        assert(stn->add_constraint(
          curr->get_end_timepoint(), next->get_start_timepoint(), c2_name, make_pair(zero, inf)));
        // assert(stn->add_constraint(c1_name, c1));
        // assert(stn->add_constraint(c2_name, c2));
        return make_tuple(true, true, true);

    } else {
        if (!succ0)
            return make_tuple(succ0, false, false);

        bool succ1 = stn->add_constraint(
          prev->get_end_timepoint(), curr->get_start_timepoint(), c1_name, make_pair(zero, inf));
        if (succ1 && search_operation_history != nullptr)
            search_operation_history->push(STN_operation(prev->get_end(),
                                                         curr->get_start(),
                                                         prev->get_end_timepoint(),
                                                         curr->get_start_timepoint(),
                                                         STN_operation_type::ADD_CONSTRAINT,
                                                         STN_constraint_type::SEQUENCE,
                                                         make_pair(get<2>(c1), get<3>(c1))));

        // bool succ1 = stn->add_constraint(c1_name, c1, search_history);
        if (!succ1 && get<0>(del) != "" && get<1>(del) != "" &&
            search_operation_history != nullptr) {
            assert(stn->add_constraint(prev->get_end_timepoint(),
                                       next->get_start_timepoint(),
                                       to_del,
                                       make_pair(zero, inf)));
            search_operation_history->push(STN_operation(prev->get_end(),
                                                         next->get_start(),
                                                         prev->get_end_timepoint(),
                                                         next->get_start_timepoint(),
                                                         STN_operation_type::ADD_CONSTRAINT,
                                                         STN_constraint_type::SEQUENCE,
                                                         make_pair(get<2>(del), get<3>(del))));
            // assert(stn->add_constraint(
            //   to_del, make_tuple(prev->get_end(), next->get_start(), zero, inf),
            //   search_history));
            return make_tuple(succ0, succ1, false);
        }

        bool succ2 = stn->add_constraint(
          curr->get_end_timepoint(), next->get_start_timepoint(), c2_name, make_pair(zero, inf));
        if (succ2 && search_operation_history != nullptr)
            search_operation_history->push(STN_operation(curr->get_end(),
                                                         next->get_start(),
                                                         curr->get_end_timepoint(),
                                                         next->get_start_timepoint(),
                                                         STN_operation_type::ADD_CONSTRAINT,
                                                         STN_constraint_type::SEQUENCE,
                                                         make_pair(get<2>(c2), get<3>(c2))));
        // search_operation_history->push(make_tuple(STN_operation_type::ADD_CONSTRAINT, c2,
        // c2_name));

        // bool succ2 = stn->add_constraint(c2_name, c2, search_history);
        if (!succ2 && get<0>(del) != "" && get<1>(del) != "" &&
            search_operation_history != nullptr) {
            assert(
              stn->del_constraint(prev->get_end_timepoint(), curr->get_start_timepoint(), c1_name));
            search_operation_history->push(STN_operation(prev->get_end(),
                                                         curr->get_start(),
                                                         prev->get_end_timepoint(),
                                                         curr->get_start_timepoint(),
                                                         STN_operation_type::DEL_CONSTRAINT,
                                                         STN_constraint_type::SEQUENCE,
                                                         make_pair(get<2>(c1), get<3>(c1))));
            assert(stn->add_constraint(prev->get_end_timepoint(),
                                       next->get_start_timepoint(),
                                       to_del,
                                       make_pair(zero, inf)));
            search_operation_history->push(STN_operation(prev->get_end(),
                                                         next->get_start(),
                                                         prev->get_end_timepoint(),
                                                         next->get_start_timepoint(),
                                                         STN_operation_type::ADD_CONSTRAINT,
                                                         STN_constraint_type::SEQUENCE,
                                                         make_pair(get<2>(del), get<3>(del))));
            // assert(stn->del_constraint(c1_name, search_history));
            // assert(stn->add_constraint(
            //   to_del, make_tuple(prev->get_end(), next->get_start(), zero, inf),
            //   search_history));
            return make_tuple(succ0, succ1, succ2);
        }

        return make_tuple(succ0, succ1, succ2);
    }
}

pair<bool, vector<slot>>
satisfy_precondition(literal* precondition,
                     Token* failing_tk,
                     Token* prev,
                     world_state* current_state,
                     Plan* p,
                     stack<STN_operation>* search_operation_history,
                     STN* stn,
                     vector<slot>* explored,
                     int depth)
{
    pair<bool, vector<slot>> result = pair<bool, vector<slot>>();

    set<arg_and_type> knowns = failing_tk->get_knowns();
    set<arg_and_type> arguments = failing_tk->get_arguments();
    set<string> prec_arg_types = get_literal_args_and_types(precondition, &arguments);

    pair<bool, task> potential_task = find_satisfying_task(precondition, &prec_arg_types);

    if (!potential_task.first)
        return make_pair(false, vector<slot>());

    task satisfying_task = potential_task.second;

    bool functional = false;
    for (method m : methods)
        if (satisfying_task.name.substr(method_precondition_action_name.length()) == m.name &&
            m.functional)
            functional = true;

    if (satisfying_task.artificial && !functional)
        return make_pair(false, vector<slot>());

    // Identify all the required arguments of the satisfying functional
    // method or primitive task
    pair<set<arg_and_type>, set<arg_and_type>> knowns_and_args = get_satisfying_knowns_and_args(
      &satisfying_task, &arguments, &knowns, current_state, &prec_arg_types, functional);
    set<arg_and_type> satisfying_knowns = knowns_and_args.first;
    set<arg_and_type> satisfying_arguments = knowns_and_args.second;

    if (satisfying_knowns.size() != satisfying_arguments.size())
        return make_pair(false, vector<slot>());

    // Now the satisfying task has been identified and all the required
    // arguments have also been identified. The only remaining portion is to
    // generate corresponding tokens and try to schedule them

    if (!satisfying_task.artificial && !functional) {
        bool scheduled = false;

        // Easier case where the satisfying task is a known primitive task
        Token tk = instantiate_token(satisfying_task.name,
                                     "robot",
                                     stn,
                                     failing_tk->get_request_id(),
                                     satisfying_task.duration,
                                     search_operation_history);
        tk.set_external();
        tk.add_effects(satisfying_task.eff);
        tk.add_preconditions(satisfying_task.prec);
        for (arg_and_type arg : satisfying_arguments)
            tk.add_arguments(arg);
        for (arg_and_type k : satisfying_knowns)
            tk.add_knowns(k);

        if (precondition->temp_qual == temporal_qualifier_type::AT &&
            precondition->timed == timed_type::START) {
            bool succ1 = true;
            if (prev->get_name() != "head" && prev->get_request_id() == tk.get_request_id())
                succ1 = add_meets_constraint(prev, &tk, stn, search_operation_history);
            bool succ2 = add_meets_constraint(&tk, failing_tk, stn, search_operation_history);
            if (!succ1 || !succ2) {
                stn->del_timepoint(tk.get_start_timepoint());
                search_operation_history->push(STN_operation(tk.get_start(),
                                                             string(),
                                                             tk.get_start_timepoint(),
                                                             nullptr,
                                                             STN_operation_type::DEL_TIMEPOINT,
                                                             STN_constraint_type::TIMEPOINT,
                                                             make_pair(0, 0)));
                stn->del_timepoint(tk.get_end_timepoint());
                search_operation_history->push(STN_operation(tk.get_end(),
                                                             string(),
                                                             tk.get_end_timepoint(),
                                                             nullptr,
                                                             STN_operation_type::DEL_TIMEPOINT,
                                                             STN_constraint_type::TIMEPOINT,
                                                             make_pair(0, 0)));
                // stn->del_timepoint(tk.get_start(), search_history);
                // stn->del_timepoint(tk.get_end(), search_history);
                return make_pair(scheduled, vector<slot>());
            }
        }

        pair<bool, vector<slot>> ret =
          schedule_token(&tk, explored, p, search_operation_history, stn, depth + 1);
        scheduled = ret.first;
        vector<slot> new_slots = ret.second;

        if (!scheduled) {
            stn->del_timepoint(tk.get_start_timepoint());
            search_operation_history->push(STN_operation(tk.get_start(),
                                                         string(),
                                                         tk.get_start_timepoint(),
                                                         nullptr,
                                                         STN_operation_type::DEL_TIMEPOINT,
                                                         STN_constraint_type::TIMEPOINT,
                                                         make_pair(0, 0)));
            stn->del_timepoint(tk.get_end_timepoint());
            search_operation_history->push(STN_operation(tk.get_end(),
                                                         string(),
                                                         tk.get_end_timepoint(),
                                                         nullptr,
                                                         STN_operation_type::DEL_TIMEPOINT,
                                                         STN_constraint_type::TIMEPOINT,
                                                         make_pair(0, 0)));
            // stn->del_timepoint(tk.get_start(), search_history);
            // stn->del_timepoint(tk.get_end(), search_history);
            return make_pair(scheduled, vector<slot>());
        }

        result.first = scheduled;
        result.second = new_slots;

    } else {
        method satisfying_method = method();
        for (method m : methods)
            if (m.name == satisfying_task.name.substr(method_precondition_action_name.length()))
                satisfying_method = m;

        vector<string> args = vector<string>();
        for (string arg : satisfying_method.atargs)
            for (arg_and_type sk : satisfying_knowns)
                if (arg == sk.first)
                    args.push_back(sk.second);

        vector<pair<task, var_declaration>> tasks_to_do =
          satisfying_method.func_method(args, current_state);
        if (tasks_to_do.size() == 0)
            return make_pair(false, vector<slot>());

        vector<Token> satisfying_tokens = vector<Token>();
        for (unsigned int i = 0; i < tasks_to_do.size(); i++) {
            Token tk = instantiate_token(tasks_to_do[i].first.name,
                                         "robot",
                                         stn,
                                         failing_tk->get_request_id(),
                                         tasks_to_do[i].first.duration,
                                         search_operation_history);

            tk.set_external();
            tk.add_effects(tasks_to_do[i].first.eff);
            tk.add_knowns(tasks_to_do[i].second.vars);
            tk.add_arguments(tasks_to_do[i].first.vars);
            tk.add_preconditions(tasks_to_do[i].first.prec);

            // The child tokens need to inherit the preconditions, effects
            // and arguments of the parent task as well!
            tk.add_arguments(satisfying_task.vars);
            for (arg_and_type k : satisfying_knowns)
                tk.add_knowns(k);

            // AT START preconditions and effects need to be added only to
            // the first token in the output chain AT END preconditions and
            // effects need to be added only to the last token in the output
            // chain OVER ALL preconditions and effects need to be added to
            // all the tokens in the output chain
            for (literal prec : satisfying_task.prec) {
                if (prec.timed == timed_type::START && i == 0)
                    tk.add_preconditions(prec);
                else if (prec.timed == timed_type::ALL)
                    tk.add_preconditions(prec);
                else if (prec.timed == timed_type::END && i == tasks_to_do.size() - 1)
                    // This case should not happen but adding just for
                    // completeness
                    tk.add_preconditions(prec);
            }
            for (literal eff : satisfying_task.eff) {
                if (eff.timed == timed_type::START && i == 0)
                    // This case should not happen but adding just for
                    // completeness
                    tk.add_effects(eff);
                else if (eff.timed == timed_type::ALL)
                    tk.add_effects(eff);
                else if (eff.timed == timed_type::END && i == tasks_to_do.size() - 1)
                    tk.add_effects(eff);
            }

            satisfying_tokens.push_back(tk);
        }

        for (auto it = satisfying_tokens.begin(); it != satisfying_tokens.end() - 1; it++) {
            Token curr = *it;
            Token next = *(it + 1);
            assert(add_meets_constraint(&curr, &next, stn, search_operation_history));
        }

        if (precondition->temp_qual == temporal_qualifier_type::AT &&
            precondition->timed == timed_type::START) {

            string failing_timeline = "", new_timeline = "";
            for (arg_and_type a : failing_tk->get_arguments())
                if (a.second == "robot")
                    for (arg_and_type k : failing_tk->get_knowns())
                        if (a.first == k.first)
                            failing_timeline = k.second;

            for (arg_and_type var : tasks_to_do[0].first.vars)
                if (var.second == "robot")
                    for (arg_and_type k : tasks_to_do[0].second.vars)
                        if (var.first == k.first)
                            new_timeline = k.second;
            // If the failing timeline and the procedural tokens belong to the same timeline then
            // you can connect them in a chain
            if (failing_timeline == new_timeline) {
                bool succ1 = true;
                if (prev->get_request_id() == satisfying_tokens[0].get_request_id()) {
                    PLOGD << "Pushing meets between " << prev->get_end() << " and first tk\n";
                }
                if (prev->get_request_id() == satisfying_tokens[0].get_request_id())
                    succ1 = add_meets_constraint(
                      prev, &satisfying_tokens[0], stn, search_operation_history);

                bool succ2 = add_meets_constraint(&satisfying_tokens[satisfying_tokens.size() - 1],
                                                  failing_tk,
                                                  stn,
                                                  search_operation_history);

                if (!succ1 || !succ2) {
                    for (Token tk : satisfying_tokens) {
                        stn->del_timepoint(tk.get_start_timepoint());
                        search_operation_history->push(
                          STN_operation(tk.get_start(),
                                        string(),
                                        tk.get_start_timepoint(),
                                        nullptr,
                                        STN_operation_type::DEL_TIMEPOINT,
                                        STN_constraint_type::TIMEPOINT,
                                        make_pair(0, 0)));
                        stn->del_timepoint(tk.get_end_timepoint());
                        search_operation_history->push(
                          STN_operation(tk.get_end(),
                                        string(),
                                        tk.get_end_timepoint(),
                                        nullptr,
                                        STN_operation_type::DEL_TIMEPOINT,
                                        STN_constraint_type::TIMEPOINT,
                                        make_pair(0, 0)));
                        // stn->del_timepoint(tk.get_start(), search_history);
                        // stn->del_timepoint(tk.get_end(), search_history);
                    }
                    return make_pair(false, vector<slot>());
                }
            } else {
                // If the timeline of the procedural tokens and the failing token belong to separate
                // timelines then we need to connect the last end-timepoint of the procedural tokens
                // to the start-timepoint of the failing token to ensure that the effect of the
                // procedural tokens gets reflected when we check the preconditions of the failing
                // token while validating the plan
                bool succ = add_meets_constraint(&satisfying_tokens[satisfying_tokens.size() - 1],
                                                 failing_tk,
                                                 stn,
                                                 search_operation_history);
                if (!succ) {
                    for (Token tk : satisfying_tokens) {
                        stn->del_timepoint(tk.get_start_timepoint());
                        search_operation_history->push(
                          STN_operation(tk.get_start(),
                                        string(),
                                        tk.get_start_timepoint(),
                                        nullptr,
                                        STN_operation_type::DEL_TIMEPOINT,
                                        STN_constraint_type::TIMEPOINT,
                                        make_pair(0, 0)));
                        stn->del_timepoint(tk.get_end_timepoint());
                        search_operation_history->push(
                          STN_operation(tk.get_end(),
                                        string(),
                                        tk.get_end_timepoint(),
                                        nullptr,
                                        STN_operation_type::DEL_TIMEPOINT,
                                        STN_constraint_type::TIMEPOINT,
                                        make_pair(0, 0)));
                        // stn->del_timepoint(tk.get_start(), search_history);
                        // stn->del_timepoint(tk.get_end(), search_history);
                    }
                    return make_pair(false, vector<slot>());
                }
            }
        }

        vector<slot> satisfying_tokens_slots = vector<slot>();
        for (Token tk : satisfying_tokens) {
            pair<bool, vector<slot>> ret =
              schedule_token(&tk, explored, p, search_operation_history, stn, depth + 1);
            bool scheduled = ret.first;
            vector<slot> new_slots = ret.second;

            if (!scheduled) {
                for (Token tk : satisfying_tokens) {
                    stn->del_timepoint(tk.get_start_timepoint());
                    search_operation_history->push(STN_operation(tk.get_start(),
                                                                 string(),
                                                                 tk.get_start_timepoint(),
                                                                 nullptr,
                                                                 STN_operation_type::DEL_TIMEPOINT,
                                                                 STN_constraint_type::TIMEPOINT,
                                                                 make_pair(0, 0)));
                    stn->del_timepoint(tk.get_end_timepoint());
                    search_operation_history->push(STN_operation(tk.get_end(),
                                                                 string(),
                                                                 tk.get_end_timepoint(),
                                                                 nullptr,
                                                                 STN_operation_type::DEL_TIMEPOINT,
                                                                 STN_constraint_type::TIMEPOINT,
                                                                 make_pair(0, 0)));
                    // stn->del_timepoint(tk.get_start(), search_history);
                    // stn->del_timepoint(tk.get_end(), search_history);
                }

                return make_pair(scheduled, vector<slot>());
            }

            satisfying_tokens_slots.insert(
              satisfying_tokens_slots.end(), new_slots.begin(), new_slots.end());
        }

        result.first = true;
        result.second = satisfying_tokens_slots;
    }

    return result;
}

bool
check_temporal_bounds(slot* to_explore, STN* stn)
{
    stn_bounds curr_start_bounds = stn->get_feasible_values(to_explore->tk.get_start_timepoint());
    stn_bounds next_start_bounds = stn->get_feasible_values(to_explore->next.get_start_timepoint());

    stn_bounds curr_end_bounds = stn->get_feasible_values(to_explore->tk.get_end_timepoint());
    stn_bounds prev_end_bounds = stn->get_feasible_values(to_explore->prev.get_end_timepoint());

    // stn_bounds curr_start_bounds = stn->get_feasible_values(to_explore->tk.get_start());
    // stn_bounds next_start_bounds = stn->get_feasible_values(to_explore->next.get_start());

    // stn_bounds curr_end_bounds = stn->get_feasible_values(to_explore->tk.get_end());
    // stn_bounds prev_end_bounds = stn->get_feasible_values(to_explore->prev.get_end());

    // Add a check to rule out slots where a meets constraint exists
    // between the previous and next token.
    constraint meets = stn->get_constraint(to_explore->prev.get_end() + meets_constraint +
                                           to_explore->next.get_start());
    if (get<0>(meets) != "" && get<1>(meets) != "")
        return false;

    if (to_explore->prev.get_name() != "head")
        if (abs(get<0>(prev_end_bounds)) > abs(get<1>(curr_start_bounds)))
            return false;

    if (to_explore->next.get_name() != "tail")
        if (abs(get<0>(curr_end_bounds)) > abs(get<1>(next_start_bounds)))
            return false;

    constraint duration = stn->get_constraint(to_explore->tk.get_start() + duration_constraint +
                                              to_explore->tk.get_end());

    assert(get<0>(duration) != "");
    if (to_explore->prev.get_name() != "head" && to_explore->next.get_name() != "tail") {
        if (get<3>(duration) > get<1>(next_start_bounds) - abs(get<0>(prev_end_bounds)))
            return false;
    }

    return true;
}

void
initialize_token_state(Token* tk,
                       Plan* p,
                       vector<Timeline*>* robots,
                       world_state* current_state,
                       vector<arg_and_type>* other_resources,
                       vector<arg_and_type>* affecting_resources)
{
    task tk_task = task();
    for (task t : primitive_tasks)
        if (t.name == tk->get_name()) {
            tk_task = t;
            break;
        }

    for (arg_and_type arg : tk->get_arguments()) {
        if (find(tk_task.vars.begin(), tk_task.vars.end(), arg) == tk_task.vars.end())
            continue;

        if (arg.second == "robot")
            for (arg_and_type k : tk->get_knowns())
                if (k.first == arg.first)
                    robots->push_back(p->get_timelines(k.second));

        if (is_resource(arg.second)) {
            for (arg_and_type k : tk->get_knowns())
                if (k.first == arg.first) {
                    for (pair<string, object_state> is : initial_state)
                        if (is.second.parent == "robot" || (is.second.object_name == k.second))
                            current_state->insert(make_pair(is.second.object_name, is.second));
                    if (arg.second != "robot")
                        other_resources->push_back(make_pair(k.second, arg.second));
                }

            for (sort_definition sd : sort_definitions)
                for (string decl_sort : sd.declared_sorts)
                    if (decl_sort == arg.second)
                        for (arg_and_type sd_arg : sd.vars.vars)
                            if (is_resource(sd_arg.second)) {
                                arg_and_type candidate =
                                  make_pair(arg.first, sd_arg.first.substr(1));
                                if (find(affecting_resources->begin(),
                                         affecting_resources->end(),
                                         candidate) == affecting_resources->end())
                                    affecting_resources->push_back(candidate);
                            }
        }
    }
}

void
extract_other_resource_tokens(vector<arg_and_type>* other_resources,
                              Token* tk,
                              vector<Token>* other_resource_tokens,
                              vector<int>* other_resource_pos,
                              world_state* current_state,
                              vector<slot>* local_set,
                              vector<bool>* exhausted,
                              Plan* p,
                              STN* stn)
{
    for (auto const& o_r : (*other_resources) | boost::adaptors::indexed(0)) {
        Timeline* o_r_tl = p->get_timelines(o_r.value().first, o_r.value().second);
        Token o_r_tk = (*other_resource_tokens)[o_r.index()];

        // Add contains constraint between free floating
        // dependent token and the causal primitive token
        bool succ = add_meets_constraint(tk, &o_r_tk, stn);
        if (!succ) {
            (*exhausted)[o_r.index()] = true;
            break;
        } else {
            assert(stn->del_constraint(tk->get_start_timepoint(),
                                       o_r_tk.get_start_timepoint(),
                                       tk->get_start() + meets_constraint + o_r_tk.get_start()));
            // assert(stn->del_constraint(tk->get_start() + meets_constraint + o_r_tk.get_start()));
        }
        vector<Token> o_r_tokens = o_r_tl->get_tokens();
        for (auto itr = o_r_tokens.rbegin(); itr != o_r_tokens.rend(); itr++) {

            Token n = Token();
            if (itr + 1 == o_r_tokens.rend()) {
                (*exhausted)[o_r.index()] = true;
                n = o_r_tokens[0];

            } else
                n = *(itr + 1);

            slot to_exp_o_r = slot(&o_r_tk, &n, &(*itr), o_r_tl->get_id());

            if (!check_temporal_bounds(&to_exp_o_r, stn)) {
                continue;
            }

            // Update the state of the other resource based on the current token
            // we are considering
            if (current_state->find(o_r_tl->get_id()) != current_state->end()) {
                for (auto itri = o_r_tokens.begin(); itri != o_r_tokens.end(); itri++) {
                    if (*itr == *itri) {
                        break;
                    } else {
                        object_state obj = current_state->at(o_r_tl->get_id());
                        update_object_state(&obj, &(*itri));
                        (*current_state)[o_r_tl->get_id()] = obj;
                    }
                }
            }

            if ((int)local_set->size() > o_r.index() + 1 &&
                (*other_resource_pos)[o_r.index()] < itr - o_r_tokens.rbegin()) {
                (*local_set)[o_r.index() + 1] = to_exp_o_r;
                (*other_resource_pos)[o_r.index()] = itr - o_r_tokens.rbegin();
                break;

            } else if ((int)local_set->size() <= o_r.index() + 1) {
                local_set->insert(local_set->begin() + o_r.index() + 1, to_exp_o_r);
                (*other_resource_pos)[o_r.index()] = itr - o_r_tokens.rbegin();
                break;
            }
        }
    }
}

pair<bool, bool>
precondition_check_phase(Token* tk,
                         Timeline* r,
                         world_state* current_state,
                         vector<slot>* local_set,
                         vector<slot>* explored,
                         pair<bool, vector<slot>>* return_slots,
                         Plan p,
                         stack<STN_operation>* search_operation_history,
                         STN* stn,
                         int depth)
{
    bool prec_succ = true, satisfied_once = false;
    set<arg_and_type> knowns = tk->get_knowns();

    for (literal prec : tk->get_preconditions()) {
        bool succ = check_precondition(&prec, &knowns, current_state, &init);

        if (!succ) {
            if (!satisfied_once) {
                *return_slots = satisfy_precondition(&prec,
                                                     tk,
                                                     &(*local_set)[0].prev,
                                                     current_state,
                                                     &p,
                                                     search_operation_history,
                                                     stn,
                                                     explored,
                                                     depth);
                if (!return_slots->first) {
                    prec_succ = false;
                    break;

                } else {
                    satisfied_once = true;
                    for (slot s : return_slots->second) {
                        if (s.tl_id == r->get_id()) {
                            object_state rs = current_state->at(s.tl_id);
                            update_object_state(&rs, &(s.tk));
                            (*current_state)[s.tl_id] = rs;
                        }
                    }
                }

            } else {
                prec_succ = false;
                break;
            }
        }
    }

    if (!prec_succ) {
        if (satisfied_once) // Some precondition was satisfied but another
                            // precondition failed
            for (slot s : return_slots->second) {
                stn->del_timepoint(s.tk.get_start_timepoint());
                search_operation_history->push(STN_operation(s.tk.get_start(),
                                                             string(),
                                                             s.tk.get_start_timepoint(),
                                                             nullptr,
                                                             STN_operation_type::DEL_TIMEPOINT,
                                                             STN_constraint_type::TIMEPOINT,
                                                             make_pair(0, 0)));
                stn->del_timepoint(s.tk.get_end_timepoint());
                search_operation_history->push(STN_operation(s.tk.get_end(),
                                                             string(),
                                                             s.tk.get_end_timepoint(),
                                                             nullptr,
                                                             STN_operation_type::DEL_TIMEPOINT,
                                                             STN_constraint_type::TIMEPOINT,
                                                             make_pair(0, 0)));
                // stn->del_timepoint(s.tk.get_start(), search_history);
                // stn->del_timepoint(s.tk.get_end(), search_history);
            }
        satisfied_once = false;
    } else if (prec_succ && satisfied_once) {
        // Tried to satisfy this precondition and it
        // succeeded so need to address the local set

        vector<slot> newly_added_slots = return_slots->second;
        for (int i = 0; i < (int)local_set->size(); i++)
            for (int j = (int)newly_added_slots.size() - 1; j >= 0; j--)
                if ((*local_set)[i].tl_id == newly_added_slots[j].tl_id) {
                    (*local_set)[i].prev = newly_added_slots[j].tk;
                    (*local_set)[i].next = newly_added_slots[j].next;
                    break;
                }
    }

    return make_pair(prec_succ, satisfied_once);
}

bool
local_stn_check_phase(Timeline* r,
                      vector<slot>* local_set,
                      world_state* current_state,
                      bool satisfied_once,
                      pair<bool, vector<slot>>* return_slots,
                      Plan* p,
                      stack<STN_operation>* search_operation_history,
                      STN* stn)
{
    bool local_check = true;

    Token main_tk = Token();
    for (slot s : (*local_set)) {
        if (s.tk.get_resource() == "robot")
            main_tk = s.tk;

        tuple<bool, bool, bool> succ =
          del_and_add_sequencing_constraint(&s.prev, &s.tk, &s.next, stn, search_operation_history);
        if (!get<0>(succ) || !get<1>(succ) || !get<2>(succ)) {
            local_check = false;
            break;
        }

        if (s.tk.get_name() != main_tk.get_name()) {
            PLOGD << "meets between " << main_tk.get_end() << " " << s.tk.get_start() << endl;
            bool succ1 = add_meets_constraint(&main_tk, &s.tk, stn, search_operation_history);
            PLOGD << "succ1 = " << succ1 << endl;

            if (!succ1) {
                local_check = false;
                if (get<2>(succ)) {
                    constraint del = make_tuple(s.tk.get_end(), s.next.get_start(), zero, inf);
                    string to_del = s.tk.get_end() + sequencing_constraint + s.next.get_start();
                    assert(stn->del_constraint(
                      s.tk.get_end_timepoint(), s.next.get_start_timepoint(), to_del));
                    search_operation_history->push(
                      STN_operation(s.tk.get_end(),
                                    s.next.get_start(),
                                    s.tk.get_end_timepoint(),
                                    s.next.get_start_timepoint(),
                                    STN_operation_type::DEL_CONSTRAINT,
                                    STN_constraint_type::SEQUENCE,
                                    make_pair(get<2>(del), get<3>(del))));
                }
                // assert(stn->del_constraint(
                //   s.tk.get_end() + sequencing_constraint + s.next.get_end(), search_history));

                if (get<1>(succ)) {
                    constraint del = make_tuple(s.prev.get_end(), s.tk.get_start(), zero, inf);
                    string to_del = s.prev.get_end() + sequencing_constraint + s.tk.get_start();
                    assert(stn->del_constraint(
                      s.prev.get_end_timepoint(), s.tk.get_start_timepoint(), to_del));
                    search_operation_history->push(
                      STN_operation(s.prev.get_end(),
                                    s.tk.get_start(),
                                    s.prev.get_end_timepoint(),
                                    s.tk.get_start_timepoint(),
                                    STN_operation_type::DEL_CONSTRAINT,
                                    STN_constraint_type::SEQUENCE,
                                    make_pair(get<2>(del), get<3>(del))));
                }
                // assert(stn->del_constraint(
                //   s.prev.get_end() + sequencing_constraint + s.tk.get_start(), search_history));

                if (get<0>(succ)) {
                    constraint add = make_tuple(s.prev.get_end(), s.next.get_start(), zero, inf);
                    string to_add = s.prev.get_end() + sequencing_constraint + s.next.get_start();
                    assert(stn->add_constraint(s.prev.get_end_timepoint(),
                                               s.next.get_start_timepoint(),
                                               to_add,
                                               make_pair(zero, inf)));
                    search_operation_history->push(
                      STN_operation(s.prev.get_end(),
                                    s.next.get_start(),
                                    s.prev.get_end_timepoint(),
                                    s.next.get_start_timepoint(),
                                    STN_operation_type::ADD_CONSTRAINT,
                                    STN_constraint_type::SEQUENCE,
                                    make_pair(get<2>(add), get<3>(add))));
                }
                // assert(
                //   stn->add_constraint(s.prev.get_end() + sequencing_constraint +
                //   s.next.get_start(),
                //                       make_tuple(s.prev.get_end(), s.next.get_start(), zero,
                //                       inf), search_history));
                break;
            }
        }

        if (s.tk.is_external() && s.tk.get_resource() == "robot" && s.prev.is_external()) {
            // If the current robot token is external and the
            // previous robot token is external as well, then in
            // this case we need to add a meets constraint from the
            // current robot token's end timepooint to the previous
            // robot token's dependent token's end timepoint to
            // ensure that the previous robot token's dependent
            // token does not exceed its time bound. B --> s_A_occ_e
            // A -->              s_A_occ_e
            // ur5A --> s_rmB_e   s_rmA_e
            // The following edges exist at this point,
            // rmB_e --> <0,0> --> s_A_occ
            // rmA_e --> <0,0> --> s_A_occ
            // Now the new edge needs to be added,
            // rmA_e --> <0,0> --> A_occ_e
            // This will tie the s_A_occ_e token to its 20 unit time
            // bound
            set<arg_and_type> prev_token_args = s.prev.get_arguments();
            for (auto argit = prev_token_args.begin();
                 argit != prev_token_args.end() && local_check;
                 argit++)
                if (argit->second != "robot") {
                    string current_assignment = "", assignment_type = "";
                    for (arg_and_type types : (*current_state)[r->get_id()].attribute_types.vars)
                        if (types.second == argit->second)
                            assignment_type = types.first;
                    for (arg_and_type vars : (*current_state)[r->get_id()].attribute_states.vars)
                        if (vars.first == assignment_type)
                            current_assignment = vars.second;

                    Timeline* aff_res_t = p->get_timelines(current_assignment);
                    if (aff_res_t != nullptr)
                        for (Token t : aff_res_t->get_tokens()) {
                            string meets_name = s.prev.get_end() + meets_constraint + t.get_start();
                            constraint meets = stn->get_constraint(meets_name);
                            if (get<0>(meets) != "") {
                                string new_meets_name =
                                  s.tk.get_end() + meets_constraint + t.get_end();
                                constraint new_meets =
                                  make_tuple(s.tk.get_end(), t.get_end(), zero, zero);
                                bool succ = stn->add_constraint(s.tk.get_end_timepoint(),
                                                                t.get_end_timepoint(),
                                                                new_meets_name,
                                                                make_pair(zero, zero));
                                // bool succ =
                                //   stn->add_constraint(new_meets_name, new_meets, search_history);
                                if (!succ)
                                    local_check = false;
                                else
                                    search_operation_history->push(STN_operation(
                                      s.tk.get_end(),
                                      t.get_end(),
                                      s.tk.get_end_timepoint(),
                                      t.get_end_timepoint(),
                                      STN_operation_type::ADD_CONSTRAINT,
                                      STN_constraint_type::MEETS,
                                      make_pair(get<2>(new_meets), get<3>(new_meets))));

                                break;
                            }
                        }
                }
        }
    }

    if (!local_check) {
        if (satisfied_once)
            for (slot s : return_slots->second) {
                stn->del_timepoint(s.tk.get_start_timepoint());
                search_operation_history->push(STN_operation(s.tk.get_start(),
                                                             string(),
                                                             s.tk.get_start_timepoint(),
                                                             nullptr,
                                                             STN_operation_type::DEL_TIMEPOINT,
                                                             STN_constraint_type::TIMEPOINT,
                                                             make_pair(0, 0)));
                stn->del_timepoint(s.tk.get_end_timepoint());
                search_operation_history->push(STN_operation(s.tk.get_end(),
                                                             string(),
                                                             s.tk.get_end_timepoint(),
                                                             nullptr,
                                                             STN_operation_type::DEL_TIMEPOINT,
                                                             STN_constraint_type::TIMEPOINT,
                                                             make_pair(0, 0)));
                // stn->del_timepoint(s.tk.get_start(), search_history);
                // stn->del_timepoint(s.tk.get_end(), search_history);
            }
    }

    return local_check;
}

// TODO: Fix the rewiring check so that item timelines are also modified properly. Currently I just
// check for affecting resources and the previous token but if the affecting resource is not an item
// for some token then later down the line if another affecting resource becomes an item then
// checking just previous wont work so need to iterate backwards till the current request tokens to
// see if there were any existing dependent meets constraints
bool
rewiring_check_phase(slot* to_explore,
                     Token* tk,
                     Timeline* r,
                     vector<arg_and_type> affecting_resources,
                     bool satisfied_once,
                     world_state* current_state,
                     pair<bool, vector<slot>>* return_slots,
                     Plan* p,
                     stack<STN_operation>* search_operation_history,
                     STN* stn)
{
    bool rewiring_check = true;
    if (satisfied_once)
        for (slot s : return_slots->second) {
            Timeline* t = p->get_timelines(s.tl_id);
            t->insert_token(s.tk, s.prev, s.next);
        }
    // At this point we need to ensure that the tokens on
    // timelines of resources that the scheduling token is
    // affecting get updated so that they reflect the correct
    // state of the world. So for example grasp preceded by
    // rail_moves have to ensure that the occupied tokens on the
    // last rail_block extends till the time grasp ends.
    // Additionally the contains needs to be removed from the EP
    // of rail_move to the EP of grasp for the EP of
    // corresponding occupied token on rail_block

    for (arg_and_type aff_res : affecting_resources) {
        string var = aff_res.first;
        string attribute = aff_res.second;

        string attribute_assignment = string();
        for (arg_and_type vars : (*current_state)[r->get_id()].attribute_states.vars) {
            if (vars.first == attribute)
                attribute_assignment = vars.second;
        }

        Timeline* aff_res_t = p->get_timelines(attribute_assignment);
        if (aff_res_t != nullptr) {
            for (Token t : aff_res_t->get_tokens()) {
                string dep_meets_name =
                  to_explore->prev.get_end() + dependent_meets_constraint + t.get_end();
                constraint dep_meets = stn->get_constraint(dep_meets_name);
                if (get<0>(dep_meets) != "") {
                    // The case when the dependent token's end
                    // timpoint has a meets constraint between
                    // the previous token's end timepoint. In
                    // this case we need to first delete this
                    // existing constraint and then add a new
                    // meets constraint in its place from the
                    // same dependent token's end timepoint to
                    // the current token's end timepoint.

                    assert(stn->del_constraint(
                      to_explore->prev.get_end_timepoint(), t.get_end_timepoint(), dep_meets_name));

                    search_operation_history->push(
                      STN_operation(to_explore->prev.get_end(),
                                    t.get_end(),
                                    to_explore->prev.get_end_timepoint(),
                                    t.get_end_timepoint(),
                                    STN_operation_type::DEL_CONSTRAINT,
                                    STN_constraint_type::DEPENDENT_MEETS,
                                    make_pair(get<2>(dep_meets), get<3>(dep_meets))));
                    // assert(stn->del_constraint(dep_meets_name, search_history));
                    string c_name = tk->get_end() + dependent_meets_constraint + t.get_end();
                    constraint c = make_tuple(tk->get_end(), t.get_end(), zero, inf);
                    bool succ = stn->add_constraint(
                      tk->get_end_timepoint(), t.get_end_timepoint(), c_name, make_pair(zero, inf));
                    // bool succ = stn->add_constraint(c_name, c, search_history);
                    if (!succ) {
                        rewiring_check = false;
                        assert(
                          stn->add_constraint(to_explore->prev.get_end_timepoint(),
                                              t.get_end_timepoint(),
                                              dep_meets_name,
                                              make_pair(get<2>(dep_meets), get<3>(dep_meets))));
                        search_operation_history->push(
                          STN_operation(to_explore->prev.get_end(),
                                        t.get_end(),
                                        to_explore->prev.get_end_timepoint(),
                                        t.get_end_timepoint(),
                                        STN_operation_type::ADD_CONSTRAINT,
                                        STN_constraint_type::DEPENDENT_MEETS,
                                        make_pair(get<2>(dep_meets), get<3>(dep_meets))));
                        // assert(stn->add_constraint(dep_meets_name, dep_meets, search_history));
                    } else
                        search_operation_history->push(
                          STN_operation(tk->get_end(),
                                        t.get_end(),
                                        tk->get_end_timepoint(),
                                        t.get_end_timepoint(),
                                        STN_operation_type::ADD_CONSTRAINT,
                                        STN_constraint_type::DEPENDENT_MEETS,
                                        make_pair(get<2>(c), get<3>(c))));
                    break;
                }
            }
        }

        Token updated_prev = Token();
        world_state current_state_mod = *current_state;
        if (satisfied_once) {
            for (slot s : return_slots->second) {
                if (s.tl_id == r->get_id()) {
                    object_state rs = current_state_mod.at(s.tl_id);
                    update_object_state(&rs, &(s.tk));
                    current_state_mod[s.tl_id] = rs;
                    updated_prev = s.tk;
                }
            }
        }
        attribute_assignment = string();
        for (arg_and_type vars : current_state_mod[r->get_id()].attribute_states.vars) {
            if (vars.first == attribute)
                attribute_assignment = vars.second;
        }
        assert(attribute_assignment != string());
        aff_res_t = p->get_timelines(attribute_assignment);
        if (aff_res_t != nullptr)
            for (Token t : aff_res_t->get_tokens()) {
                string first_meets_name = updated_prev.get_end() + meets_constraint + t.get_start();
                constraint first_meets = stn->get_constraint(first_meets_name);
                if (get<0>(first_meets) != "") {
                    // The case when the previous token's end
                    // timepoint has a meets constraint with the
                    // dependent token's start timepoint. In this
                    // case we need to add an additional meets
                    // constraint from the dependent token's end
                    // timepoint to the current token's end
                    // timepoint.
                    string c_name = tk->get_end() + dependent_meets_constraint + t.get_end();
                    constraint c = make_tuple(tk->get_end(), t.get_end(), zero, inf);
                    bool succ = stn->add_constraint(
                      tk->get_end_timepoint(), t.get_end_timepoint(), c_name, make_pair(zero, inf));
                    // bool succ = stn->add_constraint(c_name, c, search_history);
                    if (!succ)
                        rewiring_check = false;
                    else
                        search_operation_history->push(
                          STN_operation(tk->get_end(),
                                        t.get_end(),
                                        tk->get_end_timepoint(),
                                        t.get_end_timepoint(),
                                        STN_operation_type::ADD_CONSTRAINT,
                                        STN_constraint_type::DEPENDENT_MEETS,
                                        make_pair(get<2>(c), get<3>(c))));
                    break;
                }

                string second_meets_name =
                  updated_prev.get_end() + dependent_meets_constraint + t.get_end();
                constraint second_meets = stn->get_constraint(second_meets_name);
                if (get<0>(second_meets) != "") {
                    // The case when the dependent token's end
                    // timpoint has a meets constraint between
                    // the previous token's end timepoint. In
                    // this case we need to first delete this
                    // existing constraint and then add a new
                    // meets constraint in its place from the
                    // same dependent token's end timepoint to
                    // the current token's end timepoint.

                    assert(stn->del_constraint(
                      updated_prev.get_end_timepoint(), t.get_end_timepoint(), second_meets_name));
                    search_operation_history->push(
                      STN_operation(updated_prev.get_end(),
                                    t.get_end(),
                                    updated_prev.get_end_timepoint(),
                                    t.get_end_timepoint(),
                                    STN_operation_type::DEL_CONSTRAINT,
                                    STN_constraint_type::DEPENDENT_MEETS,
                                    make_pair(get<2>(second_meets), get<3>(second_meets))));
                    // assert(stn->del_constraint(second_meets_name, search_history));
                    string c_name = tk->get_end() + dependent_meets_constraint + t.get_end();
                    constraint c = make_tuple(tk->get_end(), t.get_end(), zero, inf);
                    bool succ = stn->add_constraint(
                      tk->get_end_timepoint(), t.get_end_timepoint(), c_name, make_pair(zero, inf));
                    // bool succ = stn->add_constraint(c_name, c, search_history);
                    if (!succ) {
                        rewiring_check = false;
                        assert(stn->add_constraint(
                          updated_prev.get_end_timepoint(),
                          t.get_end_timepoint(),
                          second_meets_name,
                          make_pair(get<2>(second_meets), get<3>(second_meets))));
                        search_operation_history->push(
                          STN_operation(updated_prev.get_end(),
                                        t.get_end(),
                                        updated_prev.get_end_timepoint(),
                                        t.get_end_timepoint(),
                                        STN_operation_type::ADD_CONSTRAINT,
                                        STN_constraint_type::DEPENDENT_MEETS,
                                        make_pair(get<2>(second_meets), get<3>(second_meets))));
                        // assert(
                        //   stn->add_constraint(second_meets_name, second_meets, search_history));
                    } else
                        search_operation_history->push(
                          STN_operation(tk->get_end(),
                                        t.get_end(),
                                        tk->get_end_timepoint(),
                                        t.get_end_timepoint(),
                                        STN_operation_type::ADD_CONSTRAINT,
                                        STN_constraint_type::DEPENDENT_MEETS,
                                        make_pair(get<2>(c), get<3>(c))));
                    break;
                }
            }
    }

    if (!rewiring_check)
        if (satisfied_once) {
            for (slot s : return_slots->second) {
                Timeline* t = p->get_timelines(s.tl_id);
                vector<Token> t_tks = t->get_tokens();
                for (int i = 0; i < (int)t_tks.size(); i++)
                    if (t_tks[i] == s.tk) {
                        t->del_token(i);
                        break;
                    }
            }
            for (slot s : return_slots->second) {

                stn->del_timepoint(s.tk.get_start_timepoint());
                search_operation_history->push(STN_operation(s.tk.get_start(),
                                                             string(),
                                                             s.tk.get_start_timepoint(),
                                                             nullptr,
                                                             STN_operation_type::DEL_TIMEPOINT,
                                                             STN_constraint_type::TIMEPOINT,
                                                             make_pair(0, 0)));
                stn->del_timepoint(s.tk.get_end_timepoint());
                search_operation_history->push(STN_operation(s.tk.get_end(),
                                                             string(),
                                                             s.tk.get_end_timepoint(),
                                                             nullptr,
                                                             STN_operation_type::DEL_TIMEPOINT,
                                                             STN_constraint_type::TIMEPOINT,
                                                             make_pair(0, 0)));
                // stn->del_timepoint(s.tk.get_start(), search_history);
                // stn->del_timepoint(s.tk.get_end(), search_history);
            }
        }

    return rewiring_check;
}

pair<bool, vector<slot>>
schedule_token(Token* tk,
               vector<slot>* explored,
               Plan* p,
               stack<STN_operation>* search_operation_history,
               STN* stn,
               int depth)
{

    // If we exceed the limit of recursion then we need to stop immediately
    if (depth > max_recursive_depth)
        return make_pair(false, vector<slot>());

    bool scheduled = false;
    vector<slot> local_set = vector<slot>();
    vector<Timeline*> robots = vector<Timeline*>();

    world_state current_state = map<string, object_state>();
    vector<arg_and_type> other_resources = vector<arg_and_type>();
    vector<arg_and_type> affecting_resources = vector<arg_and_type>();

    initialize_token_state(tk, p, &robots, &current_state, &other_resources, &affecting_resources);

    vector<Token> other_resource_tokens = vector<Token>();
    for (arg_and_type arg : other_resources)
        other_resource_tokens.push_back(gen_token(arg, tk, search_operation_history, stn));

    assert(robots.size() > 0);
    assert(robots.size() == 1);

    for (Timeline* r : robots) {
        vector<Token> r_tokens = r->get_tokens();

        for (auto it = r_tokens.rbegin(); it != r_tokens.rend() - 1; it++) {
            local_set = vector<slot>();

            bool skip = false;
            slot to_explore = slot(tk, &(*(it + 1)), &(*it), r->get_id());
            for (slot entry : (*explored))
                if (entry == to_explore) {
                    skip = true;
                    break;
                }

            if (skip)
                continue;

            if (!check_temporal_bounds(&to_explore, stn))
                continue;

            local_set.push_back(to_explore);
            explored->push_back(to_explore);

            for (auto iti = r_tokens.begin(); iti != r_tokens.end(); iti++) {
                if (*it == *iti) {
                    break;
                } else {
                    object_state obj = current_state.at(r->get_id());
                    update_object_state(&obj, &(*iti));
                    current_state[r->get_id()] = obj;
                }
            }

            vector<bool> exhausted(other_resources.size(), false);
            vector<int> other_resource_pos = vector<int>(other_resources.size(), 0);
            while (!scheduled) {

                unordered_map<string, constraint> pre_stn_constraints = stn->get_constraints();
                world_state current_state_copy = current_state;

                extract_other_resource_tokens(&other_resources,
                                              tk,
                                              &other_resource_tokens,
                                              &other_resource_pos,
                                              &current_state_copy,
                                              &local_set,
                                              &exhausted,
                                              p,
                                              stn);

                if (find(exhausted.begin(), exhausted.end(), true) != exhausted.end()) {
                    break;
                }

                double eft = 0.0;
                for (slot s : local_set) {
                    if (s.prev.get_name() == "head" || s.prev.get_name() == "tail")
                        continue;
                    stn_bounds prev_end_bounds =
                      stn->get_feasible_values(s.prev.get_end_timepoint());
                    // stn_bounds prev_end_bounds = stn->get_feasible_values(s.prev.get_end());
                    if (eft <= abs(get<0>(prev_end_bounds)))
                        eft = abs(get<0>(prev_end_bounds));
                }

                for (pair<string, object_state> ws : current_state_copy) {
                    if (ws.first != r->get_id() && ws.second.parent == "robot") {
                        Timeline* o_robot_tl = p->get_timelines(ws.first);
                        for (Token o_robot_tk : o_robot_tl->get_tokens()) {
                            if (o_robot_tk.get_name() == "head")
                                continue;
                            stn_bounds o_robot_start =
                              stn->get_feasible_values(o_robot_tk.get_start_timepoint());
                            stn_bounds o_robot_end =
                              stn->get_feasible_values(o_robot_tk.get_end_timepoint());
                            // stn_bounds o_robot_start =
                            //   stn->get_feasible_values(o_robot_tk.get_start());
                            // stn_bounds o_robot_end =
                            // stn->get_feasible_values(o_robot_tk.get_end());
                            if (get<1>(o_robot_end) > eft && abs(get<0>(o_robot_start)) > eft)
                                break;
                            update_object_state(&ws.second, &o_robot_tk);
                            current_state_copy[ws.first] = ws.second;
                        }
                    }
                }

                vector<slot> local_set_copy = local_set;

                // Check the preconditions and try to satisfy them
                pair<bool, vector<slot>> return_slots = pair<bool, vector<slot>>();
                pair<bool, bool> prec_check = precondition_check_phase(tk,
                                                                       r,
                                                                       &current_state_copy,
                                                                       &local_set,
                                                                       explored,
                                                                       &return_slots,
                                                                       *p,
                                                                       search_operation_history,
                                                                       stn,
                                                                       depth);
                bool prec_succ = get<0>(prec_check), satisfied_once = get<1>(prec_check);

                if (!prec_succ && other_resources.size() == 0)
                    break;
                else if (!prec_succ) {
                    satisfied_once = false;
                    current_state_copy = current_state;
                    continue;
                }

                // Check the stn connections
                bool local_check = local_stn_check_phase(r,
                                                         &local_set,
                                                         &current_state_copy,
                                                         satisfied_once,
                                                         &return_slots,
                                                         p,
                                                         search_operation_history,
                                                         stn);

                if (!local_check && other_resources.size() == 0)
                    break;
                else if (!local_check) {
                    satisfied_once = false;
                    local_set = local_set_copy;
                    current_state_copy = current_state;
                    continue;
                }

                // Check the stn connections with external tokens
                bool rewiring_check = true;
                if (local_check && prec_succ)
                    rewiring_check = rewiring_check_phase(&to_explore,
                                                          tk,
                                                          r,
                                                          affecting_resources,
                                                          satisfied_once,
                                                          &current_state,
                                                          &return_slots,
                                                          p,
                                                          search_operation_history,
                                                          stn);

                if (!rewiring_check && other_resources.size() == 0)
                    break;
                else if (!rewiring_check) {
                    satisfied_once = false;
                    local_set = local_set_copy;
                    current_state_copy = current_state;
                    continue;
                }

                if (local_check && prec_succ && rewiring_check) {
                    for (slot s : local_set) {
                        Timeline* t = p->get_timelines(s.tl_id);
                        t->insert_token(s.tk, s.prev, s.next);
                    }

                    if (satisfied_once)
                        local_set.insert(local_set.begin(),
                                         return_slots.second.begin(),
                                         return_slots.second.end());

                    scheduled = true;
                }

                if (!scheduled) {
                    unordered_map<string, constraint> post_stn_constraints = stn->get_constraints();

                    for (pair<string, constraint> post : post_stn_constraints)
                        if (pre_stn_constraints.find(post.first) == pre_stn_constraints.end()) {
                            assert(stn->del_constraint(post.first));
                            // TODO: Check if i still need this?
                            STN_constraint_type c_type = STN_constraint_type::TIMEPOINT;
                            if (strstr(post.first.c_str(), cz_constraint.c_str()))
                                c_type = STN_constraint_type::CZ;
                            if (strstr(post.first.c_str(), meets_constraint.c_str()))
                                c_type = STN_constraint_type::MEETS;
                            else if (strstr(post.first.c_str(), before_constraint.c_str()))
                                c_type = STN_constraint_type::BEFORE;
                            else if (strstr(post.first.c_str(), duration_constraint.c_str()))
                                c_type = STN_constraint_type::DURATION;
                            else if (strstr(post.first.c_str(), contains_constraint.c_str()))
                                c_type = STN_constraint_type::CONTAINS;
                            else if (strstr(post.first.c_str(), sequencing_constraint.c_str()))
                                c_type = STN_constraint_type::SEQUENCE;
                            else if (strstr(post.first.c_str(), dependent_meets_constraint.c_str()))
                                c_type = STN_constraint_type::DEPENDENT_MEETS;
                            assert(c_type != STN_constraint_type::TIMEPOINT);
                            search_operation_history->push(
                              STN_operation(get<0>(post.second),
                                            get<1>(post.second),
                                            nullptr,
                                            nullptr,
                                            STN_operation_type::DEL_CONSTRAINT,
                                            c_type,
                                            make_pair(get<2>(post.second), get<3>(post.second))));
                        }
                    for (pair<string, constraint> pre : pre_stn_constraints)
                        if (post_stn_constraints.find(pre.first) == post_stn_constraints.end()) {
                            assert(stn->add_constraint(pre.first, pre.second));
                            // TODO: Check if i still need this?
                            STN_constraint_type c_type = STN_constraint_type::TIMEPOINT;
                            if (strstr(pre.first.c_str(), cz_constraint.c_str()))
                                c_type = STN_constraint_type::CZ;
                            if (strstr(pre.first.c_str(), meets_constraint.c_str()))
                                c_type = STN_constraint_type::MEETS;
                            else if (strstr(pre.first.c_str(), before_constraint.c_str()))
                                c_type = STN_constraint_type::BEFORE;
                            else if (strstr(pre.first.c_str(), duration_constraint.c_str()))
                                c_type = STN_constraint_type::DURATION;
                            else if (strstr(pre.first.c_str(), contains_constraint.c_str()))
                                c_type = STN_constraint_type::CONTAINS;
                            else if (strstr(pre.first.c_str(), sequencing_constraint.c_str()))
                                c_type = STN_constraint_type::SEQUENCE;
                            else if (strstr(pre.first.c_str(), dependent_meets_constraint.c_str()))
                                c_type = STN_constraint_type::DEPENDENT_MEETS;
                            assert(c_type != STN_constraint_type::TIMEPOINT);
                            search_operation_history->push(
                              STN_operation(get<0>(pre.second),
                                            get<1>(pre.second),
                                            nullptr,
                                            nullptr,
                                            STN_operation_type::ADD_CONSTRAINT,
                                            c_type,
                                            make_pair(get<2>(pre.second), get<3>(pre.second))));
                        }

                    stn->cleanup();
                }
            }

            if (scheduled)
                break;
        }

        if (!scheduled) {
            if (other_resource_tokens.size() != 0)
                for (Token tk : other_resource_tokens) {
                    stn->del_timepoint(tk.get_start_timepoint());
                    search_operation_history->push(STN_operation(tk.get_start(),
                                                                 string(),
                                                                 tk.get_start_timepoint(),
                                                                 nullptr,
                                                                 STN_operation_type::DEL_TIMEPOINT,
                                                                 STN_constraint_type::TIMEPOINT,
                                                                 make_pair(0, 0)));
                    stn->del_timepoint(tk.get_end_timepoint());
                    search_operation_history->push(STN_operation(tk.get_end(),
                                                                 string(),
                                                                 tk.get_end_timepoint(),
                                                                 nullptr,
                                                                 STN_operation_type::DEL_TIMEPOINT,
                                                                 STN_constraint_type::TIMEPOINT,
                                                                 make_pair(0, 0)));
                    // stn->del_timepoint(tk.get_start(), search_history);
                    // stn->del_timepoint(tk.get_end(), search_history);
                }
        }
    }

    return make_pair(scheduled, local_set);
}

bool
schedule_leafs(vector<task_vertex> leafs,
               vector<primitive_solution>* solution,
               Plan p,
               vector<slot>* explored_slots,
               double* value,
               string metric)
{
    int leaf_id = 0, num_actions = 0;
    for (task_vertex leaf : leafs) {
        primitive_solution leaf_sol = primitive_solution();
        leaf_sol.primitive_token = leaf.tk;
        pair<bool, vector<slot>> res =
          schedule_token(&leaf.tk, explored_slots, &p, &leaf_sol.search_operations, &stn, 0);
        bool scheduled = res.first;
        if (scheduled) {
            for (slot s : res.second) {
                leaf_sol.token_slots.push_back(s);
                if (s.tk.get_resource() == "robot")
                    num_actions++;
            }
        }

        leaf_id++;
        solution->push_back(leaf_sol);
        if (!scheduled) {
            for (int i = 0; i < leaf_id; i++)
                for (slot s : (*solution)[i].token_slots) {
                    if (s.tk.get_start() == leafs[i].tk.get_start())
                        continue;
                    stn.del_timepoint(s.tk.get_start_timepoint());
                    stn.del_timepoint(s.tk.get_end_timepoint());
                    leaf_sol.search_operations.push(STN_operation(s.tk.get_start(),
                                                                  string(),
                                                                  s.tk.get_start_timepoint(),
                                                                  nullptr,
                                                                  STN_operation_type::DEL_TIMEPOINT,
                                                                  STN_constraint_type::TIMEPOINT,
                                                                  make_pair(0, 0)));
                    leaf_sol.search_operations.push(STN_operation(s.tk.get_end(),
                                                                  string(),
                                                                  s.tk.get_end_timepoint(),
                                                                  nullptr,
                                                                  STN_operation_type::DEL_TIMEPOINT,
                                                                  STN_constraint_type::TIMEPOINT,
                                                                  make_pair(0, 0)));
                    // stn.del_timepoint(s.tk.get_start(), &leaf_sol.search_constraints);
                    // stn.del_timepoint(s.tk.get_end(), &leaf_sol.search_constraints);
                }
            *value = std::numeric_limits<double>::infinity();

            return false;
        }
    }

    vector<tuple<int, int, slot>> main = vector<tuple<int, int, slot>>();
    vector<tuple<int, int, slot>> dependents = vector<tuple<int, int, slot>>();

    for (int i = 0; i < (int)solution->size(); i++)
        for (int j = 0; j < (int)(*solution)[i].token_slots.size(); j++)
            if ((*solution)[i].token_slots[j].tk.get_resource() == "robot")
                main.push_back(make_tuple(i, j, (*solution)[i].token_slots[j]));
            else
                dependents.push_back(make_tuple(i, j, (*solution)[i].token_slots[j]));

    unordered_map<string, constraint> post_stn_constraints = stn.get_constraints();
    for (tuple<int, int, slot> os : main) {
        string dep_meets_substr = get<2>(os).tk.get_end() + dependent_meets_constraint;
        for (pair<string, constraint> post_stn_c : post_stn_constraints)
            if (strstr(post_stn_c.first.c_str(), dep_meets_substr.c_str())) {
                (*solution)[get<0>(os)].token_slots[get<1>(os)].dependent_ends.push_back(
                  get<1>(post_stn_c.second));
            }
    }

    // Once a feasible solution has been found for the given task then we need to check that it can
    // be validated which if failed needs to be recitifed via the patching process
    validation_state state = plan_validator(&p);
    if (state.status != validation_exception::NO_EXCEPTION) {
        bool success = patch_plan(
          &p, &request_solutions[state.failing_token.get_request_id()], solution, &state);
        if (!success) {
            PLOGE << "Patching failed! Please refer to the following exception state: "
                  << state.to_string() << endl;
            assert(false);
        }
    }

    double best_metric = -1.0 * std::numeric_limits<double>::infinity();
    if (metric == "makespan") {
        best_metric = compute_makespan(&p);
    } else if (metric == "actions") {
        best_metric = num_actions;
    }
    *value = best_metric;

    return true;
}

void
commit_stn_operations(vector<slot>* leaf_slots,
                      stack<STN_operation>* commit_operations,
                      STN* stn,
                      vector<shared_ptr<Node>>* newly_added_tps)
{

    while (!commit_operations->empty()) {
        STN_operation operation = commit_operations->top();
        string tp1 = operation.tp1, tp2 = operation.tp2;
        shared_ptr<Node> tp1_node = operation.tp1_node, tp2_node = operation.tp2_node;
        STN_operation_type op_type = operation.op_type;
        STN_constraint_type c_type = operation.c_type;
        pair<double, double> bounds = operation.bounds;
        if (op_type == STN_operation_type::ADD_TIMEPOINT) {
            PLOGD << "Performing ADD_TIMEPOINT operation: " << tp1 << endl;
            shared_ptr<Node> tp = stn->add_timepoint(tp1);
            assert(tp != nullptr);
            bool stop = false;
            for (auto it = leaf_slots->begin(); it != leaf_slots->end(); it++) {
                if (it->tk.get_start() == tp1) {
                    it->tk.set_start_timepoint(tp);
                    stop = true;
                } else if (it->tk.get_end() == tp1) {
                    it->tk.set_end_timepoint(tp);
                    stop = true;
                }
                if (stop)
                    break;
            }
            newly_added_tps->push_back(tp);
        } else if (op_type == STN_operation_type::ADD_CONSTRAINT) {

            string c_name = string();
            if (c_type == STN_constraint_type::BEFORE)
                c_name = tp1 + before_constraint + tp2;
            else if (c_type == STN_constraint_type::CONTAINS)
                c_name = tp1 + contains_constraint + tp2;
            else if (c_type == STN_constraint_type::DURATION)
                c_name = tp1 + duration_constraint + tp2;
            else if (c_type == STN_constraint_type::MEETS)
                c_name = tp1 + meets_constraint + tp2;
            else if (c_type == STN_constraint_type::SEQUENCE)
                c_name = tp1 + sequencing_constraint + tp2;
            else if (c_type == STN_constraint_type::DEPENDENT_MEETS)
                c_name = tp1 + dependent_meets_constraint + tp2;

            assert(c_name != string());

            PLOGD << "Performing ADD_CONSTRAINT operation: " << tp1 << ", " << tp2 << ", " << c_name
                  << endl;

            for (shared_ptr<Node> tps : *newly_added_tps) {
                if (tps != nullptr && tps->timepoint_id == tp1)
                    tp1_node = tps;
                if (tps != nullptr && tps->timepoint_id == tp2)
                    tp2_node = tps;
            }

            for (slot s : *leaf_slots) {
                if (tp1_node == nullptr) {
                    if (s.tk.get_start() == tp1)
                        tp1_node = s.tk.get_start_timepoint();
                    else if (s.tk.get_end() == tp1)
                        tp1_node = s.tk.get_end_timepoint();
                    else if (s.prev.get_start() == tp1)
                        tp1_node = s.prev.get_start_timepoint();
                    else if (s.prev.get_end() == tp1)
                        tp1_node = s.prev.get_end_timepoint();
                    else if (s.next.get_start() == tp1)
                        tp1_node = s.next.get_start_timepoint();
                    else if (s.next.get_end() == tp1)
                        tp1_node = s.next.get_end_timepoint();
                }

                if (tp2_node == nullptr) {
                    if (s.tk.get_start() == tp2)
                        tp2_node = s.tk.get_start_timepoint();
                    else if (s.tk.get_end() == tp2)
                        tp2_node = s.tk.get_end_timepoint();
                    else if (s.prev.get_start() == tp2)
                        tp2_node = s.prev.get_start_timepoint();
                    else if (s.prev.get_end() == tp2)
                        tp2_node = s.prev.get_end_timepoint();
                    else if (s.next.get_start() == tp2)
                        tp2_node = s.next.get_start_timepoint();
                    else if (s.next.get_end() == tp2)
                        tp2_node = s.next.get_end_timepoint();
                }
            }

            assert(tp1_node != nullptr);
            assert(tp2_node != nullptr);
            assert(stn->add_constraint(tp1_node, tp2_node, c_name, bounds));
        } else if (op_type == STN_operation_type::DEL_TIMEPOINT) {
            PLOGD << "Performing DEL_TIMEPOINT operation: " << tp1 << endl;
            // shared_ptr<Node> del_node = nullptr;
            // for (auto it = newly_added_tps->begin(); it != newly_added_tps->end(); it++)
            //     if ((*it)->timepoint_id == tp1) {
            //         del_node = *it;
            //         *it = nullptr;
            //         break;
            //     }
            // assert(del_node != nullptr);
            assert(tp1_node != nullptr);
            stn->del_timepoint(tp1_node);
        } else if (op_type == STN_operation_type::DEL_CONSTRAINT) {

            string c_name = string();
            if (c_type == STN_constraint_type::BEFORE)
                c_name = tp1 + before_constraint + tp2;
            else if (c_type == STN_constraint_type::CONTAINS)
                c_name = tp1 + contains_constraint + tp2;
            else if (c_type == STN_constraint_type::DURATION)
                c_name = tp1 + duration_constraint + tp2;
            else if (c_type == STN_constraint_type::MEETS)
                c_name = tp1 + meets_constraint + tp2;
            else if (c_type == STN_constraint_type::SEQUENCE)
                c_name = tp1 + sequencing_constraint + tp2;
            else if (c_type == STN_constraint_type::DEPENDENT_MEETS)
                c_name = tp1 + dependent_meets_constraint + tp2;

            assert(c_name != string());

            PLOGD << "Performing DEL_CONSTRAINT operation: " << tp1 << ", " << tp2 << ", " << c_name
                  << endl;

            for (shared_ptr<Node> tps : *newly_added_tps) {
                if (tps != nullptr && tps->timepoint_id == tp1)
                    tp1_node = tps;
                if (tps != nullptr && tps->timepoint_id == tp2)
                    tp2_node = tps;
            }

            for (slot s : *leaf_slots) {
                if (tp1_node == nullptr) {
                    if (s.tk.get_start() == tp1)
                        tp1_node = s.tk.get_start_timepoint();
                    else if (s.tk.get_end() == tp1)
                        tp1_node = s.tk.get_end_timepoint();
                    else if (s.prev.get_start() == tp1)
                        tp1_node = s.prev.get_start_timepoint();
                    else if (s.prev.get_end() == tp1)
                        tp1_node = s.prev.get_end_timepoint();
                    else if (s.next.get_start() == tp1)
                        tp1_node = s.next.get_start_timepoint();
                    else if (s.next.get_end() == tp1)
                        tp1_node = s.next.get_end_timepoint();
                }

                if (tp2_node == nullptr) {
                    if (s.tk.get_start() == tp2)
                        tp2_node = s.tk.get_start_timepoint();
                    else if (s.tk.get_end() == tp2)
                        tp2_node = s.tk.get_end_timepoint();
                    else if (s.prev.get_start() == tp2)
                        tp2_node = s.prev.get_start_timepoint();
                    else if (s.prev.get_end() == tp2)
                        tp2_node = s.prev.get_end_timepoint();
                    else if (s.next.get_start() == tp2)
                        tp2_node = s.next.get_start_timepoint();
                    else if (s.next.get_end() == tp2)
                        tp2_node = s.next.get_end_timepoint();
                }
            }

            assert(tp1_node != nullptr);
            assert(tp2_node != nullptr);
            assert(stn->del_constraint(tp1_node, tp2_node, c_name));
        }
        commit_operations->pop();
    }
}

// TODO: Change this function to use search operation stack for STN related stuff and slots for plan
// related stuff
void
commit_slots(Plan* p, pq* solution)
{

    tasknetwork_solution slot_to_commit = solution->top();
    request_solutions.insert(make_pair(slot_to_commit.request_id, slot_to_commit));
    p->num_tasks_robot[slot_to_commit.robot_assignment]++;

    vector<shared_ptr<Node>> newly_added_tps;

    int counter = 0;
    for (int i = 0; i < (int)slot_to_commit.solution.size(); i++) {
        primitive_solution leaf_solution = slot_to_commit.solution[i];
        vector<slot> leaf_slots = leaf_solution.token_slots;
        assert(leaf_slots.size() > 0);

        stack<STN_operation> search_operations = leaf_solution.search_operations;
        stack<STN_operation> commit_operations;
        while (!search_operations.empty()) {
            STN_operation top = search_operations.top();
            if (top.op_type == STN_operation_type::ADD_TIMEPOINT)
                cout << "ADD_TIMEPOINT: " << top.tp1 << endl;
            else if (top.op_type == STN_operation_type::DEL_TIMEPOINT)
                cout << "DEL_TIMEPOINT: " << top.tp1 << endl;
            else if (top.op_type == STN_operation_type::ADD_CONSTRAINT)
                cout << "ADD_CONSTRAINT: " << top.tp1 << ", " << top.tp2 << ", " << top.c_type
                     << endl;
            else if (top.op_type == STN_operation_type::DEL_CONSTRAINT)
                cout << "DEL_CONSTRAINT: " << top.tp1 << ", " << top.tp2 << ", " << top.c_type
                     << endl;
            search_operations.pop();
            commit_operations.push(top);
        }

        string main_tl = "";
        double main_dur = 0.0;
        Token main_tk = Token();

        commit_stn_operations(&leaf_slots, &commit_operations, &stn, &newly_added_tps);

        PLOGD << "Regular constraints in order\n";
        for (slot s : leaf_slots) {
            // if (s.tk.get_resource() == "robot") {
            //     // Special case when the robot token of one type needs to connect to another
            //     robot
            //     // token of another type. This is required for the "clear" case.
            //     if (main_tk.is_external() && s.tk.is_external() &&
            //         main_tl != slot_to_commit.robot_assignment) {

            //         cout << main_tk.get_end() << ", " << s.tk.get_start() << ", " << zero << ", "
            //              << zero << endl;
            //         assert(add_meets_constraint(&main_tk, &s.tk, &stn, nullptr, true));
            //     }

            //     main_tk = s.tk;
            //     main_tl = s.tl_id;

            //     for (task t : primitive_tasks)
            //         if (s.tk.get_name() == t.name)
            //             main_dur = t.duration;

            //     string dur_name = s.tk.get_start() + duration_constraint + s.tk.get_end();

            //     constraint dur = make_tuple(s.tk.get_start(), s.tk.get_end(), main_dur,
            //     main_dur); cout << get<0>(dur) << ", " << get<1>(dur) << ", " << get<2>(dur) <<
            //     ", "
            //          << get<3>(dur) << endl;
            //     assert(stn.add_constraint(s.tk.get_start_timepoint(),
            //                               s.tk.get_end_timepoint(),
            //                               dur_name,
            //                               make_pair(main_dur, main_dur)));
            //     // assert(stn.add_constraint(dur_name, dur));

            //     for (string dep_meets : s.dependent_ends) {
            //         unordered_map<string, constraint> current_stn_constraints =
            //           stn.get_constraints();
            //         string match_target = dependent_meets_constraint + dep_meets;
            //         for (pair<string, constraint> curr : current_stn_constraints)
            //             if (strstr(curr.first.c_str(), match_target.c_str()))
            //                 assert(stn.del_constraint(curr.first));
            //         string meets_name = s.tk.get_end() + dependent_meets_constraint + dep_meets;
            //         constraint meets = make_tuple(s.tk.get_end(), dep_meets, zero, inf);
            //         cout << get<0>(meets) << ", " << get<1>(meets) << ", " << get<2>(meets) << ",
            //         "
            //              << get<3>(meets) << endl;
            //         assert(stn.add_constraint(meets_name, meets));
            //     }
            // }

            Timeline* t = p->get_timelines(s.tl_id);
            // cout << s.prev.get_end() << ", " << s.tk.get_start() << ", " << zero << ", " << inf
            //      << endl;
            // cout << s.tk.get_end() << ", " << s.next.get_start() << ", " << zero << ", " << inf
            //      << endl;
            // del_and_add_sequencing_constraint(&s.prev, &s.tk, &s.next, &stn, nullptr, true);

            // if (s.tk.get_name() != main_tk.get_name()) {
            //     string dur_name = s.tk.get_start() + duration_constraint + s.tk.get_end();

            //     constraint dur = make_tuple(s.tk.get_start(), s.tk.get_end(), zero, inf);

            //     cout << get<0>(dur) << ", " << get<1>(dur) << ", " << get<2>(dur) << ", "
            //          << get<3>(dur) << endl;
            //     assert(stn.add_constraint(s.tk.get_start_timepoint(),
            //                               s.tk.get_end_timepoint(),
            //                               dur_name,
            //                               make_pair(zero, inf)));
            //     // assert(stn.add_constraint(dur_name, dur));
            //     cout << main_tk.get_end() << ", " << s.tk.get_start() << ", " << zero << ", "
            //          << zero << endl;
            //     assert(add_meets_constraint(&main_tk, &s.tk, &stn));
            // }

            // if ((s.prev.is_external() || s.tk.is_external()) && counter != 0 &&
            //     s.prev.get_request_id() == s.tk.get_request_id()) {
            //     cout << s.prev.get_end() << ", " << s.tk.get_start() << ", " << zero << ", " <<
            //     zero
            //          << endl;
            //     assert(add_meets_constraint(&s.prev, &s.tk, &stn));
            // }

            t->insert_token(s.tk, s.prev, s.next);

            // counter++;
        }
    }
}

bool
patch_plan(Plan* p,
           tasknetwork_solution* old_request_solution,
           vector<primitive_solution>* current_solution,
           validation_state* exception_state)
{

    // Assumption: The plan can only fail to be validated at the external rail move tokens for now
    assert(exception_state->failing_token.is_external());

    switch (exception_state->status) {

        case validation_exception::NO_EXCEPTION:
            break;
        case validation_exception::UNKNOWN:
            PLOGE << "Fatal error! Plan validation failed due to unknown reasons\n";
        case validation_exception::EDGE_SKIP:
            // The edge required does not exist so we have to add additional tokens to make it
            // happen
            PLOGE << "EDGE SKIP\n\n";
            PLOGD << exception_state->to_string() << endl;
            break;
        case validation_exception::VERTEX_SAME: {
            // We are already at the vertex where we want to go so no need for additional task as
            // this is essentially a no-op

            // Find the affected tokens that need to be deleted
            /* vector<Token> affected_tokens = vector<Token>(); */
            /* affected_tokens.push_back(exception_state->failing_token); */

            /* int ps_counter = 0; */
            /* Token previous_token = Token(); */
            /* primitive_solution new_ps = primitive_solution(); */
            /* for (primitive_solution ps : old_request_solution->solution) { */
            /*     new_ps = ps; */
            /*     int ts_counter = 0; */
            /*     bool found = false; */
            /*     for (slot ts : ps.token_slots) { */
            /*         string c_name = exception_state->failing_token.get_end() + meets_constraint +
             */
            /*                         ts.tk.get_start(); */
            /*         if (ts.tk.get_resource() != exception_state->failing_token.get_resource() &&
             */
            /*             get<0>(stn.get_constraint(c_name)) != "") { */
            /*             affected_tokens.push_back(ts.tk); */
            /*             new_ps.token_slots.erase( */
            /*               std::remove(new_ps.token_slots.begin(), new_ps.token_slots.end(), ts),
             */
            /*               new_ps.token_slots.end()); */

            /*         } else if (ts.prev.get_start() == exception_state->failing_token.get_start())
             * { */
            /*             ptrdiff_t pos = std::distance( */
            /*               new_ps.token_slots.begin(), */
            /*               std::find(new_ps.token_slots.begin(), new_ps.token_slots.end(), ts));
             */
            /*             new_ps.token_slots[pos].prev = previous_token; */
            /*         } */
            /*         if (!found && ts.tk.get_start() ==
             * exception_state->failing_token.get_start()) { */
            /*             found = true; */
            /*             new_ps.token_slots.erase( */
            /*               std::remove(new_ps.token_slots.begin(), new_ps.token_slots.end(), ts),
             */
            /*               new_ps.token_slots.end()); */
            /*             previous_token = ts.prev; */
            /*         } */
            /*         ts_counter++; */
            /*     } */
            /*     if (found) */
            /*         break; */
            /*     ps_counter++; */
            /* } */

            /* // Update the original solution slots by deleting the unneeded slots */
            /* old_request_solution->solution[ps_counter] = new_ps; */

            /* PLOGE << "Old Solution\n"; */
            /* PLOGD << old_request_solution->to_string() << endl; */

            /* PLOGE << "Current Solution\n"; */
            /* for (primitive_solution ps : (*current_solution)) */
            /*     PLOGD << ps.to_string() << endl; */

            /* map<string, constraint> constraints = stn.get_constraints(); */
            /* for (Token tk : affected_tokens) */
            /*     for (pair<string, constraint> c : constraints) { */
            /*         if (c.first.find(tk.get_start()) != string::npos || */
            /*             c.first.find(tk.get_end()) != string::npos) { */
            /*             PLOGD << "Constraint: " << get<0>(c.second) << " " << get<1>(c.second) */
            /*                   << " " << get<2>(c.second) << " " << get<3>(c.second) << endl; */
            /*         } */
            /*     } */
            break;
        }
        case validation_exception::PREC_FAIL:
            // If the failed precondition is due to a rail move operation then that can be tried to
            // be fixed
            PLOGE << "PRECONDITION FAILED\n\n";
            break;
        default:
            break;
    }

    // Need to change this later
    return true;
}

/* pair<bool, Plan> */
/* patch_plan(Plan p, Token* failing_tk, vector<ground_literal>* init) */
/* { */
/*     vector<Timeline> tls = p.get_timelines(); */
/*     // Initialize the world state of all the robots and items */
/*     world_state current_state = world_state(); */
/*     for (pair<string, set<map<string, var_declaration>>> cso : csorts) */
/*         if (cso.first == "item" or cso.first == "robot") */
/*             for (map<string, var_declaration> cs : cso.second) */
/*                 for (pair<string, var_declaration> m : cs) { */
/*                     object_state obj = object_state(); */
/*                     obj.object_name = m.first; */
/*                     obj.attribute_states = m.second; */
/*                     current_state.insert(make_pair(obj.object_name, obj)); */
/*                 } */
/*     // Access the failing token's timeline to update the world state for the */
/*     // failing robot token and get its earliest starting time. Also find the */
/*     // token that was not an external token i.e the token whose preconditions */
/*     // failed originally. */
/*     double earliest_start_time = abs(get<0>(stn.get_feasible_values(failing_tk->get_start())));
 */
/*     Token original_failing_tk = Token(); */
/*     Timeline failing_timeline = Timeline(); */
/*     for (Timeline t : tls) { */
/*         for (Token tk : t.get_tokens()) { */
/*             if (tk == (*failing_tk)) { */
/*                 failing_timeline = t; */
/*                 break; */
/*             } */
/*         } */
/*     } */
/*     int failing_tk_index = 0; */
/*     vector<Token> failing_timeline_tks = failing_timeline.get_tokens(); */
/*     for (Token tk : failing_timeline_tks) { */
/*         if (tk == (*failing_tk)) */
/*             break; */
/*         object_state rs = current_state.at(failing_timeline.get_id()); */
/*         update_object_state(&rs, &tk); */
/*         current_state[failing_timeline.get_id()] = rs; */
/*         failing_tk_index++; */
/*     } */
/*     for (int i = failing_tk_index; i < (int)failing_timeline_tks.size(); i++) { */
/*         if (!failing_timeline_tks[i].is_external()) { */
/*             original_failing_tk = failing_timeline_tks[i]; */
/*             break; */
/*         } */
/*     } */
/*     // For all the other robot timelines update their world state till their */
/*     // earliest start times are less than the failing token's earliest start */
/*     // time */
/*     for (Timeline t : tls) { */
/*         if (t.get_resource() != "rail_block" && t.get_id() != failing_timeline.get_id()) { */
/*             for (Token tk : t.get_tokens()) { */
/*                 if (abs(get<0>(stn.get_feasible_values(tk.get_start()))) < earliest_start_time) {
 */
/*                     object_state rs = current_state.at(t.get_id()); */
/*                     update_object_state(&rs, &tk); */
/*                     current_state[t.get_id()] = rs; */
/*                 } */
/*             } */
/*         } */
/*     } */
/*     task tk_task = task(); */
/*     for (task t : primitive_tasks) */
/*         if (t.name == original_failing_tk.get_name()) { */
/*             tk_task = t; */
/*             break; */
/*         } */
/*     // Remove the external tokens till the token that was the original failing */
/*     // task. Correspondingly remove the dependent tokens on other resources that */
/*     // were created */
/*     pair<int, int> range = pair<int, int>(); */
/*     int i = failing_tk_index; */
/*     for (i = failing_tk_index; failing_timeline_tks[i] != original_failing_tk; i++) { */
/*         if (failing_timeline_tks[i].is_external()) { */
/*             task ftk_task = task(); */
/*             for (task t : primitive_tasks) */
/*                 if (t.name == failing_timeline_tks[i].get_name()) { */
/*                     ftk_task = t; */
/*                     break; */
/*                 } */
/*             // For the other resources dependents of this token, we need to */
/*             // remove them as well and update their corresponding timeline */
/*             // states with new sequencing constraints */
/*             for (arg_and_type arg : failing_timeline_tks[i].get_arguments()) */
/*                 if (is_resource(arg.second) && */
/*                     find(ftk_task.vars.begin(), ftk_task.vars.end(), arg) != ftk_task.vars.end())
 */
/*                     for (arg_and_type k : failing_timeline_tks[i].get_knowns()) */
/*                         if (k.first == arg.first && arg.second != "robot") { */
/*                             Timeline* rt = p.get_timelines(k.second, arg.second); */
/*                             vector<Token> rt_tokens = rt->get_tokens(); */
/*                             for (int j = 0; j < (int)rt_tokens.size(); j++) { */
/*                                 string contains_co = failing_timeline_tks[i].get_start() + */
/*                                                      contains_constraint +
 * rt_tokens[j].get_start(); */
/*                                 constraint contains = stn.get_constraint(contains_co); */
/*                                 if (get<0>(contains) != "") { */
/*                                     stn.del_timepoint(rt_tokens[j].get_start()); */
/*                                     stn.del_timepoint(rt_tokens[j].get_end()); */
/*                                     rt->del_token(j); */
/*                                     if (j < (int)rt->get_tokens().size()) { */
/*                                         string seq_name = rt_tokens[j - 1].get_end() + */
/*                                                           sequencing_constraint + */
/*                                                           rt_tokens[j + 1].get_start(); */
/*                                         constraint seq = make_tuple(rt_tokens[j - 1].get_end(),
 */
/*                                                                     rt_tokens[j + 1].get_start(),
 */
/*                                                                     zero, */
/*                                                                     inf); */
/*                                         assert(stn.add_constraint(seq_name, seq)); */
/*                                     } */
/*                                     break; */
/*                                 } */
/*                             } */
/*                         } */
/*             stn.del_timepoint(failing_timeline_tks[i].get_start()); */
/*             stn.del_timepoint(failing_timeline_tks[i].get_end()); */
/*         } */
/*     } */
/*     range = make_pair(failing_tk_index, i); */
/*     p.get_timelines(failing_timeline.get_id())->del_tokens(range); */
/*     failing_timeline_tks = p.get_timelines(failing_timeline.get_id())->get_tokens(); */
/*     if (abs(failing_tk_index - i) > 0 && failing_tk_index < (int)failing_timeline_tks.size()) {
 */
/*         // Now need to add the sequencing constraint between the tokens that */
/*         // remained after we deleted the external tokens */
/*         string seq_name = failing_timeline_tks[failing_tk_index - 1].get_end() + */
/*                           sequencing_constraint + */
/*                           failing_timeline_tks[failing_tk_index].get_start(); */
/*         constraint seq = make_tuple(failing_timeline_tks[failing_tk_index - 1].get_end(), */
/*                                     failing_timeline_tks[failing_tk_index].get_start(), */
/*                                     zero, */
/*                                     inf); */
/*         assert(stn.add_constraint(seq_name, seq)); */
/*     } */

/*     // Check the preconditions of the original failing task against the current */
/*     // updated world state */
/*     bool failed = false; */
/*     for (literal prec : original_failing_tk.get_preconditions()) { */
/*         // Need this check to limit ourselves to preconditions of the orginal */
/*         // task and not compare against the ones that were inherited from the */
/*         // task tree */
/*         // */
/*         if (find(tk_task.prec.begin(), tk_task.prec.end(), prec) != tk_task.prec.end()) { */
/*             set<arg_and_type> tk_knowns = original_failing_tk.get_knowns(); */
/*             bool succ = check_precondition(&prec, &tk_knowns, &current_state, init); */
/*             if (!succ) { */
/*                 // TODO: Add the case when we need to rectify the states */
/*                 Plan plan_copy = p; */
/*                 vector<slot> empty_explored = vector<slot>(); */
/*                 pair<bool, vector<slot>> return_slots = */
/*                   satisfy_precondition(&prec, */
/*                                        &original_failing_tk, */
/*                                        &failing_timeline_tks[failing_tk_index - 1], */
/*                                        &current_state, */
/*                                        &plan_copy, */
/*                                        &stn, */
/*                                        &empty_explored, */
/*                                        depth + 1); */

/*                 if (!return_slots.first) { */
/*                     failed = true; */
/*                     break; */
/*                 } else { */
/*                     // Managed to successfully be able to schedule new tokens to */
/*                     // satisfy the failed precondition */
/*                     for (slot s : return_slots.second) { */
/*                         Timeline* t = p.get_timelines(s.tl_id); */
/*                         t->insert_token(s.tk, s.prev, s.next); */
/*                     } */
/*                 } */
/*             } */
/*         } */
/*     } */
/*     return make_pair(!failed, p); */
/* } */

pq
find_feasible_slots(task_network tree, Plan p, string robot_assignment, int attempts, string metric)
{
    pq ret = pq();

    vector<task_vertex> leafs = vector<task_vertex>();

    task_vertex_it vi = task_vertex_it(), vi_end = task_vertex_it();
    for (boost::tie(vi, vi_end) = boost::vertices(tree.adj_list); vi != vi_end; vi++)
        if (boost::out_degree(*vi, tree.adj_list) == 0)
            leafs.push_back(tree.adj_list[*vi]);

    vector<slot> explored_slots = vector<slot>();

    // -1 attempts means that we need automatic scaling of the number of attempts
    if (attempts == -1)
        attempts = p.num_tasks_robot[robot_assignment] + 1;

    for (int plan_id = 0; plan_id < attempts; plan_id++) {
        double value = 0.0;
        tasknetwork_solution sol = tasknetwork_solution();
        sol.plan_id = plan_id;
        sol.request_id = tree.id;
        sol.robot_assignment = robot_assignment;
        vector<primitive_solution> solution = vector<primitive_solution>();
        unordered_map<string, constraint> pre_stn_constraints = stn.get_constraints();

        bool success = schedule_leafs(leafs, &solution, p, &explored_slots, &value, metric);

        if (success) {

            for (int i = 0; i < (int)solution.size(); i++)
                for (slot s : solution[i].token_slots) {
                    if (s.tk.get_start() == solution[i].primitive_token.get_start())
                        continue;
                    stn.del_timepoint(s.tk.get_start_timepoint());
                    stn.del_timepoint(s.tk.get_end_timepoint());
                    // stn.del_timepoint(s.tk.get_start());
                    // stn.del_timepoint(s.tk.get_end());
                }

            sol.solution = solution;
            sol.metric_value = value;
            ret.push(sol);
        }

        unordered_map<string, constraint> post_stn_constraints = stn.get_constraints();
        for (pair<string, constraint> post : post_stn_constraints)
            if (pre_stn_constraints.find(post.first) == pre_stn_constraints.end())
                assert(stn.del_constraint(post.first));
        for (pair<string, constraint> pre : pre_stn_constraints)
            if (post_stn_constraints.find(pre.first) == post_stn_constraints.end())
                assert(stn.add_constraint(pre.first, pre.second));
    }

    return ret;
}

bool
is_resource(string type)
{
    for (sort_definition sd : sort_definitions)
        if (find(sd.declared_sorts.begin(), sd.declared_sorts.end(), type) !=
            sd.declared_sorts.end())
            if (sd.has_parent_sort && sd.parent_sort == "discrete_reusable_resource")
                return true;
    return false;
}

map<string, arg_and_type>
find_assignments(string cart_prod, vector<arg_and_type> open_vars)
{
    vector<string> assignments = vector<string>();
    map<string, arg_and_type> arg_to_assignment_map = map<string, arg_and_type>();

    boost::algorithm::split(assignments, cart_prod, boost::is_any_of(","));
    for (string arg : assignments) {
        string type = "";
        for (pair<string, set<string>> m : sorts)
            for (string s : m.second)
                if (s == arg) {
                    type = m.first;
                    break;
                }

        for (arg_and_type o : open_vars) {
            if (o.second == type)
                arg_to_assignment_map.insert(make_pair(arg, o));
        }
    }

    return arg_to_assignment_map;
}

bool
clear(vector<string> arguments, world_state* current_state)
{
    vertex source = vertex(), sink = vertex();
    sink.id = arguments[1];
    source.id = arguments[0];

    if (source.id == sink.id)
        return true;

    vertex_t t = get_vertex(sink.id, rail_network);
    vertex_t s = get_vertex(source.id, rail_network);

    vector<vector<vertex_t>> paths = find_all_paths(s, t);

    sort(paths.begin(), paths.end(), [](const vector<vertex_t>& a, const vector<vertex_t>& b) {
        return a.size() < b.size();
    });

    string robot_id = "", rail_attr = "";
    for (sort_definition sd : sort_definitions)
        if (find(sd.declared_sorts.begin(), sd.declared_sorts.end(), "robot") !=
            sd.declared_sorts.end())
            for (arg_and_type var : sd.vars.vars)
                if (var.second == "rail_block")
                    rail_attr = var.first.substr(1);

    assert(rail_attr.length() != 0);

    for (map<string, var_declaration> cs : csorts["robot"])
        for (pair<string, var_declaration> m : cs)
            if ((*current_state).find(m.first) != (*current_state).end())
                for (arg_and_type attr : (*current_state)[m.first].attribute_states.vars)
                    if (attr.first == rail_attr && attr.second == source.id)
                        robot_id = m.first;

    assert(robot_id.length() != 0);

    vector<vertex_t> occupied = vector<vertex_t>();
    for (pair<string, object_state> ws : (*current_state))
        if (ws.first != robot_id)
            for (map<string, var_declaration> cs : csorts["robot"])
                for (pair<string, var_declaration> m : cs)
                    if (m.first == ws.first)
                        for (arg_and_type v : ws.second.attribute_states.vars)
                            if (v.first == "block")
                                occupied.push_back(get_vertex(v.second, rail_network));

    bool free = true;
    for (vector<vertex_t> path : paths) {
        free = true;
        for (vertex_t v : path)

            if (find(occupied.begin(), occupied.end(), v) != occupied.end()) {
                free = false;
                break;
            }
    }

    return free;
}

// Assuming that there is only one other robot apart from robot_id
vector<pair<task, var_declaration>>
clear_and_move(vector<string> arguments, world_state* current_state)
{
    string robot_id = arguments[0];
    vertex sink = vertex();
    sink.id = arguments[1];

    vector<pair<task, var_declaration>> result = vector<pair<task, var_declaration>>();

    vertex source = vertex();
    vector<pair<string, vertex_t>> occupied = vector<pair<string, vertex_t>>();
    for (pair<string, object_state> ws : (*current_state)) {
        if (ws.first != robot_id) {
            for (map<string, var_declaration> cs : csorts["robot"])
                for (pair<string, var_declaration> m : cs)
                    if (m.first == ws.first)
                        for (arg_and_type v : ws.second.attribute_states.vars)
                            if (v.first == "block")
                                occupied.push_back(
                                  make_pair(ws.first, get_vertex(v.second, rail_network)));

        } else {
            for (arg_and_type var : ws.second.attribute_states.vars)
                if (var.first == "block")
                    source.id = var.second;
        }
    }

    if (source.id == sink.id)
        return result;

    vertex_t t = get_vertex(sink.id, rail_network);
    vertex_t s = get_vertex(source.id, rail_network);

    vector<vector<vertex_t>> paths = find_all_paths(s, t);

    sort(paths.begin(), paths.end(), [](const vector<vertex_t>& a, const vector<vertex_t>& b) {
        return a.size() < b.size();
    });

    bool free = true;
    double length = 0, smallest_path_length = 1000;
    vector<vertex_t> offending_path = vector<vertex_t>();
    vector<vertex_t> smallest_offending_path = vector<vertex_t>();
    string offending_robot = string(), smallest_offending_robot = string();
    vertex_t offending_source = vertex_t(), smallest_offending_source = vertex_t();
    for (vector<vertex_t> path : paths) {
        free = true;
        vector<pair<string, vertex_t>>::iterator it = vector<pair<string, vertex_t>>::iterator();
        for (vertex_t v : path) {
            it = find_if(occupied.begin(), occupied.end(), [&v](const pair<string, vertex_t>& p) {
                return p.second == v;
            });
            if (it != occupied.end()) {
                free = false;
                break;
            }
        }
        length = path.size();

        if (!free) {
            offending_robot = it->first;
            offending_source = it->second;
            vertex_t offending_sink = vertex_t();

            vertex_it v = vertex_it(), vend = vertex_it();
            bool path_found = false;
            offending_path = vector<vertex_t>();
            for (boost::tie(v, vend) = boost::vertices(rail_network.adj_list); v != vend; v++) {
                if (find(path.begin(), path.end(), *v) == path.end()) {
                    // Find a non-blocking path for the offending robot
                    offending_sink = *v;
                    vector<vector<vertex_t>> offending_paths =
                      find_all_paths(offending_source, offending_sink);
                    sort(offending_paths.begin(),
                         offending_paths.end(),
                         [](const vector<vertex_t>& a, const vector<vertex_t>& b) {
                             return a.size() < b.size();
                         });

                    for (vector<vertex_t> off_path : offending_paths) {
                        if (find(off_path.begin(), off_path.end(), s) == off_path.end()) {
                            offending_path = off_path;
                            path_found = true;
                            break;
                        }
                    }

                    if (path_found)
                        break;
                }
            }

            length += offending_path.size();
        }

        if (smallest_path_length > length) {
            smallest_path_length = length;
            smallest_offending_path = offending_path;
            smallest_offending_robot = offending_robot;
            smallest_offending_source = offending_source;
        }
    }

    for (task t : primitive_tasks)
        if (!t.artificial && t.name == "rail_move") {
            for (vertex_t p : smallest_offending_path) {
                if (p == smallest_offending_source)
                    continue;
                var_declaration vardecl = var_declaration();
                for (arg_and_type var : t.vars) {
                    if (var.second == "robot")
                        vardecl.vars.push_back(make_pair(var.first, smallest_offending_robot));
                    else if (var.second == "rail_block")
                        vardecl.vars.push_back(make_pair(var.first, rail_network.adj_list[p].id));
                }

                result.push_back(make_pair(t, vardecl));
            }

            break;
        }

    return result;
}

vector<pair<task, var_declaration>>
reachable_location(vector<string> arguments, world_state* current_state)
{
    // TODO: Make it work irrespective of process location
    string nv = std::to_string(num_vertices(rail_network.adj_list));
    string nr = std::to_string(requests.size());

    filesystem::path json_file = filesystem::current_path().parent_path() / "data";
    string target_file = nv + "_blocks_" + nr + "_requests.json";
    json_file = json_file / filesystem::path(target_file);
    ifstream i(json_file.string());
    nlohmann::json datafile = nlohmann::json::parse(i);
    i.close();

    nlohmann::json blocks = datafile["block_bounds"];
    nlohmann::json locations = datafile["locations"];

    string robot_id = arguments[0];
    string loc = arguments[1];

    coordinate location = coordinate();
    location.x = locations[loc][0];
    location.y = locations[loc][1];
    location.z = locations[loc][2];

    vertex sink = vertex();
    for (auto it = blocks.begin(); it != blocks.end(); it++)
        if (it.value()[0].get<double>() < location.y && it.value()[1].get<double>() >= location.y)
            sink.id = it.key();

    vertex source = vertex();
    for (arg_and_type attr : (*current_state)[robot_id].attribute_states.vars)
        for (string declared_sort : sorts["rail_block"])
            if (attr.second == declared_sort) {
                source.id = attr.second;
                break;
            }

    vertex_t t = get_vertex(sink.id, rail_network);
    vertex_t s = get_vertex(source.id, rail_network);

    vector<vector<vertex_t>> paths = find_all_paths(s, t);
    sort(paths.begin(), paths.end(), [](const vector<vertex_t>& a, const vector<vertex_t>& b) {
        return a.size() < b.size();
    });

    vector<pair<task, var_declaration>> result = vector<pair<task, var_declaration>>();

    if (source.id == sink.id)
        return result;

    vector<vertex_t> path = paths[0];
    for (task t : primitive_tasks)
        if (!t.artificial && t.name == "rail_move") {
            for (vertex_t p : path) {
                if (p == s)
                    continue;
                var_declaration vardecl = var_declaration();
                for (arg_and_type var : t.vars) {
                    if (var.second == "robot")
                        vardecl.vars.push_back(make_pair(var.first, robot_id));
                    else if (var.second == "rail_block")
                        vardecl.vars.push_back(make_pair(var.first, rail_network.adj_list[p].id));
                }

                result.push_back(make_pair(t, vardecl));
            }

            break;
        }

    return result;
}
