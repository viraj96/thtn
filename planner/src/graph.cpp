#include "graph.hpp"

string
coordinate::to_string() const
{
    string result = "Coordinate: ";
    result += "\n\t X = " + std::to_string(x) + "\n";
    result += "\t Y = " + std::to_string(y) + "\n";
    result += "\t Z = " + std::to_string(z) + "\n";

    return result;
}

string
vertex::to_string() const
{
    string result = "Vertex: ";
    result += "\n\t Id = " + id + "\n";

    return result;
}

string
edge::to_string() const
{
    string result = "Edge: ";
    result += "\n\t Id 1 = " + id1 + "\n";
    result += "\n\t Id 2 = " + id2 + "\n";

    return result;
}

string
graph::to_string() const
{
    string result = "Graph: ";
    result += "\n\t Id = " + id + "\n";

    edge_it it, end;
    result += "\t Adjacency List = \n";
    for (boost::tie(it, end) = edges(adj_list); it != end; ++it)
        result += "\t\t " + adj_list[source(*it, adj_list)].id + " -> " +
                  adj_list[target(*it, adj_list)].id + "\n";

    return result;
}

vertex_t
get_vertex(string id, graph G)
{
    vertex_it v = vertex_it(), vend = vertex_it();
    for (boost::tie(v, vend) = boost::vertices(G.adj_list); v != vend; v++)
        if (G.adj_list[*v].id == id)
            return *v;

    return vertex_t();
}

graph
construct_network(string vertices, string edges)
{
    graph G = graph();

    // Adding vertices
    size_t pos = 0;
    while ((pos = vertices.find(" ")) != string::npos) {
        vertex v = vertex();
        v.id = vertices.substr(0, pos);
        boost::add_vertex(v, G.adj_list);
        vertices.erase(0, pos + 1);
    }

    vertex v = vertex();
    v.id = vertices;
    boost::add_vertex(v, G.adj_list);

    // Adding edges
    pos = 0;
    while ((pos = edges.find(")")) != string::npos) {
        string candidate = "";
        candidate = edges.substr(0, pos);

        unsigned start = candidate.find("(");
        unsigned end = candidate.find(")");
        candidate = candidate.substr(start + 1, end - start - 1);

        edge e = edge();
        vertex_t v1 = vertex_t(), v2 = vertex_t();
        size_t local_pos = 0;
        while ((local_pos = candidate.find(" ")) != string::npos) {
            e.id1 = candidate.substr(0, local_pos);
            v1 = get_vertex(e.id1, G);
            candidate.erase(0, local_pos + 1);
        }
        e.id2 = candidate;
        v2 = get_vertex(e.id2, G);

        boost::add_edge(v1, v2, e, G.adj_list);
        edges.erase(0, pos + 1);
    }

    return G;
}

void
find_all_paths_helper(vertex_t source,
                      vertex_t sink,
                      vector<vertex_t> path,
                      vector<vector<vertex_t>>& paths)
{
    path.push_back(source);

    if (source == sink)
        paths.push_back(path);
    else {
        for (edge_t out :
             boost::make_iterator_range(boost::out_edges(source, rail_network.adj_list))) {
            vertex_t v = target(out, rail_network.adj_list);
            if (find(path.begin(), path.end(), v) == path.end())
                find_all_paths_helper(v, sink, path, paths);
        }
    }

    path.pop_back();
}

vector<vector<vertex_t>>
find_all_paths(vertex_t source, vertex_t sink)
{
    vector<vertex_t> path = vector<vertex_t>();
    vector<vector<vertex_t>> paths = vector<vector<vertex_t>>();

    find_all_paths_helper(source, sink, path, paths);

    return paths;
}
