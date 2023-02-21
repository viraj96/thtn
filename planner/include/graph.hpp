#ifndef __GRAPH

#define __GRAPH

#include <plog/Log.h>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/adjacency_list.hpp>

using namespace std;

struct coordinate {
    double x, y, z;

    coordinate() {
        x = 0;
        y = 0;
        z = 0;
    }

    string to_string() const;
};

struct vertex {
    string id;

    vertex() { id = ""; }

    string to_string() const;
};

struct edge {
    string id1;
    string id2;

    edge() {
        id1 = "";
        id2 = "";
    }

    edge(string source, string target) {
        id1 = source;
        id2 = target;
    }

    string to_string() const;
};

typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS,
                              vertex, edge>
    adjacency_list;
typedef boost::graph_traits<adjacency_list>::edge_iterator edge_it;
typedef boost::graph_traits<adjacency_list>::edge_descriptor edge_t;
typedef boost::graph_traits<adjacency_list>::vertex_iterator vertex_it;
typedef boost::graph_traits<adjacency_list>::vertex_descriptor vertex_t;

struct graph {
    string id;
    adjacency_list adj_list;

    graph() { id = ""; }

    string to_string() const;
};

extern graph rail_network;

// Building the graph
vertex_t get_vertex(string id, graph G);
graph construct_network(string vertices, string edges);
vector<vector<vertex_t> > find_all_paths(vertex_t source, vertex_t sink);

void find_all_paths_helper(vertex_t source, vertex_t sink,
                           vector<vertex_t> path,
                           vector<vector<vertex_t> > &paths);

#endif
