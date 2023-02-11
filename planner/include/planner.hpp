#ifndef __PLANNER

#define __PLANNER

#ifdef USERDEBUG
#define DEBUG(x) std::cout << x
#else
#define DEBUG(x)
#endif

#include <queue>
#include <variant>
#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>

#include "cwa.hpp"
#include "graph.hpp"
#include "domain.hpp"
#include "parsetree.hpp"
#include "timelines.hpp"

using namespace std;

struct slot {
    Token tk;
    Token prev;
    Token next;
    string tl_id;
    vector<string> dependent_ends;

    slot() {
        tl_id = "";
        tk = Token();
        prev = Token();
        next = Token();
        dependent_ends = vector<string>();
    }

    slot(Token *_tk, Token *_prev, Token *_next, string _tl_id)
        : tk(*_tk), prev(*_prev), next(*_next), tl_id(_tl_id) {
        dependent_ends = vector<string>();
    }

    string to_string() const;
};

struct primitive_solution {
    Token primitive_token;
    vector<slot> token_slots;

    primitive_solution() {
        primitive_token = Token();
        token_slots = vector<slot>();
    }

    primitive_solution(Token *_primitive_token, vector<slot> *_token_slots)
        : primitive_token(*_primitive_token), token_slots(*_token_slots) {}

    string to_string() const;
};

struct tasknetwork_solution {
    int plan_id;
    string request_id;
    double metric_value;
    vector<primitive_solution> solution;

    tasknetwork_solution() {
        plan_id = 0;
        request_id = "";
        metric_value = std::numeric_limits<double>::infinity();
        solution = vector<primitive_solution>();
    }

    tasknetwork_solution(int _plan_id, string _request_id, double _metric_value,
                         vector<primitive_solution> *_solution)
        : plan_id(_plan_id),
          request_id(_request_id),
          metric_value(_metric_value),
          solution(*_solution) {}

    string to_string() const;
};

bool operator==(const slot &lhs, const slot &rhs);
bool operator!=(const slot &lhs, const slot &rhs);

class CompareSolutions {
   public:
    bool operator()(tasknetwork_solution &a, tasknetwork_solution &b) {
        return a.metric_value > b.metric_value;
    }
};

typedef priority_queue<tasknetwork_solution, vector<tasknetwork_solution>,
                       CompareSolutions>
    pq;

extern world_state initial_state;

bool is_resource(string type);
void commit_slots(Plan *p, pq *solution);
arg_and_type get_arg_and_attr(string arg);
string evaluate(string arg, set<arg_and_type> knowns);
bool check_temporal_bounds(slot *to_explore, STN *stn);
void update_object_state(object_state *obj, Token *prev);
bool add_meets_constraint(Token *first, Token *second, STN *stn);
Token gen_token(arg_and_type argument, Token *causal_token, STN *stn);

pair<bool, Plan> patch_plan(Plan p, Token *failing_tk,
                            vector<ground_literal> *init);
set<string> get_literal_args_and_types(literal *lit,
                                       set<arg_and_type> *arguments);
variant<bool, pair<bool, Token>> plan_validator(Plan *p,
                                                vector<ground_literal> *init);
pair<bool, vector<slot>> schedule_token(Token *tk, vector<slot> *explored,
                                        Plan *p, STN *stn);

bool add_contains_constraint(Token *first, Token *second, STN *stn,
                             bool submit = false);

pair<bool, task> find_satisfying_task(literal *precondition,
                                      set<string> *prec_arg_types);

set<string> get_precondition_args_and_types(literal precondition,
                                            set<arg_and_type> arguments);

map<string, arg_and_type> find_assignments(string cart_prod,
                                           vector<arg_and_type> open_vars);

bool check_functional_predicate(literal *precondition, set<arg_and_type> knowns,
                                world_state current_state);

bool check_equal_predicate(literal *precondition, set<arg_and_type> knowns,
                           world_state current_state);

arg_and_type evaluate(arg_and_type argument, set<arg_and_type> knowns,
                      world_state *current_state);

pq find_feasible_slots(task_network tree, Plan p, int attempts = 1,
                       string metric = "makespan");

tuple<bool, bool, bool> del_and_add_sequencing_constraint(Token *prev,
                                                          Token *curr,
                                                          Token *next, STN *stn,
                                                          bool submit = false);

bool check_init(literal *precondition, set<arg_and_type> knowns,
                world_state current_state, ground_literal *lit);

bool schedule_leafs(vector<task_vertex> leafs,
                    vector<vector<slot>> *tasknetwork_slots, Plan p,
                    double *makespan, string metric = "makespan");

void initialize_token_state(Token *tk, Plan *p, vector<Timeline *> *robots, 
        world_state *current_state, vector<arg_and_type> *other_resources,
        vector<arg_and_type> *affecting_resources);

bool check_precondition(literal *precondition, set<arg_and_type> knowns,
                        pair<string, var_declaration> world_state,
                        vector<ground_literal> *init);

pair<set<arg_and_type>, set<arg_and_type>> get_satisfying_knowns_and_args(
    task *satisfying_task, set<arg_and_type> *arguments,
    set<arg_and_type> *knowns, world_state *current_state,
    set<string> *prec_arg_types, bool functional = false);

pair<bool, vector<slot>> satisfy_precondition(literal *precondition,
                                              Token *failing_tk, Token *prev,
                                              world_state *current_state,
                                              Plan *p, STN *stn,
                                              vector<slot> *explored);

set<arg_and_type> align_args_and_knowns(task satisfying_task, Token prev,
                                        set<arg_and_type> satisfying_arguments,
                                        set<arg_and_type> satisfying_knowns,
                                        world_state current_state,
                                        vector<ground_literal> init);

// Functional Predicates
bool clear(vector<string> arguments, world_state *current_state);

// Functional Methods
vector<pair<task, var_declaration>> clear_and_move(vector<string> arguments,
                                                   world_state *current_state);
vector<pair<task, var_declaration>> reachable_location(
    vector<string> arguments, world_state *current_state);

#endif
