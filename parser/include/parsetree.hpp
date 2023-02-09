#ifndef __PARSETREE

#define __PARSETREE

#include <map>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "domain.hpp"

using namespace std;

typedef set<arg_and_type> additional_variables;

struct sort_definition {
    string parent_sort;
    bool has_parent_sort;
    var_declaration vars;
    vector<string> declared_sorts;

    sort_definition() {
        parent_sort = "";
        has_parent_sort = false;
        vars = var_declaration();
        declared_sorts = vector<string>();
    }

    string to_string() const;
};

struct predicate_definition {
    string name;
    bool functional;
    vector<string> argument_sorts;

    predicate_definition() {
        name = "";
        functional = false;
        argument_sorts = vector<string>();
    }

    string to_string() const;
};

struct var_and_const {
    vector<string> vars;
    additional_variables newVar;

    var_and_const() {
        vars = vector<string>();
        newVar = additional_variables();
    }

    string to_string() const;
};

struct function_expression {
    int value;
    string name;
    bool isOnlyValue;
    var_and_const arguments;

    function_expression() {
        value = 0;
        name = "";
        isOnlyValue = false;
        arguments = var_and_const();
    }

    string to_string() const;
};

enum formula_type {

    EMPTY,
    AND,
    OR,
    FORALL,
    EXISTS,
    ATOM,
    NOTATOM,
    SYNC,  // Formulae
    EQUAL,
    NOTEQUAL,
    WHEN,  // Conditional effect
    VALUE,
    COST,
    COST_CHANGE  // Cost statement

};

class general_formula {
   public:
    bool functional;
    string predicate;
    timed_type timed;
    formula_type type;
    var_and_const arguments;
    var_declaration qvariables;
    temporal_qualifier_type temp_qual;
    vector<general_formula*> subformulae;
    function_expression *value_increased, *increase_amount;

    int value;
    string arg1;
    string arg2;

    int lb;
    int ub;
    string task1;
    string task2;
    bool contiguous;

    void negate();
    bool isEmpty();
    bool hasEquals();
    bool hasExists();
    bool hasForall();
    bool isFunctional();
    bool isDisjunctive();
    literal atomLiteral();
    literal equalsLiteral();
    set<string> occuringUnQuantifiedVariables();
    additional_variables variables_for_constants();
    map<string, string> existsVariableReplacement();
    general_formula* copyReplace(map<string, string>& replace);

    pair<vector<map<string, string> >, additional_variables>
    forallVariableReplacement();

    /* First is effect, second is additional precondition for that effect
            If it is an uncompiled conditional effect, the additional
    precondition will be empty */
    vector<pair<
        pair<vector<variant<literal, conditional_effect> >, vector<literal> >,
        additional_variables> >
    expand(bool compileConditionalEffects);

    general_formula() {
        predicate = "";
        type = EMPTY;
        timed = START;
        temp_qual = AT;
        functional = false;
        value_increased = nullptr;
        increase_amount = nullptr;
        arguments = var_and_const();
        qvariables = var_declaration();
        subformulae = vector<general_formula*>();

        value = 0;
        arg1 = "";
        arg2 = "";

        lb = -inf_int;
        ub = inf_int;
        task1 = "";
        task2 = "";
        contiguous = false;
    }

    string to_string() const;
};

struct parsed_task {
    float dur;
    string name;
    general_formula* eff;
    general_formula* prec;
    var_declaration* arguments;

    parsed_task() {
        dur = 0.0;
        name = "";
        eff = nullptr;
        prec = nullptr;
        arguments = nullptr;
    }

    string to_string() const;
};

struct sub_task {
    string id;
    string task;
    var_and_const* arguments;

    sub_task() {
        id = "";
        task = "";
        arguments = nullptr;
    }

    string to_string() const;
};

struct parsed_task_network {
    vector<sub_task*> tasks;
    general_formula* constraint;
    vector<arg_and_type*> ordering;
    vector<general_formula*> synchronize_constraints;

    parsed_task_network() {
        constraint = nullptr;
        tasks = vector<sub_task*>();
        ordering = vector<arg_and_type*>();
        synchronize_constraints = vector<general_formula*>();
    }

    string to_string() const;
};

struct parsed_method {
    string name;
    bool functional;
    general_formula* eff;
    var_declaration* vars;
    general_formula* prec;
    parsed_task_network* tn;
    vector<string> atArguments;
    additional_variables newVarForAT;

    parsed_method() {
        name = "";
        tn = nullptr;
        eff = nullptr;
        vars = nullptr;
        prec = nullptr;
        functional = false;
        atArguments = vector<string>();
        newVarForAT = additional_variables();
    }

    string to_string() const;
};

// Global places to put data structures
extern string domain_name;
extern int task_id_counter;
extern string problem_name;
extern string metric_target;
extern vector<string> parsed_requests;
extern vector<parsed_task> parsed_abstract;
extern vector<parsed_task> parsed_primitive;
extern vector<sort_definition> sort_definitions;
extern vector<predicate_definition> predicate_definitions;
extern map<string, vector<parsed_method> > parsed_methods;
extern vector<pair<predicate_definition, string> > parsed_functions;

string sort_for_const(string c);
void compile_goal_into_action();

#endif
