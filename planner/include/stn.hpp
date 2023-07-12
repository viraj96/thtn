#ifndef __STN

#define __STN

#include <deque>
#include <limits>
#include <memory>
#include <set>
#include <stack>
#include <tuple>
#include <vector>
#include <unordered_map>

using namespace std;

typedef tuple<double, double> stn_bounds;
typedef tuple<string, string, double, double> constraint;

const double inf = numeric_limits<double>::infinity();

enum STN_operation_type
{
    ADD_TIMEPOINT,
    DEL_TIMEPOINT,
    ADD_CONSTRAINT,
    DEL_CONSTRAINT
};

enum STN_constraint_type
{
    CZ,
    MEETS,
    BEFORE,
    SEQUENCE,
    CONTAINS,
    DURATION,
    DEPENDENT_MEETS,
    TIMEPOINT // When we are creating or deleting a timepoint then we do not have any constraint
              // type
};

struct SP_Data
{
    int pred;
    bool prop;
    double dist;

    SP_Data()
    {
        pred = 0;
        dist = 0.0;
        prop = false;
    }
};

struct Edge
{
    int id;
    shared_ptr<Edge> next;
    string c_id;
    double weight;

    Edge()
    {
        id = 0;
        c_id = "";
        weight = 0.0;
        next = nullptr;
    }
};

struct Node
{
    int id;
    bool in_queue;
    string timepoint_id;

    shared_ptr<SP_Data> lb_data;
    shared_ptr<SP_Data> ub_data;

    shared_ptr<Edge> edges_in;
    shared_ptr<Edge> edges_out;

    Node()
    {
        id = 0;
        in_queue = false;
        timepoint_id = "";

        lb_data = nullptr;
        ub_data = nullptr;

        edges_in = nullptr;
        edges_out = nullptr;
    }
};

struct STN_operation
{
    string tp1, tp2;
    STN_operation_type op_type;
    STN_constraint_type c_type;
    pair<double, double> bounds;
    shared_ptr<Node> tp1_node, tp2_node;

    STN_operation()
    {
        tp1 = string();
        tp2 = string();
        op_type = STN_operation_type::ADD_TIMEPOINT;
        c_type = STN_constraint_type::TIMEPOINT;
        bounds = make_pair(0, 0);
        tp1_node = nullptr;
        tp2_node = nullptr;
    }

    STN_operation(string _tp1,
                  string _tp2,
                  shared_ptr<Node> _tp1_node,
                  shared_ptr<Node> _tp2_node,
                  STN_operation_type _op_type,
                  STN_constraint_type _c_type,
                  pair<double, double> _bounds)
    {
        tp1 = _tp1;
        tp2 = _tp2;
        tp1_node = _tp1_node;
        tp2_node = _tp2_node;
        op_type = _op_type;
        c_type = _c_type;
        bounds = _bounds;
    }
};

class STN
{
  private:
    int n;
    string id;
    shared_ptr<Node> cz;
    vector<shared_ptr<Node>> graph;
    unordered_map<string, int> timepoints;
    unordered_map<string, constraint> constraints;

    void print_queue();
    int get_tp_id(string x);
    shared_ptr<Node> insert_new_node(int i, string s);
    void del_edge(int x, int y, string s);
    bool propagation(int x, int y, deque<int>* q);
    deque<int> invalidate_nodes(int x_id, int y_id);
    shared_ptr<Edge> del_all_edges(shared_ptr<Edge> e,
                                   int y,
                                   bool del_name,
                                   stack<constraint>* search_history = nullptr);
    void add_edge(int x, int y, double weight, string s);
    void invalidate_ub(set<int>& invalidated_nodes, int x);
    void invalidate_lb(set<int>& invalidated_nodes, int x);
    void rollback_updates(deque<tuple<int, bool, int, double>>);

  public:
    STN()
    {
        n = 0;
        id = "";
        cz = nullptr;
        timepoints = unordered_map<string, int>();
        graph = vector<shared_ptr<Node>>();
        constraints = unordered_map<string, constraint>();
    }

    ~STN() { destroy(); }

    void init();
    void destroy();
    void cleanup();
    int num_points();
    void print_graph();
    void print_sp_tree();
    shared_ptr<Node> get_cz();
    shared_ptr<Node> add_timepoint(string x);
    void gen_dot_file(string file_name);
    constraint get_constraint(string s);
    unordered_map<string, constraint> get_constraints();
    bool del_all_constraints(string x, string y);
    bool modify_constraint(string s, constraint c);
    tuple<double, double> get_feasible_values(string s);
    tuple<double, double> get_feasible_values(shared_ptr<Node> x);
    void del_timepoint(string x, stack<constraint>* search_history = nullptr);
    void del_timepoint(shared_ptr<Node> x_node);
    bool del_constraint(string s, stack<constraint>* search_history = nullptr);
    bool del_constraint(shared_ptr<Node> u, shared_ptr<Node> v, string s);
    bool add_constraint(string s, constraint c, stack<constraint>* search_history = nullptr);
    bool add_constraint(shared_ptr<Node> u,
                        shared_ptr<Node> v,
                        string s,
                        pair<double, double> bounds);
};

#endif
