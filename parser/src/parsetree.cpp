#include <cassert>
#include <iostream>

#include "cwa.hpp"
#include "parsetree.hpp"

int task_id_counter = 0;
int global_exists_variable_counter = 0;

string sort_definition::to_string() const {
    string result = "Sort Definition: ";

    result += "\n\t Parent Sort = " + parent_sort + "\n";

    result += "\t Has Parent Sort = ";
    if (has_parent_sort)
        result += "true\n";
    else
        result += "false\n";

    result += "\t Vars = " + vars.to_string() + "\n";

    result += "\t Declared Sorts = \n";
    for (string s : declared_sorts) result += "\t\t " + s + "\n";

    return result;
}

string predicate_definition::to_string() const {
    string result = "Predicate Definition: ";

    result += "\n\t Name = " + name + "\n";

    result += "\t Functional = ";
    if (functional)
        result += "true\n";
    else
        result += "false\n";

    result += "\t Arguments = \n";
    for (string s : argument_sorts) result += "\t\t " + s + "\n";

    return result;
}

string var_and_const::to_string() const {
    string result = "Var And Const: ";

    result += "\n\t Vars = \n";
    for (string s : vars) result += "\t\t " + s + "\n";

    result += "\t New Variables = \n";
    for (arg_and_type arg : newVar)
        result += "\t\t (" + arg.first + ", " + arg.second + ")\n";

    return result;
}

string function_expression::to_string() const {
    string result = "Function Expression: ";
    result += "\n\t Value = " + std::to_string(value) + "\n";
    result += "\t Name = " + name + "\n";
    result += "\t isOnlyValue = ";
    if (isOnlyValue)
        result += "true\n";
    else
        result += "false\n";

    result += "\t Arguments = " + arguments.to_string() + "\n";

    return result;
}

string general_formula::to_string() const {
    string result = "General Formula: ";

    result += "\n\t Functional = ";
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

    result += "\t Type = ";
    if (type == EMPTY)
        result += "empty\n";
    else if (type == AND)
        result += "and\n";
    else if (type == OR)
        result += "or\n";
    else if (type == FORALL)
        result += "for all\n";
    else if (type == EXISTS)
        result += "exists\n";
    else if (type == ATOM)
        result += "atom\n";
    else if (type == NOTATOM)
        result += "not atom\n";
    else if (type == SYNC)
        result += "sync\n";
    else if (type == EQUAL)
        result += "equal\n";
    else if (type == NOTEQUAL)
        result += "not equal\n";
    else if (type == WHEN)
        result += "when\n";
    else if (type == VALUE)
        result += "value\n";
    else if (type == COST)
        result += "cost\n";
    else
        result += "cost change\n";

    result += "\t Arguments " + arguments.to_string() + "\n";
    result += "\t Q Variables = " + qvariables.to_string() + "\n";

    result += "\t Subformulae = \n";
    for (general_formula *subf : subformulae)
        result += "\t\t " + subf->to_string() + "\n";

    result += "\t Value Increased = " + value_increased->to_string() + "\n";

    result += "\t Increase Amount = " + increase_amount->to_string() + "\n";

    result += "\t Value = " + std::to_string(value) + "\n";
    result += "\t Arg1 = " + arg1 + "\n";
    result += "\t Arg2 = " + arg2 + "\n";

    result += "\t Lower Bound = " + std::to_string(lb) + "\n";
    result += "\t Upper Bound = " + std::to_string(ub) + "\n";
    result += "\t Task 1 = " + task1 + "\n";
    result += "\t Task 2 = " + task2 + "\n";
    result += "\t Contiguous = ";
    if (contiguous)
        result += "true\n";
    else
        result += "false\n";

    return result;
}

string parsed_task::to_string() const {
    string result = "Parsed Task: ";
    result += "\n\t Duration = " + std::to_string(dur) + "\n";
    result += "\t Name = " + name + "\n";
    result += "\t Precondition = " + prec->to_string() + "\n";
    result += "\t Effect = " + eff->to_string() + "\n";
    result += "\t Arguments = " + arguments->to_string() + "\n";

    return result;
}

string sub_task::to_string() const {
    string result = "Sub Task: ";
    result += "\n\t Id = " + id + "\n";
    result += "\t Task Name = " + task + "\n";
    result += "\t Arguments = " + arguments->to_string() + "\n";

    return result;
}

string parsed_task_network::to_string() const {
    string result = "Parsed Task Network: ";

    result += "\n\t Tasks = \n";
    for (sub_task *st : tasks) result += "\t\t " + st->to_string() + "\n";

    result += "\t Constraint = " + constraint->to_string() + "\n";

    result += "\t Ordering = \n";
    for (arg_and_type *arg : ordering)
        result += "\t\t (" + arg->first + ", " + arg->second + ")\n";

    result += "\t Synchronization Constraints = \n";
    for (general_formula *sc : synchronize_constraints)
        result += "\t\t " + sc->to_string() + "\n";

    return result;
}

string parsed_method::to_string() const {
    string result = "Parsed Method: ";

    result += "\n\t Name = " + name + "\n";

    result += "\t Functional = ";
    if (functional)
        result += "true\n";
    else
        result += "false\n";

    result += "\t Vars = " + vars->to_string() + "\n";
    result += "\t Precondition = " + prec->to_string() + "\n";
    result += "\t Effect = " + eff->to_string() + "\n";
    result += "\t Task Network = " + tn->to_string() + "\n";

    result += "\t Abstract Task Arguments = \n";
    for (string arg : atArguments) result += "\t\t " + arg + "\n";

    result += "\t New Variables for Abstract Task = \n";
    for (arg_and_type arg : newVarForAT)
        result += "\t\t (" + arg.first + ", " + arg.second + ")\n";

    return result;
}

void general_formula::negate() {
    if (this->type == EMPTY)
        return;
    else if (this->type == AND)
        this->type = OR;
    else if (this->type == OR)
        this->type = AND;
    else if (this->type == FORALL)
        this->type = EXISTS;
    else if (this->type == EXISTS)
        this->type = FORALL;
    else if (this->type == EQUAL)
        this->type = NOTEQUAL;
    else if (this->type == NOTEQUAL)
        this->type = EQUAL;
    else if (this->type == ATOM)
        this->type = NOTATOM;
    else if (this->type == NOTATOM)
        this->type = ATOM;
    // Synchronization constraints cannot be negated
    else if (this->type == SYNC)
        assert(false);
    // Conditional effect cannot be negated
    else if (this->type == WHEN)
        assert(false);
    // Conditional effect cannot be negated
    else if (this->type == VALUE)
        assert(false);
    // Conditional effect cannot be negated
    else if (this->type == COST_CHANGE)
        assert(false);
    // Conditional effect cannot be negated
    else if (this->type == COST)
        assert(false);

    for (general_formula *sub : this->subformulae) sub->negate();
}

bool general_formula::isEmpty() {
    if (this->type == EMPTY) return true;

    if (this->type == ATOM) return false;
    if (this->type == NOTATOM) return false;
    if (this->type == EQUAL) return false;
    if (this->type == NOTEQUAL) return false;

    if (this->type == SYNC) return false;

    if (this->type == VALUE) return false;
    if (this->type == COST) return false;
    if (this->type == COST_CHANGE) return false;

    for (general_formula *sub : this->subformulae)
        if (!sub->isEmpty()) return false;

    return true;
}

bool general_formula::hasEquals() {
    if (this->type == EQUAL) return true;
    if (this->type == NOTEQUAL) return true;

    for (general_formula *sub : this->subformulae)
        if (sub->hasEquals()) return true;

    return false;
}

bool general_formula::hasExists() {
    if (this->type == EXISTS) return true;

    for (general_formula *sub : this->subformulae)
        if (sub->hasExists()) return true;

    return false;
}

bool general_formula::hasForall() {
    if (this->type == FORALL) return true;

    for (general_formula *sub : this->subformulae)
        if (sub->hasForall()) return true;

    return false;
}

bool general_formula::isFunctional() {
    for (predicate_definition p : predicate_definitions) {
        if (p.functional)
            if (p.name == this->predicate) return true;
    }

    return false;
}

set<string> general_formula::occuringUnQuantifiedVariables() {
    set<string> ret = set<string>();

    if (this->type == EMPTY) return ret;

    if (this->type == AND || this->type == OR || this->type == WHEN) {
        for (general_formula *sub : this->subformulae) {
            set<string> subres = sub->occuringUnQuantifiedVariables();
            ret.insert(subres.begin(), subres.end());
        }

        return ret;
    }

    if (this->type == FORALL || this->type == EXISTS) {
        set<string> subres =
            this->subformulae[0]->occuringUnQuantifiedVariables();

        for (auto [var, _] : this->qvariables.vars) {
            (void)_;
            subres.erase(var);
        }

        return subres;
    }

    if (this->type == ATOM || this->type == NOTATOM) {
        ret.insert(this->arguments.vars.begin(), this->arguments.vars.end());
        return ret;
    }

    if (this->type == EQUAL || this->type == NOTEQUAL) {
        ret.insert(this->arg1);
        ret.insert(this->arg2);
        return ret;
    }

    // Things that I don't want to support
    if (this->type == SYNC) assert(false);
    if (this->type == VALUE) assert(false);
    if (this->type == COST_CHANGE) assert(false);
    if (this->type == COST) assert(false);

    assert(false);
    return ret;
}

additional_variables general_formula::variables_for_constants() {
    additional_variables ret = additional_variables();
    ret.insert(this->arguments.newVar.begin(), this->arguments.newVar.end());
    for (general_formula *sf : this->subformulae) {
        additional_variables sfr = sf->variables_for_constants();
        ret.insert(sfr.begin(), sfr.end());
    }

    return ret;
}

string sort_for_const(string c) {
    string s = "sort_for_" + c;
    sorts[s].insert(c);
    return s;
}

general_formula *general_formula::copyReplace(map<string, string> &replace) {
    general_formula *ret = new general_formula();
    ret->type = this->type;
    ret->timed = this->timed;
    ret->functional = this->functional;
    ret->temp_qual = this->temp_qual;
    ret->qvariables = this->qvariables;
    ret->arg1 = this->arg1;
    if (replace.count(ret->arg1)) ret->arg1 = replace[ret->arg1];
    ret->arg2 = this->arg2;
    if (replace.count(ret->arg2)) ret->arg2 = replace[ret->arg2];
    ret->value = this->value;

    ret->lb = this->lb;
    ret->ub = this->ub;
    ret->task1 = this->task1;
    if (replace.count(ret->task1)) ret->task1 = replace[ret->task1];
    ret->task2 = this->task2;
    if (replace.count(ret->task2)) ret->task2 = replace[ret->task2];

    ret->predicate = this->predicate;
    for (general_formula *sub : this->subformulae)
        ret->subformulae.push_back(sub->copyReplace(replace));

    ret->arguments = this->arguments;
    for (unsigned int i = 0; i < ret->arguments.vars.size(); i++)
        if (replace.count(ret->arguments.vars[i]))
            ret->arguments.vars[i] = replace[ret->arguments.vars[i]];

    ret->functional = ret->isFunctional();

    return ret;
}

bool general_formula::isDisjunctive() {
    if (this->type == EMPTY)
        return false;
    else if (this->type == EQUAL)
        return false;
    else if (this->type == NOTEQUAL)
        return false;
    else if (this->type == ATOM)
        return false;
    else if (this->type == NOTATOM)
        return false;
    else if (this->type == SYNC)
        return false;
    else if (this->type == VALUE)
        return false;
    else if (this->type == COST_CHANGE)
        return false;
    else if (this->type == COST)
        return false;
    // OR and WHEN are disjunctive
    else if (this->type == OR)
        return true;
    else if (this->type == WHEN)
        return true;

    // else AND, FORALL, EXISTS
    for (general_formula *sub : this->subformulae)
        if (sub->isDisjunctive()) return true;

    return false;
}

// Hard expansion of formulae. This can grow up to exponentially, but is
// currently the only thing we can do about disjunctions. This will also handle
// forall and exists quantors by expansion. Sorts must have been parsed and
// expanded prior to this call
vector<
    pair<pair<vector<variant<literal, conditional_effect> >, vector<literal> >,
         additional_variables> >
general_formula::expand(bool compileConditionalEffects) {
    vector<pair<
        pair<vector<variant<literal, conditional_effect> >, vector<literal> >,
        additional_variables> >
        ret = vector<pair<pair<vector<variant<literal, conditional_effect> >,
                               vector<literal> >,
                          additional_variables> >();

    if (this->type == EMPTY ||
        (this->subformulae.size() == 0 &&
         (this->type == AND || this->type == OR || this->type == FORALL ||
          this->type == EXISTS))) {
        vector<variant<literal, conditional_effect> > empty1 =
            vector<variant<literal, conditional_effect> >();
        vector<literal> empty2 = vector<literal>();
        additional_variables none = additional_variables();
        ret.push_back(make_pair(make_pair(empty1, empty2), none));
        return ret;
    }

    // Generate a big conjunction for forall and expand it
    if (this->type == FORALL) {
        general_formula and_formula = general_formula();
        and_formula.type = AND;
        and_formula.functional = false;
        auto [var_replace, avs] = forallVariableReplacement();

        for (map<string, string> r : var_replace)
            and_formula.subformulae.push_back(
                this->subformulae[0]->copyReplace(r));

        ret = and_formula.expand(compileConditionalEffects);

        // Add variables
        for (unsigned int i = 0; i < ret.size(); i++)
            ret[i].second.insert(avs.begin(), avs.end());
    }

    // Add additional variables for every quantified variable. We have to do
    // this for every possible instance of the precondition below
    if (this->type == EXISTS) {
        map<string, string> var_replace = existsVariableReplacement();
        this->subformulae[0] = this->subformulae[0]->copyReplace(var_replace);

        vector<pair<pair<vector<variant<literal, conditional_effect> >,
                         vector<literal> >,
                    additional_variables> >
            cur = this->subformulae[0]->expand(compileConditionalEffects);

        for (pair<string, string> var : this->qvariables.vars) {
            vector<pair<pair<vector<variant<literal, conditional_effect> >,
                             vector<literal> >,
                        additional_variables> >
                prev = cur;
            cur.clear();
            string newName = var.first + "_" +
                             std::to_string(++global_exists_variable_counter);

            for (auto old : prev) {
                pair<pair<vector<variant<literal, conditional_effect> >,
                          vector<literal> >,
                     additional_variables>
                    combined = old;

                combined.second.insert(
                    make_pair(var_replace[var.first], var.second));
                cur.push_back(combined);
            }
        }

        return cur;
    }

    vector<vector<pair<
        pair<vector<variant<literal, conditional_effect> >, vector<literal> >,
        additional_variables> > >
        subresults = vector<
            vector<pair<pair<vector<variant<literal, conditional_effect> >,
                             vector<literal> >,
                        additional_variables> > >();

    for (general_formula *sub : this->subformulae)
        subresults.push_back(sub->expand(compileConditionalEffects));

    // Just add all disjuncts to set of literals
    if (this->type == OR)
        for (auto subres : subresults)
            for (auto res : subres) ret.push_back(res);

    if (this->type == AND) {
        vector<pair<pair<vector<variant<literal, conditional_effect> >,
                         vector<literal> >,
                    additional_variables> >
            cur = subresults[0];

        for (unsigned int i = 1; i < subresults.size(); i++) {
            vector<pair<pair<vector<variant<literal, conditional_effect> >,
                             vector<literal> >,
                        additional_variables> >
                prev = cur;
            cur.clear();
            for (auto next : subresults[i]) {
                for (auto old : prev) {
                    pair<pair<vector<variant<literal, conditional_effect> >,
                              vector<literal> >,
                         additional_variables>
                        combined = old;

                    for (variant<literal, conditional_effect> l :
                         next.first.first)
                        combined.first.first.push_back(l);

                    for (literal l : next.first.second)
                        combined.first.second.push_back(l);

                    for (auto v : next.second) combined.second.insert(v);

                    cur.push_back(combined);
                }
            }
        }

        ret = cur;
    }

    if (this->type == ATOM || this->type == NOTATOM || this->type == COST) {
        vector<variant<literal, conditional_effect> > ls =
            vector<variant<literal, conditional_effect> >();
        literal l = this->atomLiteral();
        ls.push_back(l);

        additional_variables vars = this->arguments.newVar;
        vector<literal> empty = vector<literal>();
        ret.push_back(make_pair(make_pair(ls, empty), vars));
    }

    if (this->type == VALUE) {
        vector<variant<literal, conditional_effect> > ls =
            vector<variant<literal, conditional_effect> >();
        literal l = literal();
        l.positive = this->type == ATOM;
        l.isConstantCostExpression = true;
        l.costValue = this->value;
        ls.push_back(l);

        additional_variables vars = additional_variables();
        vector<literal> empty = vector<literal>();
        ret.push_back(make_pair(make_pair(ls, empty), vars));
    }

    if (this->type == COST_CHANGE) {
        assert(subresults.size() == 2);
        assert(subresults[0].size() == 1);
        assert(subresults[0][0].first.first.size() == 1);
        assert(holds_alternative<literal>(subresults[0][0].first.first[0]));
        assert(get<literal>(subresults[0][0].first.first[0]).predicate ==
               metric_target);
        assert(get<literal>(subresults[0][0].first.first[0]).arguments.size() ==
               0);

        assert(subresults[1].size() == 1);
        assert(subresults[1][0].first.first.size() == 1);
        get<literal>(subresults[1][0].first.first[0]).isCostChangeExpression =
            true;
        ret.push_back(subresults[1][0]);
    }

    // Add dummy literal for equal and not equal constraints
    if (this->type == EQUAL || this->type == NOTEQUAL) {
        vector<variant<literal, conditional_effect> > ls =
            vector<variant<literal, conditional_effect> >();
        literal l = this->equalsLiteral();
        ls.push_back(l);

        // No new vars. Never
        additional_variables vars = additional_variables();
        vector<literal> empty = vector<literal>();
        ret.push_back(make_pair(make_pair(ls, empty), vars));
    }

    if (this->type == WHEN) {
        // Condition might be a disjunction
        // Effect might have multiple expansions (i.e. be disjunctive)
        // We here assume that this means angelic non-determinism
        for (auto expanded_condition : subresults[0])
            for (auto expanded_effect : subresults[1]) {
                vector<variant<literal, conditional_effect> > eff =
                    expanded_effect.first.first;
                vector<literal> cond = expanded_effect.first.second;

                for (variant<literal, conditional_effect> x :
                     expanded_condition.first.first)
                    if (holds_alternative<literal>(x))
                        cond.push_back(get<literal>(x));
                    // Conditional effect inside condition
                    else
                        assert(false);

                additional_variables avs = expanded_effect.second;
                avs.insert(expanded_condition.second.begin(),
                           expanded_condition.second.end());

                if (compileConditionalEffects)
                    ret.push_back(make_pair(make_pair(eff, cond), avs));
                else {
                    // We have to build the conditional effects
                    vector<variant<literal, conditional_effect> > newEff =
                        vector<variant<literal, conditional_effect> >();
                    for (variant<literal, conditional_effect> &e : eff) {
                        if (holds_alternative<literal>(e))
                            newEff.push_back(
                                conditional_effect(cond, get<literal>(e)));
                        else {
                            // Nested conditional effect
                            vector<literal> innercond =
                                get<conditional_effect>(e).condition;
                            literal innerE = get<conditional_effect>(e).effect;
                            innercond.insert(innercond.end(), cond.begin(),
                                             cond.end());

                            newEff.push_back(
                                conditional_effect(innercond, innerE));
                        }
                    }

                    vector<literal> empty = vector<literal>();
                    ret.push_back(make_pair(make_pair(newEff, empty), avs));
                }
            }

        if (compileConditionalEffects) {
            // Remove conditional effects by compiling them into multiple
            // actions
            general_formula cond = *(this->subformulae[0]);
            cond.negate();
            for (auto expanded_condition : cond.expand(false)) {
                // Condition cannot contain conditional effect!
                expanded_condition.first.second.clear();
                for (variant<literal, conditional_effect> x :
                     expanded_condition.first.first)
                    if (holds_alternative<literal>(x))
                        expanded_condition.first.second.push_back(
                            get<literal>(x));
                    // Conditional effect inside condition
                    else
                        assert(false);

                expanded_condition.first.first.clear();
                ret.push_back(expanded_condition);
            }
        }
    }

    return ret;
}

literal general_formula::equalsLiteral() {
    assert(this->type == EQUAL || this->type == NOTEQUAL);

    literal l = literal();
    l.func_literal = 0;
    l.timed = this->timed;
    l.temp_qual = this->temp_qual;
    l.positive = this->type == EQUAL;
    if (this->type == EQUAL || this->type == NOTEQUAL)
        l.predicate = dummy_equal_literal;
    l.isConstantCostExpression = false;
    l.isCostChangeExpression = false;
    l.arguments.push_back(this->arg1);
    l.arguments.push_back(this->arg2);
    return l;
}

literal general_formula::atomLiteral() {
    assert(this->type == ATOM || this->type == NOTATOM || this->type == COST);

    literal l = literal();
    l.func_literal = 0;
    l.timed = this->timed;
    l.temp_qual = this->temp_qual;
    l.positive = this->type == ATOM;
    l.functional = this->isFunctional();
    l.isConstantCostExpression = false;
    l.isCostChangeExpression = false;
    l.predicate = this->predicate;
    l.arguments = this->arguments.vars;

    return l;
}

pair<vector<map<string, string> >, additional_variables>
general_formula::forallVariableReplacement() {
    assert(this->type == FORALL);

    additional_variables avs = additional_variables();
    vector<map<string, string> > var_replace = vector<map<string, string> >();
    map<string, string> empty = map<string, string>();
    var_replace.push_back(empty);
    int counter = 0;
    for (arg_and_type var : this->qvariables.vars) {
        vector<map<string, string> > old_var_replace = var_replace;
        var_replace.clear();

        for (string c : sorts[var.second]) {
            string newSort = sort_for_const(c);
            string newVar = var.first + "_" + std::to_string(counter);
            counter++;
            avs.insert(make_pair(newVar, newSort));
            for (map<string, string> old : old_var_replace) {
                // Add new variable binding
                old[var.first] = newVar;
                var_replace.push_back(old);
            }
        }
    }

    return make_pair(var_replace, avs);
}

map<string, string> general_formula::existsVariableReplacement() {
    map<string, string> var_replace = map<string, string>();
    for (arg_and_type var : this->qvariables.vars) {
        string newName =
            var.first + "_" + std::to_string(++global_exists_variable_counter);
        var_replace[var.first] = newName;
    }

    return var_replace;
}

void compile_goal_into_action() {
    if (goal_formula == NULL) return;
    if (goal_formula->isEmpty()) return;

    parsed_task t = parsed_task();
    t.dur = 0;
    t.name = "goal_action";
    t.arguments = new var_declaration();
    t.prec = goal_formula;
    t.eff = new general_formula();
    t.eff->type = EMPTY;
    t.eff->functional = false;
    parsed_primitive.push_back(t);

    // Add as last task into top method
    assert(parsed_methods["__top"].size() == 1);
    parsed_method m = parsed_methods["__top"][0];
    parsed_methods["__top"].clear();
    sub_task *g = new sub_task();
    g->id = "__goal_id";
    g->task = t.name;
    g->arguments = new var_and_const();

    // Ordering
    for (sub_task *st : m.tn->tasks) {
        arg_and_type *o = new arg_and_type();
        o->first = st->id;
        o->second = g->id;
        m.tn->ordering.push_back(o);
    }

    m.tn->tasks.push_back(g);

    // Add the method back in
    parsed_methods["__top"].push_back(m);

    // Clear the goal formula
    goal_formula = NULL;
}
