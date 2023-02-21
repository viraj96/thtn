#ifndef __DOMAIN

#define __DOMAIN

#include <boost/function.hpp>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

typedef pair<string, string> arg_and_type;

const string dummy_domain_name = "d_name";
const string dummy_problem_name = "p_name";
const string dummy_equal_literal = "__equal";
const string dummy_function_type = "__none";
const string numeric_funtion_type = "number";
const int inf_int = numeric_limits<int>::max();
const string method_precondition_action_name = "__method_precondition_";

enum temporal_qualifier_type { AT, OVER };

enum timed_type { START, END, ALL };

struct var_declaration {
    vector<arg_and_type> vars;

    var_declaration() { vars = vector<arg_and_type>(); }

    string to_string() const;
};

bool operator<(const var_declaration &left, const var_declaration &right);

struct object_state {
    string parent;
    string object_name;
    var_declaration attribute_types;
    var_declaration attribute_states;

    string to_string() const;
};

typedef map<string, object_state> world_state;

typedef boost::function<bool(vector<string>, world_state *)> fpredicate;

struct literal {
    bool positive;
    bool functional;
    timed_type timed;
    string predicate;
    fpredicate func_literal;
    vector<string> arguments;
    temporal_qualifier_type temp_qual;

    int costValue;
    bool isCostChangeExpression;
    bool isConstantCostExpression;

    literal() {
        timed = START;
        temp_qual = AT;
        predicate = "";
        func_literal = 0;
        positive = false;
        functional = false;
        arguments = vector<string>();

        costValue = 0;
        isCostChangeExpression = false;
        isConstantCostExpression = false;
    }

    string to_string() const;
};

bool operator<(const literal &left, const literal &right);
bool operator==(const literal &left, const literal &right);

struct conditional_effect {
    literal effect;
    vector<literal> condition;

    conditional_effect() {
        effect = literal();
        condition = vector<literal>();
    }

    conditional_effect(vector<literal> cond, literal eff) {
        condition = cond;
        effect = eff;
    }

    string to_string() const;
};

struct task {
    string name;
    float duration;

    // The first N variables are original, i.e. exist in the HDDL input file.
    // The rest are artificial and was added by this parser for compilation
    bool artificial;
    int number_of_original_vars;

    vector<literal> eff;
    vector<literal> prec;
    vector<arg_and_type> vars;
    vector<literal> constraints;
    vector<literal> costExpression;
    vector<conditional_effect> ceff;

    task() {
        name = "";
        duration = 0;
        artificial = false;
        number_of_original_vars = 0;

        eff = vector<literal>();
        prec = vector<literal>();
        vars = vector<arg_and_type>();
        constraints = vector<literal>();
        costExpression = vector<literal>();
        ceff = vector<conditional_effect>();
    }

    void check_integrity();
    string to_string() const;
};

bool operator<(const task &left, const task &right);

struct plan_step {
    string id;
    string task;
    vector<string> args;

    plan_step() {
        id = "";
        task = "";
        args = vector<string>();
    }

    string to_string() const;
};

bool operator<(const plan_step &left, const plan_step &right);

struct sync_constraints {
    string task1;
    string task2;
    bool contiguous;
    int lower_bound;
    int upper_bound;

    sync_constraints() {
        task1 = "";
        task2 = "";
        contiguous = false;
        upper_bound = inf_int;
        lower_bound = -inf_int;
    }

    string to_string() const;
};

typedef boost::function<vector<pair<task, var_declaration> >(vector<string>,
                                                             world_state *)>
    fmethod;

struct method {
    string at;
    string name;
    bool functional;
    fmethod func_method;
    vector<plan_step> ps;
    vector<string> atargs;
    vector<arg_and_type> vars;
    vector<literal> constraints;
    vector<arg_and_type> ordering;
    vector<sync_constraints> synchronize_constraints;

    method() {
        at = "";
        name = "";
        func_method = 0;
        functional = false;
        ps = vector<plan_step>();
        atargs = vector<string>();
        vars = vector<arg_and_type>();
        constraints = vector<literal>();
        ordering = vector<arg_and_type>();
        synchronize_constraints = vector<sync_constraints>();

        adj_matrix_computed = false;
        adj_matrix = map<string, set<string> >();
    }

    void check_integrity();
    string to_string() const;
    bool is_sub_group(set<string> &sset, set<string> &beforeID,
                      set<string> &afterID);

   private:
    bool adj_matrix_computed;
    map<string, set<string> > adj_matrix;

    void compute_adj_matrix();
};

struct request {
    string id;
    task demand;
    int due_date;
    int release_time;
    vector<string> arguments;

    request() {
        id = "";
        demand = task();
        due_date = 0;
        release_time = 0;
        arguments = vector<string>();
    }

    string to_string() const;
};

// Sort name and set of elements
extern vector<method> methods;
extern vector<request> requests;
extern vector<task> abstract_tasks;
extern vector<task> primitive_tasks;
extern map<string, task> task_name_map;
extern map<string, set<string> > sorts;
extern map<string, set<string> > constants;
extern map<string, set<map<string, var_declaration> > > csorts;

void expand_sorts();
void clean_up_sorts();
void compile_requests();
void reduce_constraints();
void addAbstractTask(task &t);
void addPrimitiveTask(task &t);
void expansion_dfs(string sort);
void remove_unnecessary_predicates();
set<string> compute_constants_in_domain();
void add_to_method_as_last(method &m, plan_step ps);

void flatten_tasks(bool compileConditionalEffects,
                   bool linearConditionalEffectExpansion,
                   bool encodeDisjunctivePreconditionsInMethods);

void parsed_method_to_data_structures(
    bool compileConditionalEffects, bool linearConditionalEffectExpansion,
    bool encodeDisjunctivePreconditionsInMethods);

#endif
