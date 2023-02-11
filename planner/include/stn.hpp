#ifndef __STN

#define __STN

#include <set>
#include <map>
#include <tuple>
#include <deque>
#include <vector>
#include <limits>
#include <memory>
#include <fstream>
#include <iostream>

using namespace std;

typedef tuple<double, double> stn_bounds;
typedef tuple<string, string, double, double> constraint;

const double inf = numeric_limits<double>::infinity();

struct SP_Data {
    int pred;
    bool prop;
    double dist;

    SP_Data() {
        pred = 0;
        dist = 0.0;
        prop = false;
    }
};

struct Edge {
    int id;
    shared_ptr<Edge> next;
    string c_id;
    double weight;

    Edge() {
        id = 0;
        c_id = "";
        weight = 0.0;
        next = nullptr;
    }
};

struct Node {
    int id;
    bool in_queue;
    string timepoint_id;

    shared_ptr<SP_Data> lb_data;
    shared_ptr<SP_Data> ub_data;

    shared_ptr<Edge> edges_in;
    shared_ptr<Edge> edges_out;

    Node() {
        id = 0;
        in_queue = false;
        timepoint_id = "";

        lb_data = nullptr;
        ub_data = nullptr;

        edges_in = nullptr;
        edges_out = nullptr;
    }
};

class STN {
   private:
    int n;
    string id;
    vector<shared_ptr<Node>> graph;
    map<string, int> timepoints;
    map<string, constraint> constraints;

    void print_queue();
    int get_tp_id(string x);
    void insert_new_node(int i, string s);
    void del_edge(int x, int y, string s);
    bool propagation(int x, int y, deque<int> *q);
    deque<int> invalidate_nodes(int x_id, int y_id);
    shared_ptr<Edge> del_all_edges(shared_ptr<Edge> e, int y, bool del_name);
    void add_edge(int x, int y, double weight, string s);
    void invalidate_ub(set<int> &invalidated_nodes, int x);
    void invalidate_lb(set<int> &invalidated_nodes, int x);
    void rollback_updates(deque<tuple<int, bool, int, double>>);

   public:
    STN() {
        n = 0;
        id = "";
        timepoints = map<string, int>();
        graph = vector<shared_ptr<Node>>();
        constraints = map<string, constraint>();
    }

    ~STN() { destroy(); }

    void init();
    void destroy();
    void cleanup();
    int num_points();
    void print_graph();
    void print_sp_tree();
    void add_timepoint(string x);
    void del_timepoint(string x);
    bool del_constraint(string s);
    void gen_dot_file(string file_name);
    constraint get_constraint(string s);
    map<string, constraint> get_constraints();
    bool add_constraint(string s, constraint c);
    bool del_all_constraints(string x, string y);
    bool modify_constraint(string s, constraint c);
    tuple<double, double> get_feasible_values(string s);
};

#endif
