#ifndef __TIMELINES

#define __TIMELINES

#include <plog/Log.h>
#include <set>
#include <string>

#include "domain.hpp"
#include "graph.hpp"
#include "search.hpp"
#include "stn.hpp"

using namespace std;

extern STN stn;

class Token
{
  private:
    set<arg_and_type> knowns;
    set<arg_and_type> arguments;
    set<literal> precondition_literals, effect_literals;

  protected:
    bool external;
    string start_t, end_t;
    shared_ptr<Node> start, end;
    string name, resource, request_id;

  public:
    Token()
      : external(false)
      , start_t("")
      , end_t("")
      , start(nullptr)
      , end(nullptr)
      , name("")
      , resource("")
      , request_id("")
    {}

    void set_external();
    string get_end() const;
    shared_ptr<Node> get_end_timepoint() const;
    void set_end_timepoint(shared_ptr<Node> end);
    string get_name() const;
    string get_start() const;
    shared_ptr<Node> get_start_timepoint() const;
    void set_start_timepoint(shared_ptr<Node> start);
    string to_string() const;
    bool is_external() const;
    string get_resource() const;
    void set_name(string _name);
    string get_request_id() const;
    void add_effects(literal eff);
    set<literal> get_effects() const;
    void set_resource(string _resource);
    void add_arguments(arg_and_type arg);
    set<arg_and_type> get_knowns() const;
    void add_preconditions(literal prec);
    void add_effects(vector<literal> eff);
    set<literal> get_preconditions() const;
    set<arg_and_type> get_arguments() const;
    void set_request_id(string _request_id);
    void add_knowns(arg_and_type assignment);
    void change_known(arg_and_type new_known);
    bool create_timepoints(double duration = 0);
    void add_preconditions(vector<literal> prec);
    void add_arguments(vector<arg_and_type> args);
    void add_knowns(vector<arg_and_type> assignments);
    bool create_timepoints(STN* stn,
                           double duration = 0,
                           stack<STN_operation>* search_operation_history = nullptr);
};

bool
operator<(const Token& lhs, const Token& rhs);
bool
operator==(const Token& lhs, const Token& rhs);
bool
operator!=(const Token& lhs, const Token& rhs);

class Auxiliary_Token : public Token
{
  public:
    Auxiliary_Token()
    {
        name = "";
        end_t = "";
        start_t = "";
        resource = "";
        request_id = "";
        external = false;
    }

    Auxiliary_Token(string name)
    {
        end_t = "";
        start_t = "";
        resource = "";
        request_id = "";
        external = false;
        this->set_name(name);
    }

    string to_string() const;
};

class Timeline
{
  protected:
    string id;
    string resource;
    vector<Token> tokens;

  public:
    Timeline()
      : id("")
      , resource("")
      , tokens()
    {}

    string get_id();
    string get_resource();
    void add_token(Token t);
    void set_id(string _id);
    string to_string() const;
    vector<Token> get_tokens();
    void del_token(int token_loc);
    void set_resource(string _resource);
    void del_tokens(pair<int, int> range);
    void insert_token(Token t, Token prev, Token next);
};

class Plan
{
  protected:
    vector<Timeline> timelines;

  public:
    map<string, int> num_tasks_robot;

    Plan()
      : timelines()
    {}

    string to_string() const;
    void add_timeline(Timeline t);
    vector<Timeline> get_timelines();
    Timeline* get_timelines(string id);
    int get_num_tasks_robot(string robot);
    void update_num_tasks_robot(string robot);
    Timeline* get_timelines(string id, string resource);
};

struct task_vertex
{
    Token tk;
    string id;
    shared_ptr<task_vertex> parent;

  public:
    task_vertex()
      : tk()
      , id("")
      , parent(nullptr)
    {}

    task_vertex(Token _tk, string _id, shared_ptr<task_vertex> _parent)
    {
        tk = _tk;
        id = _id;
        parent = _parent;
    }

    string to_string() const;
};

typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, task_vertex, edge>
  task_adj_list;
typedef boost::graph_traits<task_adj_list>::edge_iterator task_edge_it;
typedef boost::graph_traits<task_adj_list>::vertex_iterator task_vertex_it;
typedef boost::graph_traits<task_adj_list>::vertex_descriptor task_vertex_t;
typedef boost::graph_traits<task_adj_list>::out_edge_iterator task_out_edge_it;

struct task_network
{
    string id;
    task_adj_list adj_list;

    task_network() { id = ""; }

    string to_string() const;
};

const double zero = 0.0;
const string cz_constraint = "_cz_";
const string meets_constraint = "_meets_";
const string before_constraint = "_before_";
const string duration_constraint = "_duration_";
const string dummy_known_name = "__dummy_known";
const string contains_constraint = "_contains_";
const string sequencing_constraint = "_sequencing_";
const string dependent_meets_constraint = "_dep_meets_";

Plan
instantiate_plan();
task_vertex_t
get_vertex(string id, task_network G);
Timeline
instantiate_timeline(string id, string resource);
void
add_sync_constraints(method m, vector<Token> tokens);
void
add_contains_constraints(method m, Token parent, vector<Token> children);
void
add_literals(task_vertex_t node_t, shared_ptr<task_vertex> parent, task_network* tn);
Token
instantiate_token(string name, string resource, string request_id = "", double duration = 0);

Token
instantiate_token(string name,
                  string resource,
                  STN* stn,
                  string request_id = "",
                  double duration = 0,
                  stack<STN_operation>* search_history_operation = nullptr);

task_network
assign_open_variables(map<string, arg_and_type> var_assign_map, task_network r_tree);

bool
check_compatibility_of_argument(string arg, vector<arg_and_type> vars, set<arg_and_type> args);

bool
check_compatibility_of_literals(literal l, vector<arg_and_type> vars, set<arg_and_type> args);

vector<Token>
expand(search_vertex t,
       shared_ptr<task_vertex> parent,
       task_network* graph,
       set<search_vertex> vert,
       request r);

pair<task_network, vector<arg_and_type>>
instantiate_task_network(search_vertex* root, set<search_vertex> vert, request r);

#endif
