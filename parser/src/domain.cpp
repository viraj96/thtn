#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>

#include "cwa.hpp"
#include "domain.hpp"
#include "parsetree.hpp"

using namespace std;

map<string, int> sort_visi = map<string, int>();
string GUARD_PREDICATE = "multi_stage_execution_guard";
map<string, set<string>> sort_adj = map<string, set<string>>();

string
var_declaration::to_string() const
{
    string result = "Var Declaration: ";
    result += "\n\t Vars = \n";
    for (arg_and_type arg : vars)
        result += "\t\t (" + arg.first + ", " + arg.second + ")\n";

    return result;
}

bool
operator<(const var_declaration& left, const var_declaration& right)
{
    return (left.vars.size() < right.vars.size());
}

string
object_state::to_string() const
{
    string result = "Object State: ";
    result += "\n\t Name = " + object_name;
    result += "\n\t Parent = " + parent;
    result += "\n\t Attribute Types = " + attribute_types.to_string();
    result += "\n\t Attribute States = " + attribute_states.to_string();
    result += "\n";

    return result;
}

string
literal::to_string() const
{
    string result = "Literal: ";

    result += "\n\t Positive = ";
    if (positive)
        result += "true\n";
    else
        result += "false\n";

    result += "\t Functional = ";
    if (functional)
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
    for (string arg : arguments)
        result += "\t\t " + arg + "\n";

    return result;
}

bool
operator<(const literal& left, const literal& right)
{
    return tie(left.positive, left.timed, left.predicate, left.arguments, left.temp_qual) <
           tie(right.positive, right.timed, right.predicate, right.arguments, right.temp_qual);
}

bool
operator==(const literal& left, const literal& right)
{
    return tie(left.positive, left.timed, left.predicate, left.arguments, left.temp_qual) ==
           tie(right.positive, right.timed, right.predicate, right.arguments, right.temp_qual);
}

string
conditional_effect::to_string() const
{
    string result = "Conditional Effect: ";
    result += "\n\t Effect = " + effect.to_string() + "\n";
    result += "\t Condition = \n";
    for (literal cond : condition)
        result += "\t\t " + cond.to_string() + "\n";

    return result;
}

string
task::to_string() const
{
    string result = "Task: ";

    result += "\n\t Name = " + name + "\n";
    result += "\t Duration = " + std::to_string(duration) + "\n";

    result += "\t Artificial = ";
    if (artificial)
        result += "true\n";
    else
        result += "false\n";

    result += "\t Number of original vars = " + std::to_string(number_of_original_vars) + "\n";

    result += "\t Effect = \n";
    for (literal e : eff)
        result += "\t\t" + e.to_string() + "\n";

    result += "\t Precondition = \n";
    for (literal p : prec)
        result += "\t\t" + p.to_string() + "\n";

    result += "\t Vars = \n";
    for (arg_and_type arg : vars)
        result += "\t\t (" + arg.first + ", " + arg.second + ")\n";

    result += "\t Constraints = \n";
    for (literal cons : constraints)
        result += "\t\t " + cons.to_string() + "\n";

    result += "\t Cost Expression = \n";
    for (literal ce : costExpression)
        result += "\t\t " + ce.to_string() + "\n";

    result += "\t Conditional Effects = \n";
    for (conditional_effect ce : ceff)
        result += "\t\t " + ce.to_string() + "\n";

    return result;
}

bool
operator<(const task& left, const task& right)
{
    return (left.name < right.name);
}

string
plan_step::to_string() const
{
    string result = "Plan Step: ";
    result += "\n\t Id = " + id + "\n";
    result += "\t Task = " + task + "\n";
    result += "\t Arguments = \n";
    for (string arg : args)
        result += "\t\t " + arg + "\n";

    return result;
}

bool
operator<(const plan_step& left, const plan_step& right)
{
    return (left.id < right.id);
}

string
sync_constraints::to_string() const
{
    string result = "Synchronization Constraints: ";

    result += "\n\t Task 1 = " + task1 + "\n";
    result += "\t Task 2 = " + task2 + "\n";

    result += "\t Contiguous = ";
    if (contiguous)
        result += "true\n";
    else
        result += "false\n";

    result += "\t Upper Bound = " + std::to_string(upper_bound) + "\n";
    result += "\t Lower Bound = " + std::to_string(lower_bound) + "\n";

    return result;
}

string
method::to_string() const
{
    string result = "Method: ";

    result += "\n\t Abtract Task = " + at + "\n";
    result += "\t Name = " + name + "\n";

    result += "\t Functional = ";
    if (functional)
        result += "true\n";
    else
        result += "false\n";

    result += "\t Plan Steps = \n";
    for (plan_step p : ps)
        result += "\t\t " + p.to_string() + "\n";

    result += "\t Abstract Task Arguments = \n";
    for (string arg : atargs)
        result += "\t\t " + arg + "\n";

    result += "\t Vars = \n";
    for (arg_and_type arg : vars)
        result += "\t\t (" + arg.first + ", " + arg.second + ")\n";

    result += "\t Constraints = \n";
    for (literal cons : constraints)
        result += "\t\t " + cons.to_string() + "\n";

    result += "\t Ordering = \n";
    for (arg_and_type o : ordering)
        result += "\t\t (" + o.first + ", " + o.second + ")\n";

    result += "\t Synchronization Constraints = \n";
    for (sync_constraints sc : synchronize_constraints)
        result += "\t\t " + sc.to_string() + "\n";

    return result;
}

string
request::to_string() const
{
    string result = "Request: ";
    result += "\n\t Id " + id + "\n";
    result += "\t Task = " + demand.to_string() + "\n";
    result += "\t Due Date = " + std::to_string(due_date) + "\n";
    result += "\t Release Time = " + std::to_string(release_time) + "\n";
    result += "\t Arguments = \n";
    for (string arg : arguments)
        result += "\t\t " + arg + "\n";

    return result;
}

void
expansion_dfs(string sort)
{
    if (sort_visi[sort] == 1) {
        cout << "Sort hierarchy contains a cycle ... " << endl;
        exit(3);
    }

    sort_visi[sort] = 1;
    for (string subsort : sort_adj[sort]) {
        // If not black
        if (sort_visi[sort] != 2)
            expansion_dfs(subsort);

        // Add constants to myself
        for (string subelem : sorts[subsort])
            sorts[sort].insert(subelem);
    }

    sort_visi[sort] = 2;
}

void
expand_sorts()
{
    for (sort_definition def : sort_definitions) {
        if (def.has_parent_sort)
            sorts[def.parent_sort].size();

        for (string subsort : def.declared_sorts) {
            // Touch to ensure it is contained in the map
            sorts[subsort].size();
            if (def.has_parent_sort)
                sort_adj[def.parent_sort].insert(subsort);
        }

        for (arg_and_type v : def.vars.vars) {
            if (sorts.count(v.second) > 0)
                sort_adj[v.second].insert(v.first);
        }
    }

    for (pair<string, set<string>> s : sorts)
        expansion_dfs(s.first);
}

void
add_to_method_as_last(method& m, plan_step ps)
{
    for (plan_step& ops : m.ps)
        m.ordering.push_back(make_pair(ops.id, ps.id));
    m.ps.push_back(ps);
}

void
addPrimitiveTask(task& t)
{
    primitive_tasks.push_back(t);
    task_name_map[t.name] = t;
}

void
addAbstractTask(task& t)
{
    abstract_tasks.push_back(t);
    task_name_map[t.name] = t;
}

pair<task, bool>
flatten_primitive_task(parsed_task& a,
                       bool compileConditionalEffects,
                       bool linearConditionalEffectExpansion,
                       bool encodeDisjunctivePreconditionsInMethods,
                       bool isArtificial)
{
    // First check whether this primitive as a disjunctive precondition
    bool disjunctivePreconditionForHTN =
      encodeDisjunctivePreconditionsInMethods && a.prec->isDisjunctive();

    // Expand effects and preconditions if necessary and possible
    vector<pair<pair<vector<variant<literal, conditional_effect>>, vector<literal>>,
                additional_variables>>
      elist = a.eff->expand(compileConditionalEffects);

    vector<pair<pair<vector<variant<literal, conditional_effect>>, vector<literal>>,
                additional_variables>>
      plist = vector<pair<pair<vector<variant<literal, conditional_effect>>, vector<literal>>,
                          additional_variables>>();

    // Precondition cannot contain conditional effects
    if (!disjunctivePreconditionForHTN)
        plist = a.prec->expand(false);
    else {
        pair<pair<vector<variant<literal, conditional_effect>>, vector<literal>>,
             additional_variables>
          _temp = pair<pair<vector<variant<literal, conditional_effect>>, vector<literal>>,
                       additional_variables>();
        plist.push_back(_temp);
    }

    // Determine whether any expansion has a conditional effect
    bool expansionHasConditionalEffect = false;
    if (linearConditionalEffectExpansion)
        for (auto e : elist)
            for (auto eff : e.first.first)
                if (holds_alternative<conditional_effect>(eff))
                    expansionHasConditionalEffect = true;

    int i = 0;
    task mainTask = task();
    bool mainTaskIsPrimitive = false;

    for (auto e : elist)
        for (auto p : plist) {
            // Precondition cannot contain conditional effects
            assert(p.first.second.size() == 0);
            i++;

            task t = task();
            t.name = a.name;
            t.artificial = isArtificial;

            // Sort out the constraints
            for (variant<literal, conditional_effect> pl : p.first.first) {
                // Precondition cannot have conditional effects in it
                if (!holds_alternative<literal>(pl))
                    assert(false);

                /* if (get<literal>(pl).predicate ==  dummy_equal_literal) */
                /*     t.constraints.push_back(get<literal>(pl)); */
                /* else t.prec.push_back(get<literal>(pl)); */
                t.prec.push_back(get<literal>(pl));
            }

            // Add preconditions for conditional effects if exponentially
            // compiled
            for (literal ep : e.first.second) {
                assert(ep.predicate != dummy_equal_literal);
                t.prec.push_back(ep);
            }

            // Set effects
            for (auto eff : e.first.first) {
                if (holds_alternative<literal>(eff)) {
                    if (get<literal>(eff).isCostChangeExpression)
                        t.costExpression.push_back(get<literal>(eff));
                    else
                        t.eff.push_back(get<literal>(eff));

                } else {
                    // TODO: State dependent action costs.
                    // This is very complicated.
                    // Ask Robert Mattmüller how to do it.
                    if (get<conditional_effect>(eff).effect.isCostChangeExpression)
                        assert(false);

                    // Conditional effect
                    t.ceff.push_back(get<conditional_effect>(eff));
                }
            }

            // Add declared vars
            t.vars = a.arguments->vars;
            t.duration = a.dur;

            // The parsed variables were the original ones
            t.number_of_original_vars = t.vars.size();

            // Gather the additional variables
            additional_variables addVars = p.second;

            for (arg_and_type elem : e.second)
                addVars.insert(elem);

            for (arg_and_type v : addVars) {
                // Check whether this variable actually occurs anywhere
                bool contained = false;
                for (literal pre : t.prec)
                    for (string arg : pre.arguments)
                        contained |= v.first == arg;

                for (literal eff : t.eff)
                    for (string arg : eff.arguments)
                        contained |= v.first == arg;

                for (conditional_effect ceff : t.ceff) {
                    for (string arg : ceff.effect.arguments)
                        contained |= v.first == arg;
                    for (literal cond : ceff.condition)
                        for (string arg : cond.arguments)
                            contained |= v.first == arg;
                }

                if (!contained)
                    continue;
                t.vars.push_back(v);
            }

            if (plist.size() > 1 || elist.size() > 1 || expansionHasConditionalEffect ||
                disjunctivePreconditionForHTN) {
                // Helper functions
                auto create_predicate_and_literal = [&](string prefix, task ce_at) {
                    // Build three predicates, one for telling that something
                    // has to be checked still
                    predicate_definition argument_predicate = predicate_definition();
                    argument_predicate.name = prefix + ce_at.name;

                    literal argument_literal = literal();
                    argument_literal.timed = START;
                    argument_literal.temp_qual = AT;
                    argument_literal.positive = true;
                    argument_literal.func_literal = 0;
                    argument_literal.functional = false;
                    argument_literal.predicate = argument_predicate.name;

                    for (arg_and_type v : t.vars) {
                        argument_predicate.argument_sorts.push_back(v.second);
                        argument_literal.arguments.push_back(v.first);
                    }

                    predicate_definitions.push_back(argument_predicate);
                    return make_pair(argument_predicate, argument_literal);
                };

                auto create_task = [&](string prefix, vector<arg_and_type> vars) {
                    task tt = task();
                    tt.vars = vars;
                    tt.artificial = false;
                    tt.constraints.clear();
                    tt.duration = t.duration;
                    tt.costExpression.clear();
                    tt.name = prefix + t.name;
                    tt.number_of_original_vars = vars.size();

                    return tt;
                };

                auto create_method = [&](task at, string prefix) {
                    method m_ce = method();
                    m_ce.at = at.name;
                    m_ce.vars = at.vars;
                    m_ce.func_method = 0;
                    m_ce.functional = false;
                    m_ce.name = prefix + at.name;
                    for (arg_and_type v : at.vars)
                        m_ce.atargs.push_back(v.first);

                    return m_ce;
                };

                auto create_singleton_method = [&](task at, task sub, string prefix) {
                    method m_ce = create_method(at, prefix);

                    plan_step ps = plan_step();
                    ps.id = "id0";
                    ps.task = sub.name;

                    set<string> existingVars = set<string>();
                    for (auto [var, _] : m_ce.vars) {
                        (void)_;
                        existingVars.insert(var);
                    }

                    for (arg_and_type v : sub.vars) {
                        if (existingVars.count(v.first) == 0) {
                            m_ce.vars.push_back(v);
                            existingVars.insert(v.first);
                        }
                        ps.args.push_back(v.first);
                    }

                    m_ce.ps.push_back(ps);
                    methods.push_back(m_ce);
                };

                auto create_plan_step = [&](task tt, string prefix) {
                    plan_step p = plan_step();
                    p.id = prefix;
                    p.args.clear();
                    p.task = tt.name;
                    for (arg_and_type v : tt.vars)
                        p.args.push_back(v.first);

                    return p;
                };

                if (plist.size() > 1 || elist.size() > 1)
                    t.name += "|instance_" + to_string(i);

                if (expansionHasConditionalEffect)
                    t.name += "|ce_base_action";

                if (disjunctivePreconditionForHTN)
                    t.name += "|disjunctive_prec";

                // We have to create a new decomposition method at this point
                method m = method();
                // Must start with an underscore s.t. this method is applied by
                // the solution compiler
                m.at = a.name;
                m.func_method = 0;
                m.functional = false;
                m.name = "_method_for_multiple_expansions_of_" + t.name;
                for (arg_and_type v : a.arguments->vars)
                    m.atargs.push_back(v.first);
                m.vars = t.vars;

                // Add the starting task as the first one to the method
                m.ps.push_back(create_plan_step(t, "id_main"));

                if (disjunctivePreconditionForHTN) {
                    // The precondition is disjunctive, so we have to check them
                    // using the HTN structure
                    int globalFCounter = 0;

                    function<void(task&, general_formula*, var_declaration)> generate_formula_HTN;

                    generate_formula_HTN = [&](task& current_task,
                                               general_formula* f,
                                               var_declaration current_vars) -> void {
                        // Do different things based on formula type
                        int fcounter = globalFCounter++;

                        if (f->type == EMPTY) {
                            method m =
                              create_method(current_task, "__formula_empty_" + to_string(fcounter));
                            m.check_integrity();
                            methods.push_back(m);

                        } else if (f->type == ATOM || f->type == NOTATOM || f->type == EQUAL ||
                                   f->type == NOTEQUAL) {
                            string typ = "eq";
                            if (f->type == NOTEQUAL)
                                typ = "neq";
                            if (f->type == ATOM)
                                typ = "atom_" + f->predicate;
                            if (f->type == NOTATOM)
                                typ = "not_atom_" + f->predicate;

                            task check = create_task("__formula_" + typ + "_" + to_string(fcounter),
                                                     current_vars.vars);
                            literal l = (f->type == ATOM || f->type == NOTATOM)
                                          ? f->atomLiteral()
                                          : f->equalsLiteral();

                            check.prec.push_back(l);
                            addPrimitiveTask(check);

                            create_singleton_method(
                              current_task, check, "__formula_" + typ + "_" + to_string(fcounter));

                        } else if (f->type == OR) {
                            int subCounter = 0;
                            for (general_formula* sub : f->subformulae) {
                                // Determine variables relevant for sub formula
                                var_declaration subVars = var_declaration();
                                set<string> occuringVariables =
                                  sub->occuringUnQuantifiedVariables();

                                for (arg_and_type var_decl : current_vars.vars)
                                    if (occuringVariables.count(var_decl.first))
                                        subVars.vars.push_back(var_decl);

                                task subTask = create_task("__formula_or_" + to_string(fcounter) +
                                                             "_" + to_string(subCounter) + "_",
                                                           subVars.vars);
                                addAbstractTask(subTask);

                                // Create the method
                                create_singleton_method(current_task,
                                                        subTask,
                                                        "__formula_or_" + to_string(fcounter) +
                                                          "_" + to_string(subCounter) + "_");

                                generate_formula_HTN(subTask, sub, subVars);
                                subCounter++;
                            }

                        } else if (f->type == WHEN || f->type == VALUE || f->type == COST_CHANGE ||
                                   f->type == COST || f->type == SYNC) {
                            // Not allowed
                            assert(false);

                        } else if (f->type == FORALL) {
                            auto [var_replace, additional_vars] = f->forallVariableReplacement();

                            map<string, string> newVarTypes = map<string, string>();
                            for (auto [var, type] : additional_vars)
                                newVarTypes[var] = type;

                            // We get new variables from the quantification,
                            // but some might already be there
                            set<string> existing_variables = set<string>();
                            for (auto& [var, _] : current_vars.vars) {
                                (void)_;
                                existing_variables.insert(var);
                            }

                            // All of the forall instantiations go into one
                            // method
                            method m = create_method(current_task,
                                                     "__formula_forall_" + to_string(fcounter));

                            int sub = 0;
                            for (map<string, string> replacement : var_replace) {
                                var_declaration sub_vars = current_vars;
                                for (auto& [qvar, newVar] : replacement) {
                                    (void)qvar;
                                    if (existing_variables.count(newVar))
                                        assert(false);
                                    else {
                                        string type = newVarTypes[newVar];
                                        sub_vars.vars.push_back(make_pair(newVar, type));
                                        m.vars.push_back(make_pair(newVar, type));
                                    }
                                }

                                task subTask =
                                  create_task("__formula_forall_" + to_string(fcounter) + "_" +
                                                to_string(sub) + "_",
                                              sub_vars.vars);
                                addAbstractTask(subTask);
                                add_to_method_as_last(
                                  m, create_plan_step(subTask, "id" + to_string(sub++)));

                                // Generate an HTN for the subtask
                                generate_formula_HTN(
                                  subTask, f->subformulae[0]->copyReplace(replacement), sub_vars);
                            }

                            m.check_integrity();
                            methods.push_back(m);

                        } else if (f->type == AND) {
                            method m =
                              create_method(current_task, "__formula_and_" + to_string(fcounter));

                            int subCounter = 0;
                            for (general_formula* sub : f->subformulae) {
                                var_declaration subVars = var_declaration();
                                set<string> occuringVariables =
                                  sub->occuringUnQuantifiedVariables();
                                for (arg_and_type var_decl : current_vars.vars)
                                    if (occuringVariables.count(var_decl.first))
                                        subVars.vars.push_back(var_decl);

                                task subTask = create_task("__formula_and_" + to_string(fcounter) +
                                                             "_" + to_string(subCounter) + "_",
                                                           subVars.vars);
                                addAbstractTask(subTask);
                                add_to_method_as_last(
                                  m, create_plan_step(subTask, "id" + to_string(subCounter++)));

                                // Generate an HTN for the subtask
                                generate_formula_HTN(subTask, sub, subVars);
                            }

                            m.check_integrity();
                            methods.push_back(m);

                        } else if (f->type == EXISTS) {
                            map<string, string> var_replace = f->existsVariableReplacement();

                            // We get new variables from the quantification,
                            // but some might already be there
                            set<string> existing_variables = set<string>();
                            for (auto& [var, _] : current_vars.vars) {
                                (void)_;
                                existing_variables.insert(var);
                            }

                            var_declaration sub_vars = current_vars;
                            for (auto& [qvar, type] : f->qvariables.vars) {
                                string newVar = var_replace[qvar];
                                if (existing_variables.count(newVar))
                                    assert(false);
                                else {
                                    sub_vars.vars.push_back(make_pair(newVar, type));
                                    m.vars.push_back(make_pair(newVar, type));
                                }
                            }

                            task subTask = create_task(
                              "__formula_exists_" + to_string(fcounter) + "_", sub_vars.vars);
                            addAbstractTask(subTask);

                            create_singleton_method(current_task,
                                                    subTask,
                                                    "__formula_exists_" + to_string(fcounter) +
                                                      "_");
                            generate_formula_HTN(
                              subTask, f->subformulae[0]->copyReplace(var_replace), sub_vars);
                        }
                    };

                    // Determine initially needed variables
                    var_declaration initialVariables = var_declaration();
                    set<string> occuringVariables = a.prec->occuringUnQuantifiedVariables();

                    for (arg_and_type var_decl : t.vars)
                        if (occuringVariables.count(var_decl.first))
                            initialVariables.vars.push_back(var_decl);

                    for (arg_and_type vars_for_consts : a.prec->variables_for_constants()) {
                        initialVariables.vars.push_back(vars_for_consts);
                        m.vars.push_back(vars_for_consts);
                    }

                    // Create a carrier task
                    task formula_carrier_root =
                      create_task("__formula-root", initialVariables.vars);
                    formula_carrier_root.check_integrity();
                    addAbstractTask(formula_carrier_root);
                    add_to_method_as_last(m,
                                          create_plan_step(formula_carrier_root, "id_prec_root"));

                    generate_formula_HTN(formula_carrier_root, a.prec, initialVariables);
                }

                if (linearConditionalEffectExpansion || disjunctivePreconditionForHTN) {
                    // Construct action applying the conditional effect
                    literal guard_literal = literal();
                    guard_literal.timed = START;
                    guard_literal.temp_qual = AT;
                    guard_literal.positive = false;
                    guard_literal.func_literal = 0;
                    guard_literal.functional = false;
                    guard_literal.predicate = GUARD_PREDICATE;
                    guard_literal.arguments.clear();

                    t.prec.push_back(guard_literal);

                    vector<pair<bool, plan_step>> steps_with_effects =
                      vector<pair<bool, plan_step>>();

                    // Effects of the main task need to be performed AFTER
                    // splitting
                    vector<literal> add_effects = vector<literal>(),
                                    del_effects = vector<literal>();
                    for (literal& el : t.eff)
                        if (el.positive)
                            add_effects.push_back(el);
                        else
                            del_effects.push_back(el);
                    t.eff.clear();

                    auto [main_arguments, main_literal] =
                      create_predicate_and_literal("doing_action_", t);
                    (void)main_arguments;
                    t.eff.push_back(main_literal);
                    guard_literal.positive = true;
                    t.eff.push_back(guard_literal);

                    int j = 0;
                    for (conditional_effect& ceff : t.ceff) {
                        // PHASE 1 check conditions
                        task ce_at = create_task("__ce_check_" + to_string(j) + "_", t.vars);
                        ce_at.check_integrity();
                        addAbstractTask(ce_at);

                        plan_step ce_at_ps = create_plan_step(ce_at, "id_ce_prec_" + to_string(j));
                        add_to_method_as_last(m, ce_at_ps);

                        auto [apply_predicate, apply_literal] =
                          create_predicate_and_literal("do_apply_", ce_at);
                        (void)apply_predicate;
                        auto [not_apply_predicate, not_apply_literal] =
                          create_predicate_and_literal("not_apply_", ce_at);
                        (void)not_apply_predicate;

                        // Add methods for this task, first the one that
                        // actually apply the CE
                        task ce_yes = create_task("__ce_yes_" + to_string(j) + "_", t.vars);
                        ce_yes.prec = ceff.condition;

                        // Additional preconditions
                        main_literal.positive = true;
                        ce_yes.prec.push_back(main_literal);
                        apply_literal.positive = true;
                        ce_yes.eff.push_back(apply_literal);
                        ce_yes.check_integrity();
                        addPrimitiveTask(ce_yes);

                        create_singleton_method(ce_at, ce_yes, "_method_for_ce_yes_");

                        // For every condition of the CE add one possible
                        // negation
                        int noCount = 0;
                        for (literal precL : ceff.condition) {
                            task ce_no = create_task("__ce_no_" + to_string(j) + "_cond_" +
                                                       to_string(noCount) + "_",
                                                     t.vars);

                            precL.positive = !precL.positive;
                            ce_no.prec.push_back(precL);
                            main_literal.positive = true;
                            ce_no.prec.push_back(main_literal);
                            not_apply_literal.positive = true;
                            ce_no.eff.push_back(not_apply_literal);

                            // Additional preconditions
                            ce_no.check_integrity();
                            addPrimitiveTask(ce_no);

                            create_singleton_method(
                              ce_at, ce_no, "_method_for_ce_no_" + to_string(noCount++));
                        }

                        // PHASE 2 apply the effect
                        task ce_apply =
                          create_task("__ce_apply_if_applicable_" + to_string(j) + "_", t.vars);
                        ce_apply.check_integrity();
                        addAbstractTask(ce_apply);

                        plan_step ce_apply_ps =
                          create_plan_step(ce_apply, "id_ce_eff_" + to_string(j));
                        steps_with_effects.push_back(make_pair(ceff.effect.positive, ce_apply_ps));

                        // A task that applies the effect if necessary
                        task ce_do = create_task("__ce_apply_" + to_string(j) + "_", t.vars);
                        ce_do.eff.push_back(ceff.effect);
                        apply_literal.positive = true;
                        ce_do.prec.push_back(apply_literal);
                        apply_literal.positive = false;
                        ce_do.eff.push_back(apply_literal);
                        ce_do.check_integrity();
                        addPrimitiveTask(ce_do);
                        create_singleton_method(ce_apply, ce_do, "_method_for_ce_do_apply_");

                        // A task that does not apply the effect if necessary
                        task ce_not_do =
                          create_task("__ce_not_apply_" + to_string(j) + "_", t.vars);
                        not_apply_literal.positive = true;
                        ce_not_do.prec.push_back(not_apply_literal);
                        not_apply_literal.positive = false;
                        ce_not_do.eff.push_back(not_apply_literal);
                        ce_not_do.check_integrity();
                        addPrimitiveTask(ce_not_do);
                        create_singleton_method(
                          ce_apply, ce_not_do, "_method_for_ce_not_do_apply_");

                        // Increment
                        j++;
                    }

                    // Remove conditional effects from main task
                    t.ceff.clear();

                    // Apply delete effects
                    if (del_effects.size()) {
                        task main_deletes = create_task("_main_delete_", t.vars);
                        main_literal.positive = true;
                        main_deletes.prec.push_back(main_literal);
                        main_deletes.eff = del_effects;

                        main_deletes.check_integrity();
                        addPrimitiveTask(main_deletes);

                        plan_step main_deletes_ps = create_plan_step(main_deletes, "id_main_del");
                        add_to_method_as_last(m, main_deletes_ps);
                    }

                    // First apply the deleting then the adding effects!
                    // Else we break basic STRIPS rules
                    sort(steps_with_effects.begin(), steps_with_effects.end());
                    for (auto& [_, ps] : steps_with_effects) {
                        (void)_;
                        add_to_method_as_last(m, ps);
                    }

                    // Lastly, we need an action that ends the CE
                    task main_add = create_task("_main_adds_", t.vars);
                    main_literal.positive = true;
                    main_add.prec.push_back(main_literal);
                    main_add.eff = add_effects;
                    guard_literal.positive = false;
                    main_add.eff.push_back(guard_literal);
                    main_literal.positive = false;
                    main_add.eff.push_back(main_literal);

                    main_add.check_integrity();
                    addPrimitiveTask(main_add);

                    plan_step main_add_ps = create_plan_step(main_add, "id_main_add");
                    add_to_method_as_last(m, main_add_ps);
                }

                // For this to be ok, we have to create an abstract task in the
                // first round
                if (i == 1) {
                    task at = task();
                    at.name = a.name;
                    at.vars = a.arguments->vars;
                    at.number_of_original_vars = at.vars.size();
                    at.artificial = false;
                    at.check_integrity();
                    addAbstractTask(at);
                    mainTask = at;
                    mainTaskIsPrimitive = false;
                }

                // Add to list, moved to if to enable integrity checking
                t.check_integrity();
                addPrimitiveTask(t);

                m.check_integrity();
                methods.push_back(m);

            } else {
                // Add to list, moved to else to enable integrity checking in if
                t.check_integrity();
                addPrimitiveTask(t);

                mainTask = t;
                mainTaskIsPrimitive = true;
            }
        }

    return make_pair(mainTask, mainTaskIsPrimitive);
}

void
flatten_tasks(bool compileConditionalEffects,
              bool linearConditionalEffectExpansion,
              bool encodeDisjunctivePreconditionsInMethods)
{
    bool artificialUnitCosts = false;
    if (metric_target == dummy_function_type) {
        metric_target = "method_precondition_cost";
        artificialUnitCosts = true;
    }

    if (linearConditionalEffectExpansion || encodeDisjunctivePreconditionsInMethods) {
        predicate_definition guardPredicate = predicate_definition();
        guardPredicate.functional = false;
        guardPredicate.name = GUARD_PREDICATE;
        guardPredicate.argument_sorts.clear();
        predicate_definitions.push_back(guardPredicate);
    }

    // Flatten the primitives
    for (parsed_task& a : parsed_primitive) {
        if (artificialUnitCosts) {
            general_formula* cost_f = new general_formula();
            cost_f->type = COST;
            cost_f->predicate = metric_target;
            cost_f->functional = cost_f->isFunctional();

            general_formula* cost_v = new general_formula();
            cost_v->type = VALUE;
            cost_v->functional = false;
            cost_v->value = 1;

            general_formula* cost_form = new general_formula();
            cost_form->type = COST_CHANGE;
            cost_form->functional = false;
            cost_form->subformulae.push_back(cost_f);
            cost_form->subformulae.push_back(cost_v);

            general_formula* fullEff = new general_formula();
            fullEff->type = AND;
            fullEff->functional = false;
            fullEff->subformulae.push_back(a.eff);
            fullEff->subformulae.push_back(cost_form);

            a.eff = fullEff;
        }

        pair<task, bool> res = flatten_primitive_task(a,
                                                      compileConditionalEffects,
                                                      linearConditionalEffectExpansion,
                                                      encodeDisjunctivePreconditionsInMethods,
                                                      false);
    }

    for (parsed_task& a : parsed_abstract) {
        task at = task();
        at.name = a.name;
        at.vars = a.arguments->vars;
        at.number_of_original_vars = at.vars.size();
        // Abstract tasks cannot have additional variables
        // (e.g. for constants): these cannot be declared in the input
        at.check_integrity();
        addAbstractTask(at);
    }
}

void
parsed_method_to_data_structures(bool compileConditionalEffects,
                                 bool linearConditionalEffectExpansion,
                                 bool encodeDisjunctivePreconditionsInMethods)
{
    int i = 0;
    for (pair<string, vector<parsed_method>> e : parsed_methods)
        for (parsed_method pm : e.second) {
            i++;
            method m = method();
            m.name = pm.name;
            m.func_method = 0;
            m.vars = pm.vars->vars;
            m.functional = pm.functional;

            set<string> mVarSet = set<string>();
            for (arg_and_type v : m.vars)
                mVarSet.insert(v.first);

            // Add variables needed due to constants in the arguments of the
            // abstract task
            map<string, string> at_arg_additional_vars = map<string, string>();
            for (arg_and_type av : pm.newVarForAT) {
                at_arg_additional_vars[av.first] = av.first + "_" + to_string(i++);
                m.vars.push_back(make_pair(at_arg_additional_vars[av.first], av.second));
            }

            // Compute arguments of the abstract task
            m.at = e.first;
            m.atargs.clear();
            for (string arg : pm.atArguments) {
                if (at_arg_additional_vars.count(arg))
                    m.atargs.push_back(at_arg_additional_vars[arg]);
                else
                    m.atargs.push_back(arg);
            }

            if (!m.functional) {
                // Subtasks
                for (sub_task* st : pm.tn->tasks) {
                    plan_step ps = plan_step();
                    ps.id = st->id;
                    ps.task = st->task;
                    map<string, string> arg_replace = map<string, string>();
                    // Add new variables for artificial variables of the subtask
                    for (arg_and_type av : st->arguments->newVar) {
                        arg_replace[av.first] = av.first + "_" + to_string(i++);
                        m.vars.push_back(make_pair(arg_replace[av.first], av.second));
                    }

                    for (string v : st->arguments->vars)
                        if (arg_replace.count(v))
                            ps.args.push_back(arg_replace[v]);
                        else
                            ps.args.push_back(v);

                    // We might have added more parameters to these tasks to
                    // account for constants in them. We have to add them here.
                    task psTask = task_name_map[ps.task];
                    if (psTask.name != ps.task) {
                        cerr << "There is no declaration of the subtask " << ps.task
                             << " in the input" << endl;
                    }

                    // Ensure that we have found one
                    assert(psTask.name == ps.task);
                    for (unsigned int j = st->arguments->vars.size(); j < psTask.vars.size(); j++) {
                        string v = psTask.vars[j].first + "_method_" + m.name + "_instance_" +
                                   to_string(i++);
                        // Add var to set of vars
                        m.vars.push_back(make_pair(v, psTask.vars[j].second));
                        ps.args.push_back(v);
                    }

                    m.ps.push_back(ps);
                }
            }

            parsed_task mPrec_task = parsed_task();
            mPrec_task.name = method_precondition_action_name + m.name;
            mPrec_task.prec = pm.prec;
            mPrec_task.arguments = new var_declaration();

            if (metric_target == dummy_function_type)
                metric_target = "method_precondition_cost";

            general_formula* cost_f = new general_formula();
            cost_f->type = COST;
            cost_f->predicate = metric_target;
            cost_f->functional = cost_f->isFunctional();

            general_formula* cost_v = new general_formula();
            cost_v->type = VALUE;
            cost_v->functional = false;
            cost_v->value = 0;

            general_formula* cost_form = new general_formula();
            cost_form->type = COST_CHANGE;
            cost_form->functional = false;
            cost_form->subformulae.push_back(cost_f);
            cost_form->subformulae.push_back(cost_v);

            general_formula* fullEff = new general_formula();
            fullEff->type = AND;
            fullEff->functional = false;
            fullEff->subformulae.push_back(pm.eff);
            fullEff->subformulae.push_back(cost_form);

            mPrec_task.eff = fullEff;

            set<string> mPrecVars = pm.prec->occuringUnQuantifiedVariables();
            set<string> mEffVars = pm.eff->occuringUnQuantifiedVariables();

            for (arg_and_type var : m.vars) {
                bool add = false;
                for (string s : mPrecVars)
                    if (s == var.first || s.substr(0, s.find(".")) == var.first)
                        add = true;

                for (string s : mEffVars)
                    if (s == var.first || s.substr(0, s.find(".")) == var.first)
                        add = true;

                if (add)
                    mPrec_task.arguments->vars.push_back(var);
            }

            auto [mPrec, isPrimitive] =
              flatten_primitive_task(mPrec_task,
                                     compileConditionalEffects,
                                     linearConditionalEffectExpansion,
                                     encodeDisjunctivePreconditionsInMethods,
                                     !m.functional);
            if (!m.functional)
                mPrec.artificial = true;

            for (size_t newVar = mPrec_task.arguments->vars.size(); newVar < mPrec.vars.size();
                 newVar++)
                m.vars.push_back(mPrec.vars[newVar]);

            // Edge case: The precondition might only have contained constraints
            if (isPrimitive && mPrec.prec.size() == 0 && mPrec.eff.size() == 0 &&
                mPrec.ceff.size() == 0) {
                // Has only constraints
                for (literal l : mPrec.constraints)
                    m.constraints.push_back(l);

                // Remove the task
                primitive_tasks.pop_back();
                task_name_map.erase(mPrec.name);

            } else {
                // Here we actually add the task as a plan step
                plan_step ps = plan_step();
                ps.id = "mprec";
                ps.task = mPrec.name;
                for (auto [v, _] : mPrec.vars) {
                    (void)_;
                    ps.args.push_back(v);
                }

                // Precondition ps precedes all other tasks
                for (plan_step other_ps : m.ps)
                    m.ordering.push_back(make_pair(ps.id, other_ps.id));

                m.ps.push_back(ps);
            }

            if (!m.functional) {
                // Ordering
                for (arg_and_type* o : pm.tn->ordering)
                    m.ordering.push_back(*o);

                for (general_formula* s : pm.tn->synchronize_constraints) {
                    sync_constraints sc = sync_constraints();
                    sc.lower_bound = s->lb;
                    sc.upper_bound = s->ub;
                    sc.task1 = s->task1;
                    sc.task2 = s->task2;
                    sc.contiguous = s->contiguous;
                    m.synchronize_constraints.push_back(sc);
                }

                // Constraints
                vector<pair<pair<vector<variant<literal, conditional_effect>>, vector<literal>>,
                            additional_variables>>
                  exconstraints = pm.tn->constraint->expand(false);
                // Constraints cannot contain conditional effects
                assert(exconstraints.size() == 1);
                // No additional vars due to constraints
                assert(exconstraints[0].second.size() == 0);
                // No conditional effects
                assert(exconstraints[0].first.second.size() == 0);
                for (variant<literal, conditional_effect> l : exconstraints[0].first.first)
                    if (holds_alternative<literal>(l))
                        m.constraints.push_back(get<literal>(l));
                    else
                        // Constraints cannot contain conditional effects
                        assert(false);
            }

            m.check_integrity();
            methods.push_back(m);
        }
}

void
reduce_constraints()
{
    int ns_count = 0;
    vector<method> oldm = methods;
    methods.clear();

    for (method m : oldm) {
        m.check_integrity();

        // Remove sort constraints
        vector<literal> oldC = m.constraints;
        m.constraints.clear();
        map<string, string> sorts_of_vars = map<string, string>();
        for (arg_and_type pp : m.vars)
            sorts_of_vars[pp.first] = pp.second;

        for (literal l : oldC)
            if (l.predicate == dummy_equal_literal)
                m.constraints.push_back(l);
            else {
                // Determine the now forbidden variables
                set<string> currentVals = sorts[sorts_of_vars[l.arguments[0]]];
                set<string> sortElems = sorts[l.arguments[1]];
                vector<string> forbidden = vector<string>();
                if (l.positive) {
                    for (string s : currentVals)
                        if (sortElems.count(s) == 0)
                            forbidden.push_back(s);

                } else {
                    for (string s : sortElems)
                        forbidden.push_back(s);
                }
                for (string f : forbidden) {
                    literal nl = literal();
                    nl.timed = START;
                    nl.temp_qual = AT;
                    nl.positive = false;
                    nl.functional = l.functional;
                    nl.func_literal = l.func_literal;
                    nl.predicate = dummy_equal_literal;
                    nl.arguments.push_back(l.arguments[0]);
                    nl.arguments.push_back(f);
                    m.constraints.push_back(nl);
                }
            }

        m.check_integrity();
        methods.push_back(m);
    }

    oldm = methods;
    methods.clear();

    for (method m : oldm) {
        method nm = m;
        vector<literal> oldC = nm.constraints;
        nm.constraints.clear();

        bool removeMethod = false;
        map<string, string> vSort = map<string, string>();
        for (arg_and_type x : m.vars)
            vSort[x.first] = x.second;

        for (literal l : oldC) {
            if (l.arguments[1][0] == '?' && l.arguments[0][0] != '?') {
                // Ensure that the constant is always the second
                swap(l.arguments[0], l.arguments[1]);
            }

            if (l.arguments[1][0] != '?' && l.arguments[0][0] != '?') {
                // Comparing two constants
                if (l.positive == (l.arguments[0][0] != l.arguments[1][0])) {
                    // Constraint cannot be satisfied
                    removeMethod = true;
                }

                continue;
            }

            if (l.arguments[1][0] == '?') {
                nm.constraints.push_back(l);
                continue;
            }

            string v = l.arguments[0];
            string c = l.arguments[1];

            // v != c and c is not in the sort of v
            if (!l.positive && !sorts[vSort[v]].count(c))
                continue;

            // Recompute sort
            set<string> vals = sorts[vSort[v]];
            if (l.positive) {
                if (vals.count(c)) {
                    vals.clear();
                    vals.insert(c);

                } else
                    vals.clear();

            } else
                vals.erase(c);

            if (vals != sorts[vSort[v]]) {
                string ns = vSort[v] + "_constraint_propagated_" + to_string(++ns_count);
                vSort[v] = ns;
                sorts[ns] = vals;
            }
        }

        // Rebuild args
        vector<arg_and_type> nvar = vector<arg_and_type>();
        for (arg_and_type x : nm.vars)
            nvar.push_back(make_pair(x.first, vSort[x.first]));

        nm.vars = nvar;
        if (!removeMethod) {
            nm.check_integrity();
            methods.push_back(nm);
        }
    }

    vector<task> oldt = primitive_tasks;
    primitive_tasks.clear();
    for (task t : oldt) {
        task nt = t;
        vector<literal> oldC = t.constraints;
        nt.constraints.clear();

        bool removeTask = false;
        map<string, string> vSort = map<string, string>();
        for (arg_and_type x : t.vars)
            vSort[x.first] = x.second;

        for (literal l : oldC) {
            if (l.arguments[1][0] == '?' && l.arguments[0][0] != '?') {
                // Ensure that the constant is always the second
                swap(l.arguments[0], l.arguments[1]);
            }

            if (l.arguments[1][0] != '?' && l.arguments[0][0] != '?') {
                // Comparing two constants
                if (l.positive == (l.arguments[0][0] != l.arguments[1][0])) {
                    // Constraint cannot be satisfied
                    removeTask = true;
                }

                continue;
            }

            if (l.arguments[1][0] == '?') {
                nt.constraints.push_back(l);
                continue;
            }

            string v = l.arguments[0];
            string c = l.arguments[1];

            // v != c and c is not in the sort of v
            if (!l.positive && !sorts[vSort[v]].count(c))
                continue;

            // Recompute sort
            set<string> vals = sorts[vSort[v]];
            if (l.positive) {
                if (vals.count(c)) {
                    vals.clear();
                    vals.insert(c);

                } else
                    vals.clear();
            } else
                vals.erase(c);

            if (vals != sorts[vSort[v]]) {
                string ns = vSort[v] + "_constraint_propagated_" + to_string(++ns_count);
                vSort[v] = ns;
                sorts[ns] = vals;
            }
        }

        // Rebuild args
        vector<arg_and_type> nvar = vector<arg_and_type>();
        for (arg_and_type x : nt.vars)
            nvar.push_back(make_pair(x.first, vSort[x.first]));
        nt.vars = nvar;
        if (!removeTask)
            primitive_tasks.push_back(nt);
    }
}

void
clean_up_sorts()
{
    // Reduce the number of sorts
    map<set<string>, string> elems_to_sort = map<set<string>, string>();

    vector<task> oldt = primitive_tasks;
    primitive_tasks.clear();
    for (task t : oldt) {
        task nt = t;
        vector<arg_and_type> nvar = vector<arg_and_type>();
        for (arg_and_type x : nt.vars) {
            set<string> elems = sorts[x.second];
            if (!elems_to_sort.count(elems))
                elems_to_sort[elems] = x.second;
            nvar.push_back(make_pair(x.first, elems_to_sort[elems]));
        }

        nt.vars = nvar;
        nt.check_integrity();
        primitive_tasks.push_back(nt);
    }

    vector<task> olda = abstract_tasks;
    abstract_tasks.clear();
    for (task t : olda) {
        task nt = t;
        vector<arg_and_type> nvar = vector<arg_and_type>();
        for (arg_and_type x : nt.vars) {
            set<string> elems = sorts[x.second];
            if (!elems_to_sort.count(elems))
                elems_to_sort[elems] = x.second;
            nvar.push_back(make_pair(x.first, elems_to_sort[elems]));
        }

        nt.vars = nvar;
        nt.check_integrity();
        abstract_tasks.push_back(nt);
    }

    vector<method> oldm = methods;
    methods.clear();
    for (method m : oldm) {
        method nm = m;
        vector<arg_and_type> nvar = vector<arg_and_type>();
        for (arg_and_type x : nm.vars) {
            set<string> elems = sorts[x.second];
            if (!elems_to_sort.count(elems))
                elems_to_sort[elems] = x.second;
            nvar.push_back(make_pair(x.first, elems_to_sort[elems]));
        }

        nm.vars = nvar;
        nm.check_integrity();
        methods.push_back(nm);
    }

    vector<predicate_definition> opred = predicate_definitions;
    predicate_definitions.clear();
    for (predicate_definition p : opred) {
        predicate_definition np = p;
        vector<string> nvar = vector<string>();
        for (string x : np.argument_sorts) {
            set<string> elems = sorts[x];
            if (!elems_to_sort.count(elems))
                elems_to_sort[elems] = x;
            nvar.push_back(elems_to_sort[elems]);
        }

        np.argument_sorts = nvar;
        predicate_definitions.push_back(np);
    }

    for (pair<string, set<map<string, var_declaration>>> c : csorts) {
        set<string> elems = set<string>();
        for (map<string, var_declaration> s : c.second)
            for (pair<string, var_declaration> m : s)
                elems.insert(m.first);

        if (!elems_to_sort.count(elems))
            elems_to_sort[elems] = c.first;
    }

    sorts.clear();
    for (pair<set<string>, string> x : elems_to_sort)
        sorts[x.second] = x.first;
}

void
remove_unnecessary_predicates()
{
    set<string> occuring_preds = set<string>();
    for (task t : primitive_tasks)
        for (literal l : t.prec)
            occuring_preds.insert(l.predicate);

    // Conditions of conditional effects also occur
    for (task t : primitive_tasks)
        for (conditional_effect ceff : t.ceff)
            for (literal l : ceff.condition)
                occuring_preds.insert(l.predicate);

    for (ground_literal gl : goal)
        occuring_preds.insert(gl.predicate);

    vector<predicate_definition> old = predicate_definitions;
    predicate_definitions.clear();

    // Find predicates that do not occur in preconditions
    set<string> removed_predicates = set<string>();
    for (predicate_definition p : old) {
        if (occuring_preds.count(p.name) || p.name == dummy_equal_literal)
            predicate_definitions.push_back(p);
        else
            removed_predicates.insert(p.name);
    }

    // Remove these from primitive tasks and the goal
    vector<task> oldt = primitive_tasks;
    primitive_tasks.clear();
    for (task t : oldt) {
        task nt = t;
        vector<literal> np = vector<literal>();
        // Filter effects
        for (literal l : nt.eff)
            if (!removed_predicates.count(l.predicate))
                np.push_back(l);
        nt.eff = np;
        nt.check_integrity();
        primitive_tasks.push_back(nt);
    }

    // Remove useless predicates from init
    vector<ground_literal> ni = vector<ground_literal>();
    for (ground_literal l : init)
        if (!removed_predicates.count(l.predicate))
            ni.push_back(l);
    init = ni;

    // Remove useless predicates from goal
    vector<ground_literal> ng = vector<ground_literal>();
    for (ground_literal l : goal)
        if (!removed_predicates.count(l.predicate))
            ng.push_back(l);
    goal = ng;
}

void
compile_requests()
{
    for (string v : parsed_requests)
        for (map<string, var_declaration> s : csorts["request"])
            for (pair<string, var_declaration> m : s)
                if (m.first == v) {
                    assert(m.second.vars.size() == 3);

                    request incoming_request = request();
                    incoming_request.id = v;

                    for (arg_and_type v : m.second.vars) {
                        if (v.first == "release_time")
                            incoming_request.release_time = stoi(v.second);
                        else if (v.first == "due_date")
                            incoming_request.due_date = stoi(v.second);
                        else if (v.first == "demand") {
                            v.second.erase(v.second.end() - 1);
                            v.second.erase(v.second.begin());

                            bool skip = true;
                            size_t pos = 0;

                            while ((pos = v.second.find(" ")) != string::npos) {
                                if (!skip)
                                    incoming_request.arguments.push_back(v.second.substr(0, pos));
                                else
                                    for (task t : abstract_tasks)
                                        if (t.name == v.second.substr(0, pos))
                                            incoming_request.demand = t;

                                skip = false;
                                v.second.erase(0, pos + 1);
                            }

                            incoming_request.arguments.push_back(v.second);
                        }
                    }

                    requests.push_back(incoming_request);
                }
}

void
task::check_integrity()
{
    // Variables must have a sort
    for (arg_and_type v : this->vars)
        assert(v.second.size() != 0);

    assert(number_of_original_vars <= int(vars.size()));

    for (literal l : this->prec) {
        bool hasPred = false;
        for (predicate_definition p : predicate_definitions)
            if (p.name == l.predicate)
                hasPred = true;

        if (!hasPred) {
            cerr << "Task " << this->name << " has the predicate \"" << l.predicate
                 << "\" in its precondition, which is not declared." << endl;
            assert(hasPred);
        }

        for (string v : l.arguments) {
            bool hasVar = false;
            for (arg_and_type mv : this->vars) {
                if (mv.first == v)
                    hasVar = true;
                if (mv.first == v.substr(0, v.find(".")))
                    hasVar = true;
                for (pair<string, set<string>> m : constants)
                    for (string s : m.second)
                        if (v == s) {
                            hasVar = true;
                            break;
                        }
            }
            if (!hasVar) {
                cerr << "Task " << this->name << " has the predicate \"" << l.predicate
                     << "\" in its precondition, which has the \
                    argument \""
                     << v << "\", which is unknown." << endl;
                assert(hasVar);
            }
        }
    }

    for (literal l : this->eff) {
        bool hasPred = false;
        for (predicate_definition p : predicate_definitions)
            if (p.name == l.predicate)
                hasPred = true;

        if (!hasPred) {
            cerr << "Task " << this->name << " has the predicate \"" << l.predicate
                 << "\" in its effects, which is not declared." << endl;
            assert(hasPred);
        }

        for (string v : l.arguments) {
            bool hasVar = false;
            for (arg_and_type mv : this->vars) {
                if (mv.first == v)
                    hasVar = true;
                if (mv.first == v.substr(0, v.find(".")))
                    hasVar = true;
                for (pair<string, set<string>> m : constants)
                    for (string s : m.second) {
                        if (v == s) {
                            hasVar = true;
                            break;
                        }
                    }
            }

            if (!hasVar) {
                cerr << "Task " << this->name << " has the predicate \"" << l.predicate
                     << "\" in its effects, which has the argument \
                    \""
                     << v << "\", which is unknown." << endl;
                assert(hasVar);
            }
        }
    }
}

void
method::check_integrity()
{
    set<string> varnames = set<string>();
    for (arg_and_type v : this->vars) {
        varnames.insert(v.first);
        if (v.second.size() == 0)
            cerr << "Variable " << v.first << " has empty sort " << endl;
        assert(v.second.size() != 0);
    }

    assert(varnames.size() == vars.size());
    set<string> ids = set<string>();

    for (plan_step ps : this->ps) {
        assert(task_name_map.count(ps.task));
        ids.insert(ps.id);
        task t = task_name_map[ps.task];
        if (ps.args.size() != t.vars.size()) {
            cerr << "Method " << this->name << " has the subtask (" << ps.id << ") " << ps.task
                 << ". The task is declared with " << t.vars.size() << " parameters, but "
                 << ps.args.size() << " are given in the method." << endl;
            assert(false);
        }

        for (string v : ps.args) {
            bool found = false;
            for (arg_and_type vd : this->vars)
                found |= vd.first == v;
            if (!found) {
                cerr << "Method " << this->name << " has the subtask (" << ps.id << ") " << ps.task
                     << ". It has a parameter " << v << " which is not declared in the method."
                     << endl;
            }
            assert(found);
        }
    }

    for (auto [b, a] : this->ordering) {
        assert(ids.count(b));
        assert(ids.count(a));
        (void)a;
        (void)b;
    }

    for (sync_constraints sc : this->synchronize_constraints) {
        assert(sc.lower_bound <= sc.upper_bound);
        assert(ids.count(sc.task1));
        assert(ids.count(sc.task2));
    }
}

bool
method::is_sub_group(set<string>& sset, set<string>& beforeID, set<string>& afterID)
{
    if (sset.size() == 1) {
        string id = *sset.begin();
        for (auto [b, a] : this->ordering)
            if (b == id)
                afterID.insert(a);
            else if (a == id)
                beforeID.insert(b);

        return true;
    }

    this->compute_adj_matrix();
    beforeID.clear();
    afterID.clear();

    for (plan_step& k : this->ps) {
        if (sset.count(k.id))
            continue;

        // 1 is unord, 2 is before k , 3 is after k
        int myrel = 0;
        for (const string& s : sset) {
            if (this->adj_matrix[s].count(k.id)) {
                if (myrel != 0 && myrel != 2)
                    return false;
                myrel = 2;

            } else if (this->adj_matrix[k.id].count(s)) {
                if (myrel != 0 && myrel != 3)
                    return false;
                myrel = 3;

            } else {
                // Unrelated task, but set relation
                if (myrel > 1)
                    return false;
                myrel = 1;
            }
        }

        if (myrel == 2)
            afterID.insert(k.id);
        if (myrel == 3)
            beforeID.insert(k.id);
    }

    return true;
}

void
method::compute_adj_matrix()
{
    if (this->adj_matrix_computed)
        return;
    this->adj_matrix_computed = true;
    this->adj_matrix.clear();

    for (auto [t1, t2] : this->ordering)
        this->adj_matrix[t1].insert(t2);

    for (plan_step& k : this->ps)
        for (plan_step& i : this->ps)
            for (plan_step& j : this->ps)
                if (this->adj_matrix[i.id].count(k.id) && this->adj_matrix[k.id].count(j.id))
                    this->adj_matrix[i.id].insert(j.id);
}

void
add_consts_to_set(general_formula* f, set<string>& const_set)
{
    if (!f)
        return;
    if (f->type == EQUAL || f->type == NOTEQUAL) {
        if (f->arg1[0] != '?')
            const_set.insert(f->arg1);
        if (f->arg2[0] != '?')
            const_set.insert(f->arg2);
    }

    for (general_formula* sub : f->subformulae)
        add_consts_to_set(sub, const_set);
}

void
add_consts_to_set(additional_variables additionalVars, set<string>& const_set)
{
    for (arg_and_type varDecl : additionalVars) {
        // Determine const of this sort
        assert(sorts[varDecl.second].size() == 1);
        const_set.insert(*(sorts[varDecl.second].begin()));
    }
}

set<string>
compute_constants_in_domain()
{
    // Determine which constants need to be declared in the domain
    set<string> constants_in_domain = set<string>();
    for (parsed_task& at : parsed_abstract) {
        for (parsed_method& method : parsed_methods[at.name]) {
            // Determine which variables are actually constants
            add_consts_to_set(method.newVarForAT, constants_in_domain);
            if (!method.functional) {
                for (sub_task* st : method.tn->tasks)
                    add_consts_to_set(st->arguments->newVar, constants_in_domain);

                add_consts_to_set(method.tn->constraint, constants_in_domain);
            }
            add_consts_to_set(method.prec, constants_in_domain);
            add_consts_to_set(method.eff, constants_in_domain);
        }
    }

    // Constants in primitives
    for (parsed_task prim : parsed_primitive) {
        add_consts_to_set(prim.prec->variables_for_constants(), constants_in_domain);
        add_consts_to_set(prim.eff->variables_for_constants(), constants_in_domain);
    }

    return constants_in_domain;
}
