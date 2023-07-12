#include "search.hpp"
#include <numeric>

string
search_vertex::to_string() const
{
    string result = "Search Vertex: ";
    result += "\n\t Id = " + id + "\n";

    result += "\t Leaf = ";
    if (leaf)
        result += "true\n";
    else
        result += "false\n";

    result += "\t Or Node = ";
    if (or_node)
        result += "true\n";
    else
        result += "false\n";

    result += "\t Task Name = " + task_name + "\n";
    result += "\t Method Name = " + method_name + "\n";

    result += "\t Parent Id = " + parent->id + "\n";

    return result;
}

bool
operator<(const search_vertex& left, const search_vertex& right)
{
    return (left.id < right.id);
}

string
search_tree::to_string() const
{
    string result = "Search Tree: ";
    result += "\n\t Id = " + id + "\n";

    result += "\t Adjacency List: \n";
    search_edge_it it = search_edge_it(), end = search_edge_it();
    for (tie(it, end) = edges(adj_list); it != end; it++)
        result += "\t\t " + adj_list[source(*it, adj_list)].id + " -> " +
                  adj_list[target(*it, adj_list)].id + "\n";

    return result;
}

// NOTE: Cannot do get_vertex for "or" node!
search_vertex_t
get_vertex(string id, search_tree G)
{
    search_vertex_it v = search_vertex_it(), vend = search_vertex_it();
    for (boost::tie(v, vend) = boost::vertices(G.adj_list); v != vend; v++)
        if (G.adj_list[*v].id == id)
            return *v;

    return search_vertex_t();
}

void
expand(task t, std::shared_ptr<search_vertex> parent, search_tree* tree)
{
    bool primitive = false;
    vector<method> alternatives = vector<method>();
    for (method m : methods)
        if (m.at == t.name)
            alternatives.push_back(m);
    for (task pt : primitive_tasks)
        if (pt.name == t.name) {
            primitive = true;
            break;
        }

    std::shared_ptr<search_vertex> node = nullptr;
    if (alternatives.size() > 1 && !primitive) {
        node = std::make_shared<search_vertex>(search_vertex("or", false, true, "", "", parent));
        search_vertex_t node_t = boost::add_vertex(*node, tree->adj_list);

        if (parent != nullptr) {
            edge e(parent->id, node->id);
            search_vertex_t parent_desc = get_vertex(parent->id, *tree);
            boost::add_edge(parent_desc, node_t, e, tree->adj_list);
        }

        for (method a : alternatives) {
            std::shared_ptr<search_vertex> alt_node = std::make_shared<search_vertex>(
              search_vertex(a.at + "-" + a.name, false, false, a.at, a.name, node));
            search_vertex_t alt_node_t = boost::add_vertex(*alt_node, tree->adj_list);

            edge e("or", alt_node->id);
            boost::add_edge(node_t, alt_node_t, e, tree->adj_list);

            for (plan_step ps : a.ps)
                if (ps.task.substr(0, method_precondition_action_name.length()) !=
                    method_precondition_action_name)
                    expand(task_name_map[ps.task], alt_node, tree);
        }

    } else if (!primitive) {
        method m = alternatives[0];
        node = std::make_shared<search_vertex>(
          search_vertex(m.at + "-" + m.name, false, false, m.at, m.name, parent));
        search_vertex_t node_t = boost::add_vertex(*node, tree->adj_list);

        if (parent != nullptr) {
            edge e(parent->id, node->id);
            search_vertex_t parent_desc = get_vertex(parent->id, *tree);
            boost::add_edge(parent_desc, node_t, e, tree->adj_list);
        }

        for (plan_step ps : m.ps)
            if (ps.task.substr(0, method_precondition_action_name.length()) !=
                method_precondition_action_name)
                expand(task_name_map[ps.task], node, tree);

    } else {
        node = std::make_shared<search_vertex>(
          search_vertex(t.name + "-" + parent->id, true, false, "", "", parent));
        search_vertex_t node_t = boost::add_vertex(*node, tree->adj_list);

        if (parent != nullptr) {
            edge e(parent->id, node->id);
            search_vertex_t parent_desc = get_vertex(parent->id, *tree);
            boost::add_edge(parent_desc, node_t, e, tree->adj_list);
        } else
            node = nullptr;
    }
}

search_tree
create_search_tree(request r)
{
    search_tree tree = search_tree();
    tree.id = r.id;

    std::shared_ptr<search_vertex> root = nullptr;
    int alternatives = 0;
    for (method m : methods)
        if (m.at == r.demand.name)
            alternatives++;

    if (alternatives > 1) {
        root = std::make_shared<search_vertex>(search_vertex("or", false, true, "", "", nullptr));
        boost::add_vertex(*root, tree.adj_list);
        expand(r.demand, root, &tree);

    } else
        expand(r.demand, nullptr, &tree);

    return tree;
}

vector<vector<string>>
cartesian(vector<vector<string>>& candidates)
{
    vector<vector<string>> cart = vector<vector<string>>();

    auto product = [](long long a, vector<string>& b) { return a * b.size(); };

    const long long N = accumulate(candidates.begin(), candidates.end(), 1LL, product);

    vector<string> inter(candidates.size());
    for (long long n = 0; n < N; ++n) {
        lldiv_t q{ n, 0 };
        for (long long i = candidates.size() - 1; i >= 0; i--) {
            q = div(q.quot, candidates[i].size());
            inter[i] = candidates[i][q.rem];
        }

        string c = boost::algorithm::join(inter, ",");
        vector<string> cand = vector<string>();
        cand.push_back(c);
        cart.push_back(cand);
    }

    return cart;
}

vector<vector<string>>
get_leafs(search_vertex node, search_tree tree)
{
    vector<vector<string>> leafs = vector<vector<string>>();
    if (node.leaf) {
        vector<string> l(1, node.id);
        leafs.push_back(l);
        return leafs;

    } else if (node.or_node) {
        vector<string> pot = vector<string>();
        search_vertex_t node_t = get_vertex(node.id, tree);
        search_out_edge_it ei = search_out_edge_it(), ei_end = search_out_edge_it();
        for (boost::tie(ei, ei_end) = boost::out_edges(node_t, tree.adj_list); ei != ei_end; ei++) {
            search_vertex_t target = boost::target(*ei, tree.adj_list);
            search_vertex t = tree.adj_list[target];
            vector<vector<string>> op = get_leafs(t, tree);
            for (vector<string> cand : op)
                for (string s : cand)
                    pot.push_back(s);
        }

        leafs.push_back(pot);

    } else {
        vector<vector<string>> collection = vector<vector<string>>();
        search_vertex_t node_t = get_vertex(node.id, tree);
        search_out_edge_it ei = search_out_edge_it(), ei_end = search_out_edge_it();
        for (boost::tie(ei, ei_end) = boost::out_edges(node_t, tree.adj_list); ei != ei_end; ei++) {
            search_vertex_t target = boost::target(*ei, tree.adj_list);
            search_vertex t = tree.adj_list[target];
            vector<vector<string>> op = get_leafs(t, tree);
            for (vector<string> cand : op)
                collection.push_back(cand);
        }

        leafs = cartesian(collection);
    }

    return leafs;
}

set<search_vertex>
get_candidate(vector<string> leafs, search_tree tree)
{
    set<search_vertex> candidate = set<search_vertex>();

    for (string l : leafs) {
        vector<string> children = vector<string>();
        boost::algorithm::split(children, l, boost::is_any_of(","));

        for (string c : children) {
            search_vertex_t t = get_vertex(c, tree);
            search_vertex task = tree.adj_list[t];
            candidate.insert(task);

            std::shared_ptr<search_vertex> par = task.parent;
            while (par != nullptr) {
                if (!par->or_node) {
                    search_vertex_t p = get_vertex(par->id, tree);
                    candidate.insert(tree.adj_list[p]);
                }

                par = par->parent;
            }
        }
    }

    return candidate;
}
