#include "stn.hpp"

bool verbose_1 = false; // Public Functions (high level checks)
bool verbose_2 = false; // Private Functions (high level checks)
bool verbose_3 = false; // Low level checks

int
STN::num_points()
{
    return n;
}

map<string, constraint>
STN::get_constraints()
{
    return constraints;
}

void
STN::print_queue()
{
    cout << "Printing queue: \n";
    for (int i = 0; i < n; i++)
        cout << "\t" << graph[i]->id << ": " << graph[i]->in_queue;
    cout << "\n";
}

void
STN::print_graph()
{
    string tp_id = "";
    shared_ptr<Node> x = nullptr;
    shared_ptr<Node> y = nullptr;
    shared_ptr<Edge> e = nullptr;

    for (int i = 0; i < (int)graph.size(); i++) {
        x = graph[i];
        if (x == nullptr) {
            if (verbose_2 && verbose_3)
                cout << "Node " << i << " is null\n";
            continue;
        }

        cout << "Timepoint: " << x->timepoint_id << "\n";

        e = x->edges_out;
        cout << "\t Edges Out:\n";
        while (e != nullptr) {
            y = graph[e->id];
            if (y == nullptr)
                cout << "Edge to null node, e->id = " << e->id << endl;
            else {
                cout << "\t\t" << e->c_id << ": (" << x->timepoint_id << ", " << y->timepoint_id
                     << ") = " << e->weight << "\n";
            }
            e = e->next;
        }
        cout << "\n";
    }
}

void
STN::gen_dot_file(string file_name)
{
    ofstream fout(file_name);
    fout << "graph plan {\n";
    for (int i = 0; i < (int)graph.size(); i++) {
        shared_ptr<Node> x = graph[i];
        if (x != nullptr) {
            fout << to_string(x->id) << "[label=";
            fout << "\"" << x->timepoint_id << "\""
                 << ",style=filled,fillcolor=black,fontcolor=white];\n";
        }
    }
    vector<tuple<string, string, string, string, string>> relations =
      vector<tuple<string, string, string, string, string>>();

    for (int i = 0; i < (int)graph.size(); i++) {
        shared_ptr<Node> x = graph[i];
        if (x == nullptr)
            continue;
        shared_ptr<Edge> e = x->edges_out;
        while (e != nullptr) {
            shared_ptr<Node> y = graph[e->id];
            string e_weight = to_string(e->weight);

            bool lower_bound = false, upper_bound = false;
            if (e_weight.at(0) == '-')
                lower_bound = true;
            else
                upper_bound = true;

            if (upper_bound && e_weight != "inf")
                e_weight = e_weight.substr(0, e_weight.find(".") + 3);
            else if (lower_bound && e_weight != "-inf")
                e_weight = e_weight.substr(0, e_weight.find(".") + 3);

            bool found = false;
            for (auto it = relations.begin(); it != relations.end(); it++)
                if (get<4>(*it) == e->c_id) {
                    if (lower_bound)
                        get<2>(*it) = e_weight.substr(1);
                    else
                        get<3>(*it) = e_weight;
                    found = true;
                    break;
                }

            if (!found) {
                tuple<string, string, string, string, string> rel =
                  tuple<string, string, string, string, string>();
                if (lower_bound)
                    rel = make_tuple(
                      to_string(x->id), to_string(y->id), e_weight.substr(1), "", e->c_id);
                else if (upper_bound)
                    rel = make_tuple(to_string(x->id), to_string(y->id), "", e_weight, e->c_id);
                relations.push_back(rel);
            }
            e = e->next;
        }
    }

    for (tuple<string, string, string, string, string> rel : relations)
        fout << "\"" << get<0>(rel) << "\""
             << "--"
             << "\"" << get<1>(rel) << "\""
             << "[label="
             << "\"[" << get<2>(rel) << ", " << get<3>(rel) << "]\""
             << ",penwidth=2,color=black];\n";
    fout << "}\n";
}

void
STN::print_sp_tree()
{
    shared_ptr<Node> x = nullptr;
    string id = "";
    cout << "------PRINTING SHORTEST PATHS------\n";

    for (size_t i = 0; i < (size_t)n; i++) {
        x = graph[i];
        if (x == nullptr) {
            cout << "Node " << i << " is null\n";
            continue;
        }
        cout << "Node [" << x->id << "]: " << x->timepoint_id << "\n";
        cout << "\t LB: (" << x->lb_data->pred << ", " << x->lb_data->dist << ")\n";
        cout << "\t UB: (" << x->ub_data->pred << ", " << x->ub_data->dist << ")\n";
    }
}

void
STN::add_edge(int x, int y, double weight, string s)
{
    if (verbose_2)
        cout << "\tAdding edge: " << s << "\n";
    shared_ptr<Node> u = graph[x];
    shared_ptr<Node> v = graph[y];

    shared_ptr<Edge> euv = make_shared<Edge>();
    euv->id = y;
    euv->c_id = s;
    euv->weight = weight;
    euv->next = nullptr;

    shared_ptr<Edge> evu = make_shared<Edge>();
    evu->id = x;
    evu->c_id = s;
    evu->weight = weight;
    evu->next = nullptr;

    if (u->edges_out == nullptr) {
        u->edges_out = euv;
    } else {
        euv->next = u->edges_out;
        u->edges_out = euv;
    }

    if (v->edges_in == nullptr) {
        v->edges_in = evu;
    } else {
        evu->next = v->edges_in;
        v->edges_in = evu;
    }
}

void
STN::del_edge(int x, int y, string s)
{
    if (verbose_2)
        cout << "Deleting edge: " << s << "\n";
    shared_ptr<Node> u = graph[x];
    shared_ptr<Node> v = graph[y];
    shared_ptr<Edge> e = nullptr, p = nullptr;

    p = nullptr;
    e = u->edges_out;
    while (e != nullptr) {
        if (e->c_id == s) {
            if (p != nullptr)
                p->next = e->next;
            else
                u->edges_out = e->next;
            e = nullptr;

        } else {
            p = e;
            e = e->next;
        }
    }

    p = nullptr;
    e = v->edges_in;
    while (e != nullptr) {
        if (e->c_id == s) {
            if (p != nullptr)
                p->next = e->next;
            else
                v->edges_in = e->next;
            e = nullptr;

        } else {
            p = e;
            e = e->next;
        }
    }
}

void
STN::rollback_updates(deque<tuple<int, bool, int, double>> q)
{
    if (verbose_2)
        cout << "\tRolling back updates\n";
    tuple<int, bool, int, double> update = tuple<int, bool, int, double>();
    shared_ptr<Node> x = nullptr;

    for (deque<tuple<int, bool, int, double>>::iterator i = q.begin(); i != q.end(); i++) {
        update = *i;
        x = graph[get<0>(update)];
        x->in_queue = false;
        x->ub_data->prop = false;
        x->lb_data->prop = false;

        if (get<1>(update)) {
            // UB was updated;
            if (verbose_2 && verbose_3)
                cout << "\t\tRolling back UB change for " << x->id << " " << x->timepoint_id
                     << "\n";

            x->ub_data->pred = get<2>(update);
            x->ub_data->dist = get<3>(update);

        } else {
            // LB was updated
            if (verbose_2 && verbose_3)
                cout << "\tRolling back LB change for " << x->id << " " << x->timepoint_id << "\n";

            x->lb_data->pred = get<2>(update);
            x->lb_data->dist = get<3>(update);
        }
    }
}

bool
STN::propagation(int x, int y, deque<int>* q)
{
    if (verbose_2)
        cout << "\tPropagation on " << x << ", " << y << "\n";

    shared_ptr<Edge> e = nullptr;
    shared_ptr<Node> u = nullptr, v = nullptr;
    bool ubp_1 = false, ubp_2 = false, lbp_1 = false, lbp_2 = false;
    deque<tuple<int, bool, int, double>> updates = deque<tuple<int, bool, int, double>>();

    // Execute propagation algorithm
    while (!q->empty()) {
        // Pop node from queue
        u = graph[q->front()];
        u->in_queue = false;
        q->pop_front();

        if (verbose_2 && verbose_3)
            cout << "\t\t Popped node " << u->timepoint_id << "\n";

        // Forwards propagation
        if (u->ub_data->prop) {
            e = u->edges_out;

            // Iterate through out edges of u
            while (e != nullptr) {
                v = graph[e->id];
                if (verbose_2 && verbose_3)
                    cout << "\t\tUB EDGE (" << u->timepoint_id << ", " << v->timepoint_id
                         << ") = " << e->weight << "\n";

                // Update UB distance of v
                if (u->ub_data->dist + e->weight < v->ub_data->dist) {
                    // Store v's original values in case we need to rollback
                    // changes
                    if (verbose_2 && verbose_3)
                        cout << "\t\tSaving ub data for " << v->timepoint_id << "\n";

                    if (verbose_2 && verbose_3)
                        cout << "\t\tOld: " << v->ub_data->dist
                             << "\tNew: " << u->ub_data->dist + e->weight << endl;
                    updates.push_front(make_tuple(v->id, true, v->ub_data->pred, v->ub_data->dist));

                    v->ub_data->dist = u->ub_data->dist + e->weight;

                    if (v->ub_data->dist + v->lb_data->dist < 0) {
                        // Lower bound value of v is > upper bound =>
                        // inconsistent!
                        if (verbose_2 && verbose_3)
                            cout << "\t\tFAIL\n";
                        rollback_updates(updates);
                        return false;

                    } else if ((u->id == x && v->id == y)) {
                        if (ubp_1) {
                            // New edge has been updated twice in shortest
                            // path tree => inconsistent!
                            if (verbose_2 && verbose_3)
                                cout << "\t\tFAIL UB updated twice for node " << v->id << "\n";
                            rollback_updates(updates);
                            return false;

                        } else {
                            ubp_1 = true;
                        }

                    } else if ((u->id == y && v->id == x)) {
                        if (ubp_2) {
                            // New edge has been updated twice in shortest
                            // path tree => inconsistent!
                            if (verbose_2 && verbose_3)
                                cout << "\t\tFAIL UB updated twice for node " << v->id << "\n";
                            rollback_updates(updates);
                            return false;

                        } else {
                            ubp_2 = true;
                        }
                    }

                    v->ub_data->pred = u->id;
                    v->ub_data->prop = true;
                    if (!(v->in_queue)) {
                        q->push_back(v->id);
                        v->in_queue = true;
                    }
                }

                e = e->next;
            }
        }

        // Backwards propagation
        if (u->lb_data->prop) {
            e = u->edges_in;

            // Iterate through in edges of u
            while (e != nullptr) {
                v = graph[e->id];
                if (verbose_2 && verbose_3)
                    cout << "\t\tLB EDGE (" << u->timepoint_id << ", " << v->timepoint_id
                         << ") = " << e->weight << "\n";

                // Update LB distance of v
                if (u->lb_data->dist + e->weight < v->lb_data->dist) {
                    // Save original LB data incase new constraint is
                    // inconsistent
                    if (verbose_2 && verbose_3)
                        cout << "\t\tSaving lb data for " << v->timepoint_id << "\n";
                    updates.push_front(
                      make_tuple(v->id, false, v->lb_data->pred, v->lb_data->dist));

                    v->lb_data->dist = u->lb_data->dist + e->weight;

                    if (v->ub_data->dist + v->lb_data->dist < 0) {
                        // Lower bound value of v is > upper bound =>
                        // inconsistent!
                        if (verbose_2 && verbose_3)
                            cout << "\t\tFAIL\n";
                        rollback_updates(updates);
                        return false;

                    } else if ((u->id == x && v->id == y)) {
                        if (lbp_1) {
                            // New edge has been updated twice in shortest
                            // path tree => inconsistent!
                            if (verbose_2 && verbose_3)
                                cout << "\t\tFAIL LB updated twice for node " << v->id << "\n";
                            rollback_updates(updates);
                            return false;

                        } else {
                            lbp_1 = true;
                        }

                    } else if ((u->id == y && v->id == x)) {
                        if (lbp_2) {
                            // New edge has been updated twice in shortest
                            // path tree => inconsistent!
                            if (verbose_2 && verbose_3)
                                cout << "\t\tFAIL LB updated twice for node " << v->id << "\n";
                            rollback_updates(updates);
                            return false;

                        } else {
                            lbp_2 = true;
                        }
                    }

                    v->lb_data->pred = u->id;
                    v->lb_data->prop = true;
                    if (!(v->in_queue)) {
                        q->push_back(v->id);
                        v->in_queue = true;
                    }
                }

                e = e->next;
            }
        }

        u->ub_data->prop = false;
        u->lb_data->prop = false;
    }

    return true;
}

void
STN::insert_new_node(int i, string s)
{
    if (verbose_2)
        cout << "\tInserting new node: " << s << endl;
    shared_ptr<Node> x = nullptr;
    shared_ptr<SP_Data> data = nullptr;

    x = nullptr;
    x = make_shared<Node>();
    x->timepoint_id = s;
    x->in_queue = false;
    x->id = i;
    x->edges_in = nullptr;
    x->edges_out = nullptr;

    data = nullptr;
    data = make_shared<SP_Data>();
    data->pred = -1;
    data->prop = false;
    if (x->timepoint_id.find("_head_") != string::npos ||
        x->timepoint_id.find("_tail_") != string::npos)
        data->dist = 0;
    else
        data->dist = inf;
    x->lb_data = data;

    data = nullptr;
    data = make_shared<SP_Data>();
    data->pred = -1;
    data->prop = false;
    data->dist = inf;
    x->ub_data = data;

    if ((int)graph.size() <= i)
        graph.push_back(x);
    else
        graph[i] = x;
}

int
STN::get_tp_id(string x)
{
    map<string, int>::iterator i = map<string, int>::iterator();

    i = timepoints.find(x);
    if (i == timepoints.end())
        return -1;
    else
        return i->second;
}

constraint
STN::get_constraint(string s)
{
    map<string, constraint>::iterator i = map<string, constraint>::iterator();
    i = constraints.find(s);
    if (i == constraints.end())
        return make_tuple("", "", 0.0, 0.0);
    else
        return i->second;
}

void
STN::init()
{
    if (verbose_1)
        cout << "Initializing STN\n";
    n = 1;
    id = "global_stn";
    graph = vector<shared_ptr<Node>>(n, nullptr);
    insert_new_node(0, "cz");
    graph[0]->lb_data->dist = 0;
    graph[0]->ub_data->dist = 0;
    timepoints = map<string, int>();
    timepoints["cz"] = 0;
    constraints = map<string, constraint>();
}

tuple<double, double>
STN::get_feasible_values(string s)
{
    if (verbose_1)
        cout << "Getting feasible values\n";
    map<string, int>::iterator i = timepoints.find(s);
    if (i == timepoints.end()) {
        if (verbose_1)
            cout << "No such timepoint";
        return make_tuple(0, inf);

    } else {
        shared_ptr<Node> x = graph[i->second];
        return make_tuple(x->lb_data->dist, x->ub_data->dist);
    }
}

void
STN::add_timepoint(string x)
{
    if (verbose_1)
        cout << "Adding timepoint: " << x << endl;
    map<string, int>::iterator i = map<string, int>::iterator();

    i = timepoints.find(x);
    if (i == timepoints.end()) {
        if (verbose_1)
            cout << "Added timepoint " << x << "\n";
        int idx = n;
        n++;

        insert_new_node(idx, x);
        timepoints[x] = idx;

    } else if (verbose_1)
        cout << "Timepoint " << x << " already in STN\n";
}

void
STN::cleanup()
{
    map<string, int> tps_copy = timepoints;
    for (pair<string, int> tps : tps_copy) {
        shared_ptr<Node> x = graph[tps.second];
        if (x != nullptr) {
            shared_ptr<Edge> out = x->edges_out;
            shared_ptr<Edge> in = x->edges_in;
            if (out == nullptr && in == nullptr) {
                del_timepoint(tps.first);
            }
        }
    }
}

void
STN::del_timepoint(string x)
{
    if (verbose_1)
        cout << "Deleting timepoint: " << x << endl;
    map<string, int>::iterator i = timepoints.find(x);

    if (i == timepoints.end()) {
        if (verbose_1)
            cout << "No such timepoint found\n";
        return;
    }

    shared_ptr<Node> x_node = graph[i->second], y_node = nullptr;
    shared_ptr<Edge> e = nullptr, p = nullptr;

    if (verbose_1 && verbose_3)
        cout << "\t\tInvalidating nodes dependent on Node[" << i->second << ": " << i->first
             << endl;

    set<int> invalidated_nodes = set<int>();
    deque<int> q = deque<int>();

    invalidate_ub(invalidated_nodes, i->second);
    invalidate_lb(invalidated_nodes, i->second);

    shared_ptr<Node> temp_1 = nullptr, temp_2 = nullptr;
    for (set<int>::iterator i = invalidated_nodes.begin(); i != invalidated_nodes.end(); i++) {
        temp_1 = graph[*i];
        e = temp_1->edges_in;
        while (e != nullptr) {
            temp_2 = graph[e->id];
            if (verbose_1 && verbose_3)
                cout << "\t\tV: " << temp_2->timepoint_id << "\n";
            if (!temp_2->in_queue && temp_2->id != x_node->id) {
                temp_2->in_queue = true;
                temp_2->ub_data->prop = true;
                temp_2->lb_data->prop = true;
                q.push_back(temp_2->id);
            }
            e = e->next;
        }
    }

    if (verbose_1 && verbose_3)
        cout << "\t\tDeleting all edges to/from Node[" << i->second << "]: " << i->first << endl;

    e = x_node->edges_out;
    p = nullptr;

    while (e != nullptr) {
        y_node = graph[e->id];
        y_node->edges_in = del_all_edges(y_node->edges_in, x_node->id, true);
        p = e->next;
        e = p;
    }

    e = x_node->edges_in;
    p = nullptr;

    while (e != nullptr) {
        y_node = graph[e->id];
        y_node->edges_out = del_all_edges(y_node->edges_out, x_node->id, true);
        p = e->next;
        e = p;
    }

    if (verbose_1 && verbose_3)
        cout << "\t\tDeleting node from memory\n";

    x_node->edges_out = nullptr;
    x_node->edges_in = nullptr;

    graph[i->second] = nullptr;
    timepoints.erase(i->first);

    propagation(-1, -1, &q);

    return;
}

bool
STN::add_constraint(string s, constraint c)
{
    if (verbose_1)
        cout << "Adding constraint " << s << endl;

    int x_id = 0, y_id = 0;
    string x = get<0>(c);
    string y = get<1>(c);
    double lb = get<2>(c);
    double ub = get<3>(c);
    deque<int> q = deque<int>();
    shared_ptr<Node> u = nullptr, v = nullptr;

    x_id = get_tp_id(x);

    if (x_id == -1) {
        if (verbose_1)
            cout << "Timepoint " << x << " not found, adding to graph\n";
        add_timepoint(x);
        x_id = get_tp_id(x);
    }

    y_id = get_tp_id(y);
    if (y_id == -1) {
        if (verbose_1)
            cout << "Timepoint " << y << " not found, adding to graph\n";
        add_timepoint(y);
        y_id = get_tp_id(y);
    }

    constraint temp = get_constraint(s);
    if (get<0>(temp) != "") {
        if (verbose_1)
            cout << "Constraint with that name already exists\n";
        return true;

    } else
        constraints[s] = c;

    u = graph[x_id];
    v = graph[y_id];

    add_edge(x_id, y_id, ub, s);
    add_edge(y_id, x_id, -lb, s);

    q.push_back(x_id);
    q.push_back(y_id);
    u->in_queue = true;
    v->in_queue = true;
    u->lb_data->prop = true;
    u->ub_data->prop = true;
    v->lb_data->prop = true;
    v->ub_data->prop = true;

    bool f = propagation(x_id, y_id, &q);

    if (!f) {
        del_edge(x_id, y_id, s);
        del_edge(y_id, x_id, s);
        constraints.erase(s);
    }

    return f;
}

bool
STN::modify_constraint(string s, constraint c)
{
    if (verbose_1)
        cout << "Modifying constraint\n";

    bool status = false;
    status = del_constraint(s);
    if (!status)
        if (verbose_1)
            cout << "Error, couldn't delete constraint\n";
    status = add_constraint(s, c);

    return status;
}

void
STN::invalidate_ub(set<int>& invalidated_nodes, int x)
{
    if (verbose_2)
        cout << "\tInvalidating upper bounds\n";

    shared_ptr<Node> u = nullptr, v = nullptr;
    u = graph[x];
    shared_ptr<Edge> e = u->edges_out;

    while (e != nullptr) {
        v = graph[e->id];

        if (v->ub_data->pred == u->id) {
            if (v->id == 0)
                if (verbose_2 && verbose_3)
                    cout << "\t\tRESETTING CZ\n";
            v->ub_data->pred = -1;
            v->ub_data->dist = inf;
            invalidated_nodes.insert(v->id);
            invalidate_ub(invalidated_nodes, v->id);
        }

        e = e->next;
    }
}

void
STN::invalidate_lb(set<int>& invalidated_nodes, int x)
{
    if (verbose_2)
        cout << "\tInvalidating lower bounds\n";

    shared_ptr<Node> u = nullptr, v = nullptr;
    u = graph[x];
    shared_ptr<Edge> e = u->edges_in;

    while (e != nullptr) {
        v = graph[e->id];

        if (v->lb_data->pred == u->id) {
            if (v->id == 0)
                if (verbose_2)
                    cout << "\t\tRESETTING CZ\n";
            v->lb_data->pred = -1;
            v->lb_data->dist = inf;
            invalidated_nodes.insert(v->id);
            invalidate_lb(invalidated_nodes, v->id);
        }

        e = e->next;
    }
}

deque<int>
STN::invalidate_nodes(int x_id, int y_id)
{
    if (verbose_2)
        cout << "\tInvalidating nodes\n";

    shared_ptr<Node> u = graph[x_id];
    shared_ptr<Node> v = graph[y_id];
    shared_ptr<Edge> e = nullptr;
    set<int> invalidated_nodes = set<int>();
    deque<int> q = deque<int>();

    if (v->ub_data->pred == u->id) {
        v->ub_data->pred = -1;
        v->ub_data->dist = inf;
        invalidated_nodes.insert(v->id);
        if (verbose_2 && verbose_3)
            cout << "\t\t[UB] Pred of " << v->id << ": " << u->id << "\n";
        invalidate_ub(invalidated_nodes, v->id);

    } else if (u->ub_data->pred == v->id) {
        u->ub_data->pred = -1;
        u->ub_data->dist = inf;
        invalidated_nodes.insert(u->id);
        if (verbose_2 && verbose_3)
            cout << "\t\t[UB] Pred of " << u->id << ": " << v->id << "\n";
        invalidate_ub(invalidated_nodes, u->id);
    }

    if (v->lb_data->pred == u->id) {
        v->lb_data->pred = -1;
        v->lb_data->dist = inf;
        invalidated_nodes.insert(v->id);
        if (verbose_2 && verbose_3)
            cout << "\t\t[LB] Pred of " << v->id << ": " << u->id << "\n";
        invalidate_lb(invalidated_nodes, v->id);

    } else if (u->lb_data->pred == v->id) {
        u->lb_data->pred = -1;
        u->lb_data->dist = inf;
        invalidated_nodes.insert(u->id);
        if (verbose_2 && verbose_3)
            cout << "\t\t[LB] Pred of " << u->id << ": " << v->id << "\n";
        invalidate_lb(invalidated_nodes, u->id);
    }

    shared_ptr<Node> temp_1 = nullptr, temp_2 = nullptr;
    for (set<int>::iterator i = invalidated_nodes.begin(); i != invalidated_nodes.end(); i++) {
        temp_1 = graph[*i];
        e = temp_1->edges_in;
        while (e != nullptr) {
            temp_2 = graph[e->id];
            if (verbose_2 && verbose_3)
                cout << "\t\tV: " << temp_2->timepoint_id << "\n";
            if (!temp_2->in_queue) {
                temp_2->in_queue = true;
                temp_2->ub_data->prop = true;
                temp_2->lb_data->prop = true;
                q.push_back(temp_2->id);
            }

            e = e->next;
        }
    }

    return q;
}

bool
STN::del_constraint(string s)
{
    if (verbose_1)
        cout << "Deleting constraint: " << s << endl;

    int x_id = 0, y_id = 0;
    string x = "", y = "";
    constraint c = constraint();

    map<string, constraint>::iterator i = constraints.find(s);

    if (i == constraints.end()) {
        if (verbose_1)
            cout << "No matching constraint for name: " << s << endl;
        return true;

    } else {
        c = i->second;
    }

    x = get<0>(c);
    y = get<1>(c);

    x_id = get_tp_id(x);
    y_id = get_tp_id(y);

    if (x_id == -1) {
        if (verbose_1 && verbose_3)
            printf("\t\tConstraint <%s, %s, %f, %f> not found in STN\n",
                   get<0>(c).c_str(),
                   get<1>(c).c_str(),
                   get<2>(c),
                   get<3>(c));
        return false;
    }

    if (y_id == -1) {
        if (verbose_1 && verbose_3)
            printf("\t\tConstraint <%s, %s, %f, %f> not found in STN\n",
                   get<0>(c).c_str(),
                   get<1>(c).c_str(),
                   get<2>(c),
                   get<3>(c));
        return false;
    }

    del_edge(x_id, y_id, s);
    del_edge(y_id, x_id, s);
    deque<int> q = invalidate_nodes(x_id, y_id);

    bool f = propagation(-1, -1, &q);
    constraints.erase(s);

    return f;
}

shared_ptr<Edge>
STN::del_all_edges(shared_ptr<Edge> e, int y, bool del_name)
{
    shared_ptr<Edge> head = e;
    while (e != nullptr && e->id == y) {
        if (del_name)
            constraints.erase(e->c_id);
        e = e->next;
    }

    while (head->next != nullptr) {
        if (head->next->id == y) {
            if (del_name)
                constraints.erase(head->next->c_id);
            shared_ptr<Edge> tmp = head->next;
            head->next = head->next->next;

        } else
            head = head->next;
    }

    return e;
}

bool
STN::del_all_constraints(string x, string y)
{
    if (verbose_1)
        cout << "Deleting all constraints between " << x << " and " << y << endl;

    int x_id = get_tp_id(x);
    int y_id = get_tp_id(y);
    shared_ptr<Node> x_node = graph[x_id];
    shared_ptr<Node> y_node = graph[y_id];
    shared_ptr<Edge> e = nullptr;

    e = x_node->edges_out;
    x_node->edges_out = del_all_edges(e, y_id, true);

    e = x_node->edges_in;
    x_node->edges_in = del_all_edges(e, y_id, true);

    e = y_node->edges_out;
    y_node->edges_out = del_all_edges(e, x_id, false);

    e = y_node->edges_in;
    y_node->edges_in = del_all_edges(e, x_id, false);

    deque<int> q = invalidate_nodes(x_id, y_id);
    if (verbose_3) {
        cout << "Deleted constraint 1 = " << x << endl;
        cout << "Deleted constraint 2 = " << y << endl;
        cout << "Invalidate Size Nodes = " << q.size() << endl;
    }
    bool f = propagation(-1, -1, &q);
    return f;
}

void
STN::destroy()
{
    if (verbose_1)
        cout << "Deleting STN " << this->id << "\n";

    shared_ptr<Node> x = nullptr;
    shared_ptr<Edge> e = nullptr, p = nullptr;

    for (size_t i = 0; i < (size_t)this->n; i++) {
        x = this->graph[i];

        if (x != nullptr) {
            e = x->edges_out;
            while (e != nullptr) {
                p = e;
                e = e->next;
            }

            e = x->edges_in;
            while (e != nullptr) {
                p = e;
                e = e->next;
            }
        }
    }
}
