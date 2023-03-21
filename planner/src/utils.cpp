#include "utils.hpp"

string
validation_state::to_string() const
{
    string result = "Validation State: ";
    result += "\t Token = " + failing_token.to_string() + "\n";
    result += "\t Source vertex = " + source.to_string() + "\n";
    result += "\t Sink vertex = " + sink.to_string() + "\n";
    result += "\t World State = ";
    for (pair<string, object_state> ws : current_state)
        result += "\t\t" + ws.second.to_string() + "\n";
    result += "\t Status = " + std::to_string(status) + "\n";
    return result;
}

double
compute_makespan(Plan* p)
{
    double makespan = -1.0 * std::numeric_limits<double>::infinity();
    vector<Timeline> tls = p->get_timelines();
    for (Timeline tl : tls)
        if (tl.get_resource() == "robot") {
            vector<Token> tks = tl.get_tokens();
            Token last = tks.end()[-2];
            if (last.get_name() != "head" && last.get_name() != "tail") {
                tuple<double, double> last_end_bounds = stn.get_feasible_values(last.get_end());
                if (abs(get<0>(last_end_bounds)) >= makespan)
                    makespan = abs(get<0>(last_end_bounds));
            }
        }
    return makespan;
}

double
compute_total_actions(Plan* p)
{
    double total_tokens = 0.0;
    vector<Timeline> tls = p->get_timelines();
    for (Timeline tl : tls)
        if (tl.get_resource() == "robot") {
            vector<Token> tks = tl.get_tokens();
            total_tokens += (double)(tks.size()) - 2;
        }
    return total_tokens;
}

pair<double, double>
compute_metrics(Plan* p)
{
    double makespan = compute_makespan(p);
    double total_tokens = compute_total_actions(p);
    return make_pair(total_tokens, makespan);
}

// TODO: There are the following issues that need to be fixed.
// 1. When a precondition is checked at the start of the action whose repair effect is taking place
// at the end of that action then how do we tackle that? Support robot 2 is at D and robot 1 is at
// C. At start of the robot 2's movement to C we check if its path is clear which its not but the
// repair operator that is moving the robot 1 out of the way has been called and since its effect
// only takes place at the end, it is causing a check failure even though timeline wise they look
// fine. [5x5]
// 2. If the plan validator fails then we nede to repair the plan. We can either repair it as each
// request gets processed or at the very end. Leaning towards the first option

validation_state
plan_validator(Plan* p)
{

    double makespan = compute_makespan(p);
    world_state current_state = initial_state;

    set<string> robots = set<string>();
    string rail_block_attribute_name = "";
    vector<Timeline> tls = p->get_timelines();
    map<Token, task> token_task_mapping = map<Token, task>();
    for (Timeline t : tls) {
        if (t.get_resource() == "robot") {
            robots.insert(t.get_id());
            if (rail_block_attribute_name == "")
                for (arg_and_type vars : current_state[t.get_id()].attribute_types.vars)
                    if (vars.second == "rail_block") {
                        rail_block_attribute_name = vars.first;
                        break;
                    }
            vector<Token> tks = t.get_tokens();
            for (Token tk : tks) {
                task tk_task = task();
                for (task t : primitive_tasks)
                    if (t.name == tk.get_name()) {
                        tk_task = t;
                        break;
                    }
                token_task_mapping.insert(make_pair(tk, tk_task));
            }
        }
    }

    for (double time = 0; time <= makespan; time += globalMinDuration) {
        // Going to use earliest start and finish times for maintaining world state and checking
        // preconditions against that world state

        int robot_counter = 0;
        vector<string> robot_locations = vector<string>();
        robot_locations.resize((int)robots.size());
        // First we update the current state of the world til the current timestep
        for (Timeline t : tls) {
            if (t.get_resource() == "robot") {
                vector<Token> tks = t.get_tokens();
                for (Token tk : tks) {
                    if (tk.get_name() == "head" || tk.get_name() == "tail")
                        continue;
                    stn_bounds tk_end_bounds = stn.get_feasible_values(tk.get_end());

                    // If the EFT of the token is same as the current time then we need to update
                    // the world state of the robot by applying the effects of this token
                    if (abs(get<0>(tk_end_bounds)) == time) {

                        // Check to ensure that the source - sink movement is correct i.e there is
                        // an edge between the source and sink vertex of the rail_move operator
                        // inside the graph. This part computes the source vertex. After updating
                        // the robot state we compute the sink vertex and the check for the edge
                        if (tk.get_name() == "rail_move") {

                            for (arg_and_type as : current_state[t.get_id()].attribute_states.vars)
                                if (as.first == rail_block_attribute_name)
                                    robot_locations[robot_counter] = as.second;
                        }

                        // Need to change the state of the other resources as well
                        for (literal eff : tk.get_effects())
                            if (eff.predicate == dummy_equal_literal &&
                                eff.timed == timed_type::END &&
                                eff.temp_qual == temporal_qualifier_type::AT) {

                                string arg1 = eff.arguments[0];
                                string arg2 = eff.arguments[1];
                                arg_and_type arg1_n_attr = arg_and_type();

                                // If second argument is a variable then ground it
                                if (arg2[0] == '?')
                                    for (arg_and_type k : tk.get_knowns())
                                        if (k.first == arg2)
                                            arg2 = k.second;

                                // Case when the predicate was pushed via the task network but the
                                // argument does not exist within its local context.
                                if (arg2[0] == '?')
                                    continue;

                                // Only first argument can have attributes as the second one is
                                // going to be assigned to the first
                                if (arg1.find(".") != string::npos)
                                    arg1_n_attr = get_arg_and_attr(arg1);

                                // First argument always has attributes. Otherwise it wont be
                                // resource which has object state maintained
                                if (arg1_n_attr != arg_and_type())
                                    for (arg_and_type k : tk.get_knowns())
                                        if (k.first == arg1_n_attr.first) {
                                            object_state other_s = current_state.at(k.second);
                                            for (auto it = other_s.attribute_states.vars.begin();
                                                 it != other_s.attribute_states.vars.end();
                                                 it++)
                                                if (it->first == arg1_n_attr.second)
                                                    it->second = arg2;
                                            current_state[k.second] = other_s;
                                        }
                            }

                        object_state rs = current_state.at(t.get_id());
                        update_object_state(&rs, &tk);
                        current_state[t.get_id()] = rs;
                    }
                }
                robot_counter++;
            }
        }

        // Now we check the preconditions and rail block constraints against the current state of
        // the world
        robot_counter = 0;
        for (Timeline t : tls) {
            if (t.get_resource() == "robot") {
                vector<Token> tks = t.get_tokens();
                for (Token tk : tks) {

                    if (tk.get_name() == "head" || tk.get_name() == "tail")
                        continue;

                    stn_bounds tk_start_bounds = stn.get_feasible_values(tk.get_start());
                    stn_bounds tk_end_bounds = stn.get_feasible_values(tk.get_end());

                    // If the EFT of the token is same as the current time then we need to update
                    // the world state of the robot by applying the effects of this token
                    if (abs(get<0>(tk_end_bounds)) == time) {

                        // This completes the check for the rail_blocks case as described before
                        if (tk.get_name() == "rail_move") {

                            vertex source = vertex(), sink = vertex();
                            source.id = robot_locations[robot_counter];

                            for (arg_and_type as : current_state[t.get_id()].attribute_states.vars)
                                if (as.first == rail_block_attribute_name)
                                    sink.id = as.second;

                            if (source.id != sink.id) {
                                vertex_t t = get_vertex(sink.id, rail_network);
                                vertex_t s = get_vertex(source.id, rail_network);

                                if (!boost::edge(s, t, rail_network.adj_list).second)
                                    return validation_state(&tk,
                                                            source,
                                                            sink,
                                                            &current_state,
                                                            validation_exception::EDGE_SKIP);
                            } else {
                                return validation_state(&tk,
                                                        source,
                                                        sink,
                                                        &current_state,
                                                        validation_exception::VERTEX_SAME);
                            }
                        }
                    }

                    // If the EST of the token is same as the current time then we need to check the
                    // "at start" preconditions against the world state and ensure that they are
                    // satisfied. From then on till the EFT we nede to check that the "over all"
                    // preconditions are also satisfied
                    if (abs(get<0>(tk_start_bounds)) == time) {
                        // Check the "at start" preconditions
                        for (literal prec : tk.get_preconditions()) {
                            if (prec.timed == timed_type::START &&
                                prec.temp_qual == temporal_qualifier_type::AT) {

                                task tk_task = token_task_mapping[tk];
                                if (find(tk_task.prec.begin(), tk_task.prec.end(), prec) !=
                                    tk_task.prec.end()) {
                                    set<arg_and_type> tk_knowns = tk.get_knowns();
                                    bool succ =
                                      check_precondition(&prec, &tk_knowns, &current_state, &init);
                                    if (!succ)
                                        return validation_state(&tk,
                                                                vertex(),
                                                                vertex(),
                                                                &current_state,
                                                                validation_exception::PREC_FAIL);
                                }
                            }
                        }
                    }

                    if (abs(get<0>(tk_start_bounds)) < time && abs(get<0>(tk_end_bounds)) > time) {
                        // Check the "over all" preconditions
                        for (literal prec : tk.get_preconditions()) {
                            if (prec.timed == timed_type::ALL &&
                                prec.temp_qual == temporal_qualifier_type::OVER) {

                                task tk_task = token_task_mapping[tk];
                                if (find(tk_task.prec.begin(), tk_task.prec.end(), prec) !=
                                    tk_task.prec.end()) {
                                    set<arg_and_type> tk_knowns = tk.get_knowns();
                                    bool succ =
                                      check_precondition(&prec, &tk_knowns, &current_state, &init);
                                    if (!succ)
                                        return validation_state(&tk,
                                                                vertex(),
                                                                vertex(),
                                                                &current_state,
                                                                validation_exception::PREC_FAIL);
                                }
                            }
                        }
                    }
                }
                robot_counter++;
            }
        }

        // Additional check that a rail block can only be occupied by one robot at a time
        set<string> occupied_blocks = set<string>();
        for (string robot : robots) {
            for (arg_and_type vars : current_state[robot].attribute_states.vars)
                if (vars.first == rail_block_attribute_name) {
                    occupied_blocks.insert(vars.second);
                    break;
                }
        }

        Token dummy_token = Token();
        if (robots.size() != occupied_blocks.size())
            return validation_state(
              &dummy_token, vertex(), vertex(), &current_state, validation_exception::MULTI_BLOCK);
    }

    return validation_state();
}
