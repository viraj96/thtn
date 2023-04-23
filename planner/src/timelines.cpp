#include "timelines.hpp"

string
Token::to_string() const
{
    string result = "Token: ";

    result += "\n\t Name = " + name;

    task tk_task = task();
    for (task t : primitive_tasks)
        if (t.name == name) {
            tk_task = t;
            break;
        }
    for (arg_and_type k : knowns)
        for (arg_and_type tk_var : tk_task.vars)
            if (tk_var.first == k.first)
                result += " " + k.second + " ";

    result += "\n\t Start Timepoint = " + start_t;
    tuple<double, double> bounds = stn.get_feasible_values(start);
    // tuple<double, double> bounds = stn.get_feasible_values(start_t);
    result +=
      " (" + std::to_string(abs(get<0>(bounds))) + ", " + std::to_string(get<1>(bounds)) + ")";

    result += "\n\t End Timepoint = " + end_t;
    bounds = stn.get_feasible_values(end);
    // bounds = stn.get_feasible_values(end_t);
    result +=
      " (" + std::to_string(abs(get<0>(bounds))) + ", " + std::to_string(get<1>(bounds)) + ")\n";

    return result;
}

bool
operator<(const Token& lhs, const Token& rhs)
{
    string l_start = lhs.get_start();
    string l_end = lhs.get_end();
    string l_name = lhs.get_name();
    bool l_external = lhs.is_external();
    string l_resource = lhs.get_resource();
    string l_request_id = lhs.get_request_id();

    string r_start = rhs.get_start();
    string r_end = rhs.get_end();
    string r_name = rhs.get_name();
    bool r_external = rhs.is_external();
    string r_resource = rhs.get_resource();
    string r_request_id = rhs.get_request_id();

    return tie(l_start, l_end, l_name, l_resource, l_external, l_request_id) <
           tie(r_start, r_end, r_name, r_resource, r_external, r_request_id);
}

bool
operator==(const Token& lhs, const Token& rhs)
{
    string l_start = lhs.get_start();
    string l_end = lhs.get_end();
    string l_name = lhs.get_name();
    bool l_external = lhs.is_external();
    string l_resource = lhs.get_resource();
    string l_request_id = lhs.get_request_id();

    string r_start = rhs.get_start();
    string r_end = rhs.get_end();
    string r_name = rhs.get_name();
    bool r_external = rhs.is_external();
    string r_resource = rhs.get_resource();
    string r_request_id = rhs.get_request_id();

    return tie(l_start, l_end, l_name, l_resource, l_external, l_request_id) ==
           tie(r_start, r_end, r_name, r_resource, r_external, r_request_id);
}

bool
operator!=(const Token& lhs, const Token& rhs)
{
    return !(lhs == rhs);
}

string
Auxiliary_Token::to_string() const
{
    string result = "Auxiliary Token: ";

    result += "\n\t Name = " + name;

    result += "\n\t Start Timepoint = " + start_t;
    tuple<double, double> bounds = stn.get_feasible_values(start);
    // tuple<double, double> bounds = stn.get_feasible_values(start_t);
    result += " (" + std::to_string(get<0>(bounds)) + ", " + std::to_string(get<1>(bounds)) + ")";

    result += "\n\t End Timepoint = " + end_t;
    bounds = stn.get_feasible_values(end);
    // bounds = stn.get_feasible_values(end_t);
    result += " (" + std::to_string(get<0>(bounds)) + ", " + std::to_string(get<1>(bounds)) + ")\n";

    return result;
}

string
Timeline::to_string() const
{
    string result = "Timeline: ";
    result += "\n\t Id = " + id + "\n";
    result += "\t Resource = " + resource + "\n";
    result += "\t Tokens = \n";
    for (Token tk : tokens)
        result += "\t\t " + tk.to_string() + "\n";

    return result;
}

string
Plan::to_string() const
{
    string result = "Plan: ";
    result += "\n\t Timelines = \n";
    for (Timeline t : timelines) {
        result += "\t\t " + t.to_string() + "\n";
    }
    result += "\n\t Number of assignments to each robot = \n";
    for (pair<string, int> num : num_tasks_robot)
        result += "\t\t" + num.first + " = " + std::to_string(num.second) + "\n";

    return result;
}

string
task_vertex::to_string() const
{
    string result = "Task Vertex: ";

    result += "\n\t Id = " + id + "\n";
    result += "\t Token = " + tk.to_string() + "\n";
    result += "\t Parent Id = " + parent->id + "\n";

    return result;
}

string
task_network::to_string() const
{
    string result = "Task Network: ";
    result += "\n\t Id = " + id + "\n";

    result += "\t Adjacency List = \n";
    task_edge_it it = task_edge_it(), end = task_edge_it();
    for (tie(it, end) = edges(adj_list); it != end; it++)
        result += "\t\t " + adj_list[source(*it, adj_list)].id + " -> " +
                  adj_list[target(*it, adj_list)].id + "\n";

    return result;
}

void
Token::set_external()
{
    this->external = true;
}

bool
Token::is_external() const
{
    return this->external;
}

void
Token::set_name(string _name)
{
    this->name = _name;
}

string
Token::get_name() const
{
    return this->name;
}

void
Token::set_resource(string _resource)
{
    this->resource = _resource;
}

string
Token::get_resource() const
{
    return this->resource;
}

void
Token::set_request_id(string _request_id)
{
    this->request_id = _request_id;
}

string
Token::get_request_id() const
{
    return this->request_id;
}

bool
Token::create_timepoints(double duration)
{
    int start_tp = 0, end_tp = 0;
    start_tp = stn.num_points() + 1;
    end_tp = start_tp + 1;
    this->start_t = this->resource + "_" + this->name + "_start" + std::to_string(start_tp);
    this->end_t = this->resource + "_" + this->name + "_end" + std::to_string(end_tp);

    this->start = stn.add_timepoint(this->start_t);
    this->end = stn.add_timepoint(this->end_t);

    if (duration == 0.0)
        duration = zero;

    constraint c = constraint();
    if (duration == zero && this->name != "head" && this->name != "tail")
        c = make_tuple(this->start_t, this->end_t, zero, inf);
    else if (duration == zero)
        c = make_tuple(this->start_t, this->end_t, zero, zero);
    else
        c = make_tuple(this->start_t, this->end_t, duration, duration);
    string c_name = this->start_t + duration_constraint + this->end_t;
    bool status =
      stn.add_constraint(this->start, this->end, c_name, make_pair(get<2>(c), get<3>(c)));
    // bool status = stn.add_constraint(c_name, c);

    if (!status)
        cout << "Constraint between (" + this->start_t + ", " + this->end_t +
                  ") was not created successfully!\n";
    return status;
}

bool
Token::create_timepoints(
  STN* stn,
  double duration,
  stack<tuple<STN_operation_type, constraint, string>>* search_operation_history)
{
    int start_tp = 0, end_tp = 0;
    start_tp = stn->num_points() + 1;
    end_tp = start_tp + 1;
    this->start_t = this->resource + "_" + this->name + "_start" + std::to_string(start_tp);
    this->end_t = this->resource + "_" + this->name + "_end" + std::to_string(end_tp);

    this->start = stn->add_timepoint(this->start_t);
    this->end = stn->add_timepoint(this->end_t);

    if (search_operation_history != nullptr) {
        search_operation_history->push(
          make_tuple(STN_operation_type::ADD_TIMEPOINT, constraint(), this->start_t));
        search_operation_history->push(
          make_tuple(STN_operation_type::ADD_TIMEPOINT, constraint(), this->end_t));
    }

    if (duration == 0.0)
        duration = zero;

    constraint c = constraint();
    if (duration == zero && this->name != "head" && this->name != "tail")
        c = make_tuple(this->start_t, this->end_t, zero, inf);
    else if (duration == zero)
        c = make_tuple(this->start_t, this->end_t, zero, zero);
    else
        c = make_tuple(this->start_t, this->end_t, duration, duration);

    string c_name = this->start_t + duration_constraint + this->end_t;
    bool status =
      stn->add_constraint(this->start, this->end, c_name, make_pair(get<2>(c), get<3>(c)));
    // bool status = stn->add_constraint(c_name, c);

    if (!status)
        cout << "Constraint between (" + this->start_t + ", " + this->end_t +
                  ") was not created successfully!\n";
    else {
        if (search_operation_history != nullptr)
            search_operation_history->push(
              make_tuple(STN_operation_type::ADD_CONSTRAINT, c, c_name));
    }

    return status;
}

string
Token::get_end() const
{
    return this->end_t;
}

string
Token::get_start() const
{
    return this->start_t;
}

shared_ptr<Node>
Token::get_end_timepoint() const
{
    assert(this->end != nullptr);
    return this->end;
}

shared_ptr<Node>
Token::get_start_timepoint() const
{
    assert(this->start != nullptr);
    return this->start;
}

void
Token::add_effects(literal eff)
{
    this->effect_literals.insert(eff);
}

void
Token::add_effects(vector<literal> eff)
{
    for (literal l : eff)
        this->effect_literals.insert(l);
}

set<literal>
Token::get_effects() const
{
    return this->effect_literals;
}

void
Token::add_preconditions(literal prec)
{
    this->precondition_literals.insert(prec);
}

void
Token::add_preconditions(vector<literal> prec)
{
    for (literal l : prec)
        this->precondition_literals.insert(l);
}

set<literal>
Token::get_preconditions() const
{
    return this->precondition_literals;
}

void
Token::add_knowns(arg_and_type assignment)
{
    this->knowns.insert(assignment);
}

void
Token::add_knowns(vector<arg_and_type> assignments)
{
    for (arg_and_type a : assignments)
        this->knowns.insert(a);
}

void
Token::change_known(arg_and_type new_known)
{
    auto it = find_if(this->knowns.begin(), this->knowns.end(), [&new_known](const arg_and_type k) {
        return new_known.first == k.first;
    });
    if (it != this->knowns.end()) {
        this->knowns.erase(it);
        this->knowns.insert(new_known);
    }
}

set<arg_and_type>
Token::get_knowns() const
{
    return this->knowns;
}

void
Token::add_arguments(arg_and_type argument)
{
    this->arguments.insert(argument);
}

void
Token::add_arguments(vector<arg_and_type> arguments)
{
    for (arg_and_type a : arguments)
        this->arguments.insert(a);
}

set<arg_and_type>
Token::get_arguments() const
{
    return this->arguments;
}

string
Timeline::get_id()
{
    return this->id;
}

string
Timeline::get_resource()
{
    return this->resource;
}

void
Timeline::add_token(Token t)
{
    this->tokens.push_back(t);
}

void
Timeline::del_token(int token_loc)
{
    assert(token_loc >= 0);
    assert(token_loc < int(this->tokens.size()));
    this->tokens.erase(this->tokens.begin() + token_loc);
}

void
Timeline::del_tokens(pair<int, int> range)
{
    assert(range.first >= 0);
    assert(range.first < int(this->tokens.size()));
    assert(range.second >= 0);
    assert(range.second < int(this->tokens.size()));
    this->tokens.erase(this->tokens.begin() + range.first, this->tokens.begin() + range.second);
}

void
Timeline::insert_token(Token t, Token prev, Token next)
{
    int position = 0, counter = 1;
    for (auto it = this->tokens.begin(); it != this->tokens.end() - 1; it++) {
        if (*it == prev && *(it + 1) == next)
            position = counter;
        counter++;
    }

    assert(position != 0);
    this->tokens.insert(this->tokens.begin() + position, t);
}

vector<Token>
Timeline::get_tokens()
{
    return this->tokens;
}

void
Timeline::set_id(string _id)
{
    this->id = _id;
}

void
Timeline::set_resource(string _resource)
{
    this->resource = _resource;
}

void
Plan::add_timeline(Timeline t)
{
    this->timelines.push_back(t);
}

vector<Timeline>
Plan::get_timelines()
{
    return this->timelines;
}

Timeline*
Plan::get_timelines(string id)
{
    Timeline* t = nullptr;
    for (auto it = this->timelines.begin(); it != this->timelines.end(); it++)
        if (it->get_id() == id) {
            t = &*it;
            break;
        }

    return t;
}

Timeline*
Plan::get_timelines(string id, string resource)
{
    Timeline* t = nullptr;
    for (auto it = this->timelines.begin(); it != this->timelines.end(); it++)
        if (it->get_id() == id && it->get_resource() == resource) {
            t = &*it;
            break;
        }

    return t;
}

Token
instantiate_token(string name, string resource, string request_id, double duration)
{
    if (name == "head" || name == "tail") {
        Auxiliary_Token atk(name);
        atk.set_resource(resource);
        atk.create_timepoints();
        return move(atk);

    } else {
        Token tk = Token();
        tk.set_name(name);
        tk.set_resource(resource);
        tk.set_request_id(request_id);
        tk.create_timepoints(duration);
        return tk;
    }
}

Token
instantiate_token(string name,
                  string resource,
                  STN* stn,
                  string request_id,
                  double duration,
                  stack<tuple<STN_operation_type, constraint, string>>* search_operation_history)
{
    if (name == "head" || name == "tail") {
        Auxiliary_Token atk(name);
        atk.set_resource(resource);
        atk.create_timepoints();
        return move(atk);

    } else {
        Token tk = Token();
        tk.set_name(name);
        tk.set_resource(resource);
        tk.set_request_id(request_id);
        tk.create_timepoints(stn, duration, search_operation_history);
        return tk;
    }
}

Timeline
instantiate_timeline(string id, string resource)
{
    Timeline t = Timeline();
    t.set_id(id);
    t.set_resource(resource);
    Token head = instantiate_token("head", resource);
    Token tail = instantiate_token("tail", resource);
    t.add_token(head);
    t.add_token(tail);

    // Identify the parent of the resource, whether its a rail_block or item
    string parent = "";
    for (pair<string, set<string>> m : sorts)
        for (string s : m.second)
            if (s == resource) {
                parent = m.first;
                break;
            }

    string c_name = head.get_end() + sequencing_constraint + tail.get_start();
    constraint c = make_tuple(head.get_end(), tail.get_start(), zero, inf);
    assert(stn.add_constraint(head.get_end_timepoint(),
                              tail.get_start_timepoint(),
                              c_name,
                              make_pair(get<2>(c), get<3>(c))));
    // assert(stn.add_constraint(c_name, c));

    return t;
}

Plan
instantiate_plan()
{
    Plan p = Plan();
    for (sort_definition sd : sort_definitions)
        if (sd.has_parent_sort && sd.parent_sort == "discrete_reusable_resource")
            // Initialize timelines for this resource
            for (string ds : sd.declared_sorts)
                for (string s : sorts[ds]) {
                    if (find(constants[ds].begin(), constants[ds].end(), s) != constants[ds].end())
                        continue;

                    Timeline t = instantiate_timeline(s, ds);
                    p.add_timeline(t);
                }
    return p;
}

task_vertex_t
get_vertex(string id, task_network G)
{
    task_vertex_it v = task_vertex_it(), vend = task_vertex_it();
    for (boost::tie(v, vend) = boost::vertices(G.adj_list); v != vend; v++)
        if (G.adj_list[*v].tk.get_name() == id)
            return *v;

    return task_vertex_t();
}

void
add_sync_constraints(method m, vector<Token> tokens)
{
    for (sync_constraints s : m.synchronize_constraints) {
        auto it0 =
          find_if(m.ps.begin(), m.ps.end(), [&s](const plan_step& p) { return p.id == s.task1; });
        auto it1 =
          find_if(m.ps.begin(), m.ps.end(), [&s](const plan_step& p) { return p.id == s.task2; });

        vector<Token> first = vector<Token>(), last = vector<Token>();
        for (Token tk : tokens) {
            if (tk.get_name().substr(0, tk.get_name().find("-")) == it0->task)
                first.push_back(tk);
            else if (tk.get_name().substr(0, tk.get_name().find("-")) == it1->task)
                last.push_back(tk);
        }

        for (Token tk1 : first)
            for (Token tk2 : last) {
                double lb = zero, ub = zero;
                if (s.lower_bound == -inf_int)
                    lb = -inf;
                else
                    lb = (double)s.lower_bound;

                if (lb == 0.0)
                    lb = zero;

                if (s.upper_bound == inf_int)
                    ub = inf;
                else
                    ub = (double)s.upper_bound;

                if (ub == 0)
                    ub = zero;

                string sync_name = "";
                if (ub == lb && ub == zero)
                    sync_name = tk1.get_end() + meets_constraint + tk2.get_start();
                else
                    sync_name = tk1.get_end() + before_constraint + tk2.get_start();
                constraint sync = make_tuple(tk1.get_end(), tk2.get_start(), lb, ub);
                assert(stn.add_constraint(tk1.get_end_timepoint(),
                                          tk2.get_start_timepoint(),
                                          sync_name,
                                          make_pair(lb, ub)));
                // assert(stn.add_constraint(sync_name, sync));
            }
    }
}

void
add_contains_constraints(method m, Token parent, vector<Token> children)
{
    if (!m.ordering.size()) {
        for (Token tk : children) {
            string tkname = tk.get_name();
            string at = tkname.substr(0, tkname.find("-"));
            tkname.erase(0, tkname.find("-") + 1);
            string mname = tkname.substr(0, tkname.find("-"));

            if (mname != m.name) {
                string st_name = parent.get_start() + contains_constraint + tk.get_start();
                constraint contains_st = make_tuple(parent.get_start(), tk.get_start(), zero, inf);
                string et_name = tk.get_end() + contains_constraint + parent.get_end();
                constraint contains_et = make_tuple(tk.get_end(), parent.get_end(), zero, inf);

                assert(stn.add_constraint(parent.get_start_timepoint(),
                                          tk.get_start_timepoint(),
                                          st_name,
                                          make_pair(get<2>(contains_st), get<3>(contains_st))));
                assert(stn.add_constraint(tk.get_end_timepoint(),
                                          parent.get_end_timepoint(),
                                          et_name,
                                          make_pair(get<2>(contains_et), get<3>(contains_et))));
                // assert(stn.add_constraint(st_name, contains_st));
                // assert(stn.add_constraint(et_name, contains_et));
            }
        }

    } else {
        int outer = 0, inner = 0;
        string first_step = "", last_step = "";
        for (arg_and_type o : m.ordering) {
            outer++;
            if (o.first != "mprec" && o.second != "mprec") {
                inner++;
                if (inner == 1)
                    first_step = o.first;
                if (outer == (int)m.ordering.size())
                    last_step = o.second;
            }
        }

        auto it0 = find_if(m.ps.begin(), m.ps.end(), [&first_step](const plan_step& p) {
            return p.id == first_step;
        });
        auto it1 = find_if(
          m.ps.begin(), m.ps.end(), [&last_step](const plan_step& p) { return p.id == last_step; });

        assert(it0 != m.ps.end());
        assert(it1 != m.ps.end());

        for (Token tk : children) {
            string name = tk.get_name();
            if (name.substr(0, name.find("-")) == it0->task) {
                string st_name = parent.get_start() + contains_constraint + tk.get_start();
                constraint contains_st = make_tuple(parent.get_start(), tk.get_start(), zero, inf);
                assert(stn.add_constraint(parent.get_start_timepoint(),
                                          tk.get_start_timepoint(),
                                          st_name,
                                          make_pair(get<2>(contains_st), get<3>(contains_st))));
                // assert(stn.add_constraint(st_name, contains_st));

            } else if (name.substr(0, name.find("-")) == it1->task) {
                string et_name = tk.get_end() + contains_constraint + parent.get_end();
                constraint contains_et = make_tuple(tk.get_end(), parent.get_end(), zero, inf);
                assert(stn.add_constraint(tk.get_end_timepoint(),
                                          parent.get_end_timepoint(),
                                          et_name,
                                          make_pair(get<2>(contains_et), get<3>(contains_et))));
                // assert(stn.add_constraint(et_name, contains_et));
            }
        }
    }
}

bool
check_compatibility_of_argument(string arg, vector<arg_and_type> vars, set<arg_and_type> args)
{
    bool local_compat = false;
    string type = "";
    for (arg_and_type v : vars)
        if (v.first == arg) {
            type = v.second;
            break;

        } else if (v.first == arg.substr(0, arg.find("."))) {
            type = v.second;
            break;
        }

    for (pair<string, set<string>> m : constants)
        for (string c : m.second)
            if (c == arg) {
                local_compat = true;
                break;
            }

    for (arg_and_type ag : args)
        if (ag.second == type) {
            local_compat = true;
            break;
        }

    return local_compat;
}

bool
check_compatibility_of_literals(literal l, vector<arg_and_type> vars, set<arg_and_type> args)
{
    bool compatible = false;
    for (string s : l.arguments) {
        bool local_compat = check_compatibility_of_argument(s, vars, args);

        if (!local_compat)
            break;

        compatible = local_compat;
    }

    return compatible;
}

void
add_literals(task_vertex_t node_t, std::shared_ptr<task_vertex> parent, task_network* tn)
{
    string par_name = parent->tk.get_name();
    string par_t_name = par_name.substr(0, par_name.find("-"));
    par_name.erase(0, par_name.find("-") + 1);
    string par_m_name = par_name.substr(0, par_name.find("-"));

    for (method par_m : methods) {
        if (par_m.name == par_m_name) {
            bool first = false, last = false;
            string node_name = tn->adj_list[node_t].tk.get_name();
            string node_t_name = node_name.substr(0, node_name.find("-"));

            if (par_m.ps[0].task == node_t_name)
                first = true;
            if (par_m.ps[par_m.ps.size() - 2].task == node_t_name)
                last = true;

            task_vertex_t par_t = get_vertex(parent->id, *tn);
            task_vertex par = tn->adj_list[par_t];

            // Validate which preconditions to push
            for (literal p : par.tk.get_preconditions()) {
                bool compatible = check_compatibility_of_literals(
                  p, par_m.vars, tn->adj_list[node_t].tk.get_arguments());

                if (p.temp_qual == AT && p.timed == START && first && compatible)
                    tn->adj_list[node_t].tk.add_preconditions(p);

                else if (p.temp_qual == OVER && p.timed == ALL && compatible)
                    tn->adj_list[node_t].tk.add_preconditions(p);
            }

            // Validate which effects to push
            for (literal e : par.tk.get_effects()) {
                bool compatible = check_compatibility_of_literals(
                  e, par_m.vars, tn->adj_list[node_t].tk.get_arguments());

                if (e.temp_qual == AT && e.timed == END && last && compatible)
                    tn->adj_list[node_t].tk.add_effects(e);
            }

            // Validate which known variable assignments to push
            for (arg_and_type k : par.tk.get_knowns()) {
                bool compatible = check_compatibility_of_argument(
                  k.first, par_m.vars, tn->adj_list[node_t].tk.get_arguments());
                if (compatible)
                    tn->adj_list[node_t].tk.add_knowns(k);
            }

            // Validate the open variable assignments to be pushed as they are
            // defined in the parent
            if (par.tk.get_knowns().size() < par.tk.get_arguments().size()) {
                set<arg_and_type> knowns = par.tk.get_knowns();
                for (arg_and_type arg : par.tk.get_arguments()) {
                    auto it = find_if(knowns.begin(), knowns.end(), [&arg](const arg_and_type& k) {
                        return k.first == arg.first;
                    });
                    if (it == knowns.end()) {
                        bool compatible = check_compatibility_of_argument(
                          arg.first, par_m.vars, tn->adj_list[node_t].tk.get_arguments());
                        if (compatible) {
                            arg_and_type k = make_pair(arg.first, dummy_known_name);
                            tn->adj_list[node_t].tk.add_knowns(k);
                        }
                    }
                }
            }
        }
    }
}

vector<Token>
expand(search_vertex t,
       std::shared_ptr<task_vertex> parent,
       task_network* tn,
       set<search_vertex> vert,
       request r)
{
    task pt = task();
    method m = method();
    vector<Token> generated = vector<Token>();

    if (!t.leaf) {
        for (method ms : methods)
            if (ms.name == t.method_name) {
                m = ms;
                break;
            }
    } else {
        for (task pts : primitive_tasks) {
            string task_name = t.id.substr(0, t.id.find("-"));
            if (pts.name == task_name) {
                pt = pts;
                break;
            }
        }
    }

    if (!t.leaf)
        assert(m.name.length() != 0);
    else
        assert(pt.name.length() != 0);

    std::shared_ptr<task_vertex> node = nullptr;

    if (!t.leaf) {
        Token node_tk = instantiate_token(m.at + "-" + m.name, "robot", r.id);
        node_tk.add_effects(task_name_map[method_precondition_action_name + m.name].eff);
        node_tk.add_arguments(task_name_map[method_precondition_action_name + m.name].vars);
        node_tk.add_preconditions(task_name_map[method_precondition_action_name + m.name].prec);

        node = std::make_shared<task_vertex>(task_vertex(node_tk, node_tk.get_name(), parent));
        task_vertex_t node_t = boost::add_vertex(*node, tn->adj_list);

        if (parent != nullptr) {
            edge e = { parent->id, node->id };
            task_vertex_t parent_desc = get_vertex(parent->id, *tn);
            boost::add_edge(parent_desc, node_t, e, tn->adj_list);

            add_literals(node_t, parent, tn);

        } else {
            vector<arg_and_type> knowns = vector<arg_and_type>();
            assert(m.atargs.size() == r.arguments.size());
            for (int i = 0; i < (int)m.atargs.size(); i++) {
                arg_and_type p = make_pair(m.atargs[i], r.arguments[i]);
                knowns.push_back(p);
            }
            tn->adj_list[node_t].tk.add_knowns(knowns);
        }

        generated.push_back(node_tk);

        for (plan_step ps : m.ps) {
            if (ps.task.substr(0, method_precondition_action_name.length()) !=
                method_precondition_action_name) {
                auto it = find_if(vert.begin(), vert.end(), [&ps](const search_vertex& t) {
                    string task_name = t.id.substr(0, t.id.find("-"));
                    return task_name == ps.task;
                });
                if (it != vert.end()) {
                    vector<Token> gen_subtokens = expand(*it, node, tn, vert, r);
                    generated.insert(generated.end(), gen_subtokens.begin(), gen_subtokens.end());
                }
            }
        }

        vector<Token> children = vector<Token>();
        for (plan_step ps : m.ps)
            for (Token tk : generated)
                if (tk.get_name().substr(0, tk.get_name().find("-")) == ps.task)
                    children.push_back(tk);

        add_sync_constraints(m, children);
        add_contains_constraints(m, node_tk, children);

    } else {
        Token node_tk = instantiate_token(pt.name, "robot", r.id, pt.duration);
        node_tk.add_effects(pt.eff);
        node_tk.add_arguments(pt.vars);
        node_tk.add_preconditions(pt.prec);

        node = std::make_shared<task_vertex>(task_vertex(node_tk, node_tk.get_name(), parent));
        task_vertex_t node_t = boost::add_vertex(*node, tn->adj_list);

        if (parent != nullptr) {
            edge e = { parent->id, node->id };
            task_vertex_t parent_desc = get_vertex(parent->id, *tn);
            boost::add_edge(parent_desc, node_t, e, tn->adj_list);

            add_literals(node_t, parent, tn);
        } else
            node = nullptr;

        generated.push_back(node_tk);
    }

    return generated;
}

pair<task_network, vector<arg_and_type>>
instantiate_task_network(search_vertex* root, set<search_vertex> vert, request r)
{
    task_network tn = task_network();
    tn.id = r.id;

    vector<Token> tokens = expand(*root, nullptr, &tn, vert, r);

    task_vertex_it v = task_vertex_it(), vend = task_vertex_it();
    vector<arg_and_type> open = vector<arg_and_type>();
    for (boost::tie(v, vend) = boost::vertices(tn.adj_list); v != vend; v++) {
        if (tn.adj_list[*v].parent == nullptr) {
            double rel_time = (r.release_time == 0.0) ? zero : r.release_time;
            double due_date = (r.due_date == 0.0) ? zero : r.due_date;

            string st_name = "cz" + cz_constraint + tn.adj_list[*v].tk.get_start();
            constraint cz_st = make_tuple("cz", tn.adj_list[*v].tk.get_start(), rel_time, inf);

            string et_name = "cz" + cz_constraint + tn.adj_list[*v].tk.get_end();
            constraint cz_et = make_tuple("cz", tn.adj_list[*v].tk.get_end(), zero, due_date);

            assert(stn.add_constraint(stn.get_cz(),
                                      tn.adj_list[*v].tk.get_start_timepoint(),
                                      st_name,
                                      make_pair(get<2>(cz_st), get<3>(cz_st))));
            assert(stn.add_constraint(stn.get_cz(),
                                      tn.adj_list[*v].tk.get_end_timepoint(),
                                      et_name,
                                      make_pair(get<2>(cz_et), get<3>(cz_et))));
            // assert(stn.add_constraint(st_name, cz_st));
            // assert(stn.add_constraint(et_name, cz_et));
        }

        Token tk = tn.adj_list[*v].tk;
        if (tk.get_arguments().size() != tk.get_knowns().size())
            for (arg_and_type arg : tk.get_arguments()) {
                set<arg_and_type> knowns = tk.get_knowns();
                auto it = find_if(knowns.begin(), knowns.end(), [&arg](const arg_and_type& k) {
                    return k.first == arg.first;
                });
                if (it == knowns.end())
                    open.push_back(arg);
            }
    }

    return make_pair(tn, open);
}

task_network
assign_open_variables(map<string, arg_and_type> var_assign_map, task_network tree)
{
    task_vertex_it vi = task_vertex_it(), vi_end = task_vertex_it();
    for (boost::tie(vi, vi_end) = boost::vertices(tree.adj_list); vi != vi_end; vi++) {
        Token node_tk = tree.adj_list[*vi].tk;
        set<arg_and_type> knowns = node_tk.get_knowns();
        set<arg_and_type> arguments = node_tk.get_arguments();

        for (pair<string, arg_and_type> var : var_assign_map) {
            // Case where open variable was first introduced
            if (arguments.size() != knowns.size()) {
                for (arg_and_type arg : arguments)
                    if (arg == var.second) {
                        arg_and_type var_assign = make_pair(arg.first, var.first);
                        tree.adj_list[*vi].tk.add_knowns(var_assign);
                    }

            }
            // Case where the open variable was pushed to its child
            else {
                for (arg_and_type k : knowns)
                    if (k.second == dummy_known_name && k.first == var.second.first) {
                        arg_and_type var_assign = make_pair(k.first, var.first);
                        tree.adj_list[*vi].tk.change_known(var_assign);
                    }
            }
        }
    }

    return tree;
}
