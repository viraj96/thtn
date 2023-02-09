#ifndef __SEARCH

#define __SEARCH

#include <stdlib.h>

#include <set>
#include <string>
#include <numeric>
#include <boost/algorithm/string.hpp>

#include "graph.hpp"
#include "domain.hpp"
#include "parsetree.hpp"

struct search_vertex {
    string id;
    bool leaf;
    bool or_node;
    string task_name;
    string method_name;
    shared_ptr<search_vertex> parent;

    search_vertex() {
        id = "";
        leaf = false;
        task_name = "";
        or_node = false;
        method_name = "";
        parent = nullptr;
    }

    search_vertex(string _id, bool _leaf, bool _or_node, string _task_name,
                  string _method_name, shared_ptr<search_vertex> _parent) {
        id = _id;
        leaf = _leaf;
        or_node = _or_node;
        task_name = _task_name;
        method_name = _method_name;
        parent = _parent;
    }

    string to_string() const;
};

bool operator<(const search_vertex &left, const search_vertex &right);

typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS,
                              search_vertex, edge>
    search_adj_list;
typedef boost::graph_traits<search_adj_list>::edge_iterator search_edge_it;
typedef boost::graph_traits<search_adj_list>::vertex_iterator search_vertex_it;
typedef boost::graph_traits<search_adj_list>::vertex_descriptor search_vertex_t;
typedef boost::graph_traits<search_adj_list>::out_edge_iterator
    search_out_edge_it;

struct search_tree {
    string id;
    search_adj_list adj_list;

    search_tree() { id = ""; }

    string to_string() const;
};

search_tree create_search_tree(request r);
search_vertex_t get_vertex(string id, search_tree tree);
void expand(task t, shared_ptr<search_vertex> parent, search_tree *tree);
vector<vector<string> > cartesian(vector<vector<string> > &candidates);
set<search_vertex> get_candidate(vector<string> leafs, search_tree tree);
vector<vector<string> > get_leafs(search_vertex node, search_tree tree);

#endif
