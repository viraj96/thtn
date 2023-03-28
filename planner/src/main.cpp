#include <getopt.h>

#include "plog/Appenders/ColorConsoleAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Initializers/ConsoleInitializer.h"
#include <plog/Log.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

#define MAX_PERMUTATIONS 10

#include "parser.hpp"
#include "planner.hpp"
#include "search.hpp"
#include "timelines.hpp"
#include "utils.hpp"

using namespace std;

STN stn = STN();
int attempts = 1;
graph rail_network = graph();
int max_recursive_depth = 2;
int globalMinDuration = INT_MAX;
string domain_name = dummy_domain_name;
string problem_name = dummy_problem_name;
world_state initial_state = world_state();
vector<method> methods = vector<method>();
string metric_target = dummy_function_type;
vector<request> requests = vector<request>();
vector<task> abstract_tasks = vector<task>();
vector<task> primitive_tasks = vector<task>();
vector<string> parsed_requests = vector<string>();
map<string, task> task_name_map = map<string, task>();
vector<parsed_task> parsed_abstract = vector<parsed_task>();
vector<parsed_task> parsed_primitive = vector<parsed_task>();
map<string, set<string>> sorts = map<string, set<string>>();
map<string, set<string>> constants = map<string, set<string>>();
vector<sort_definition> sort_definitions = vector<sort_definition>();
vector<predicate_definition> predicate_definitions = vector<predicate_definition>();
map<string, vector<parsed_method>> parsed_methods = map<string, vector<parsed_method>>();
map<string, tasknetwork_solution> request_solutions = map<string, tasknetwork_solution>();
vector<pair<predicate_definition, string>> parsed_functions =
  vector<pair<predicate_definition, string>>();
map<string, set<map<string, var_declaration>>> csorts =
  map<string, set<map<string, var_declaration>>>();

void
find_plan_for_request(request r, Plan* p, bool* plan_found, string metric)
{
    if (task_name_map.find(r.demand.name) == task_name_map.end()) {
        PLOGV << "Request " << r.id << " cannot be met!\n";
    }

    search_tree r_tree = create_search_tree(r);
    PLOGV << r_tree.to_string() << endl;

    search_vertex root = r_tree.adj_list[0];
    vector<vector<string>> leafs = get_leafs(root, r_tree);

    *(plan_found) = false;
    for (vector<string> cand : leafs) {
        set<search_vertex> candidate = get_candidate(cand, r_tree);

        search_vertex root = search_vertex();
        for (search_vertex s : candidate) {
            if (s.parent == nullptr || (s.parent->or_node && s.parent->parent == nullptr))
                root = s;
        }

        pair<task_network, vector<arg_and_type>> ret =
          instantiate_task_network(&root, candidate, r);
        task_network r_net = ret.first;
        vector<arg_and_type> open = ret.second;

        vector<vector<string>> assignments = vector<vector<string>>();
        for (arg_and_type open_arg : open) {
            set<string> assign = sorts[open_arg.second];
            vector<string> v_assign = vector<string>();
            if (assign.size() > 0) {
                copy(assign.begin(), assign.end(), back_inserter(v_assign));
                assignments.push_back(v_assign);
            }
        }

        assignments = cartesian(assignments);
        pq solutions = pq();

        for (vector<string> assign : assignments) {
            assert(assign.size() == 1); // assigning only robots for now
            map<string, arg_and_type> all_assignments = find_assignments(assign[0], open);

            task_network test_net = assign_open_variables(all_assignments, r_net);

            // Try to schedule this network and find the first feasible plan
            pq sol = find_feasible_slots(test_net, *p, assign[0], attempts, metric);
            if ((int)sol.size() > 0)
                solutions.push(sol.top());
        }

        if (solutions.top().metric_value != std::numeric_limits<double>::infinity()) {
            commit_slots(p, &solutions);
            *(plan_found) = true;
        }

        if (*(plan_found))
            break;
    }
}

int
main(int argc, char** argv)
{

    int dfile = -1;
    int pfile = -1;

    bool permute = false;
    string metric = "makespan";
    bool showProperties = false;
    plog::Severity severity = plog::error;
    bool compileConditionalEffects = true;
    bool linearConditionalEffectExpansion = false;
    bool encodeDisjunctivePreconditionsInMethods = false;

    struct option options[] = {

        { "metric", required_argument, NULL, 'm' },
        { "severity", required_argument, NULL, 'd' },
        { "attempts", required_argument, NULL, 't' },
        { "properties", optional_argument, NULL, 'p' },
        { "permute_benchmark", no_argument, NULL, 'b' },
        { "keep-conditional-effects", no_argument, NULL, 'k' },
        { "max-recursive-depth", required_argument, NULL, 'r' },
        { "linear-conditional-effect", no_argument, NULL, 'L' },
        { "encode-disjunctive_preconditions-in-htn", no_argument, NULL, 'D' },
        { NULL, 0, NULL, 0 },

    };

    // Parsing command line options
    bool optionsValid = true;
    while (true) {
        int c = getopt_long_only(argc, argv, "kLDptrmdb", options, NULL);
        if (c == -1)
            break;
        if (c == '?' || c == ':') {
            // Invalid option
            optionsValid = false;
            continue;
        }

        if (c == 'k')
            compileConditionalEffects = false;
        else if (c == 'L') {
            compileConditionalEffects = false;
            linearConditionalEffectExpansion = true;
        } else if (c == 'D')
            encodeDisjunctivePreconditionsInMethods = true;
        else if (c == 'p')
            showProperties = true;
        else if (c == 'm') {
            int choice = atoi(optarg);
            if (choice == 0)
                metric = "makespan";
            else
                metric = "actions";
        } else if (c == 'd') {
            int choice = atoi(optarg);
            severity = static_cast<plog::Severity>(choice);
        } else if (c == 'b') {
            permute = true;
        } else if (c == 'r') {
            int choice = atoi(optarg);
            max_recursive_depth = choice;
        } else if (c == 't') {
            int choice = atoi(optarg);
            attempts = choice;
        }
    }

    if (!optionsValid) {
        cout << "Invalid options! Exiting!" << endl;
        return 1;
    }

    for (int i = optind; i < argc; i++) {
        if (dfile == -1)
            dfile = i;
        else if (pfile == -1)
            pfile = i;
    }

    if (dfile == -1 || pfile == -1) {
        cout << "You need to provide a domain and problem file as input." << endl;
        return 1;
    }

    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(severity, &consoleAppender);

    // Open c-style file handle
    FILE* domain_file = fopen(argv[dfile], "r");
    FILE* problem_file = fopen(argv[pfile], "r");

    if (!domain_file) {
        cout << "Can't open " << argv[dfile] << "!" << endl;
        return 2;
    }

    if (!problem_file) {
        cout << "Can't open " << argv[pfile] << "!" << endl;
        return 2;
    }

    /* cout << "Running the parser on the given domain and problem files!\n"; */
    run_parser(domain_file,
               problem_file,
               argv[dfile],
               argv[pfile],
               showProperties,
               compileConditionalEffects,
               linearConditionalEffectExpansion,
               encodeDisjunctivePreconditionsInMethods);

    map<string, string> block_to_loc;
    block_to_loc["blockA"] = "[0.5,-1.0715,1.03]";
    block_to_loc["blockB"] = "[0.5,-0.7145,1.03]";
    block_to_loc["blockC"] = "[0.5,-0.3575,1.03]";
    block_to_loc["blockD"] = "[0.5,0.0,1.03]";
    block_to_loc["blockE"] = "[0.5,0.3575,1.03]";
    block_to_loc["blockF"] = "[0.5,0.7145,1.03]";
    block_to_loc["blockG"] = "[0.5,1.0715,1.03]";
    ofstream data_file;
    data_file.open("../data/7_blocks_5_requests.json", std::ios::out);
    string constant =
      "{\n\t\"block_bounds\": { \n\t\t\"blockA\" : [-1.25, -0.893], \n\t\t\"blockB\" : [-0.893, "
      "-0.536], \n\t\t\"blockC\" : [-0.536, -0.179], \n\t\t\"blockD\" : [-0.179, 0.179], "
      "\n\t\t\"blockE\" : [0.179, 0.536], \n\t\t\"blockF\" : [0.536, 0.893], \n\t\t\"blockG\" : "
      "[0.893, 1.25] \n\t}, \n\t\"locations\" : { ";
    data_file << constant;
    int c = 0;
    for (int i = 0; i < (int)init.size(); i++) {
        if (init[i].predicate == "reachable" && init[i].timed == timed_type::ALL &&
            init[i].temp_qual == temporal_qualifier_type::OVER && init[i].positive) {
            string block = init[i].args[0], loc = init[i].args[1];
            data_file << "\n\t\t\"" << loc << "\" : " << block_to_loc[block];
            c++;
            if (c != 10)
                data_file << ",";
        }
    }
    data_file << "\n\t}\n}";
    data_file.close();

    for (pair<string, set<map<string, var_declaration>>> cs : csorts) {
        for (map<string, var_declaration> csm : cs.second) {
            for (pair<string, var_declaration> m : csm) {
                object_state obj = object_state();
                obj.parent = cs.first;           // type information
                obj.object_name = m.first;       // object name
                obj.attribute_states = m.second; // object attributes defined under parent type
                obj.attribute_types = var_declaration(); // object attribute types
                for (sort_definition sd : sort_definitions)
                    if (find(sd.declared_sorts.begin(), sd.declared_sorts.end(), obj.parent) !=
                        sd.declared_sorts.end())
                        for (arg_and_type s_types : sd.vars.vars)
                            for (arg_and_type s_states : obj.attribute_states.vars)
                                if (s_types.first.substr(1) == s_states.first)
                                    obj.attribute_types.vars.push_back(
                                      make_pair(s_states.first, s_types.second));

                initial_state.insert(make_pair(obj.object_name, obj));
            }
        }
    }

    // extract the railway network
    for (pair<string, object_state> states : initial_state) {
        if (states.second.parent == "map") {
            string vertices = "", edges = "";
            for (arg_and_type args : states.second.attribute_states.vars) {
                if (args.first == "vertices")
                    vertices = args.second.substr(1, args.second.size() - 2);
                else if (args.first == "edges")
                    edges = args.second.substr(1, args.second.size() - 2);
                else
                    cerr << "Do not identify attribute " << args.first << " for object type "
                         << states.second.parent << "\n";
            }
            rail_network = construct_network(vertices, edges);
            rail_network.id = states.first;
        }
    }

    map<string, fmethod> fm1 = { { "m_clear_and_move", &clear_and_move } };
    map<string, fmethod> fm2 = { { "m_reachable_location", &reachable_location } };
    map<string, fpredicate> fp = { { "clear", &clear } };
    assign_func_impl(fp, fm1);
    assign_func_impl(fm2);

    for (task t : primitive_tasks)
        if (t.name.find(method_precondition_action_name) == string::npos)
            globalMinDuration = min(globalMinDuration, (int)t.duration);

    map<int, double> time_values = map<int, double>();
    map<int, double> metric_values = map<int, double>();
    if (permute) {
        vector<request> requests_copy = requests;
        if (requests_copy.size() == 5) {
            int permute_counter = 0;
            do {
                auto start = chrono::steady_clock::now();
                stn.init();
                bool plan_found = false;
                Plan p = instantiate_plan();
                for (request r : requests_copy)
                    find_plan_for_request(r, &p, &plan_found, metric);
                auto end = chrono::steady_clock::now();
                auto diff = end - start;
                if (plan_found) {
                    validation_state state = plan_validator(&p);
                    if (state.status == validation_exception::NO_EXCEPTION) {
                        pair<double, double> metric_vals = compute_metrics(&p);
                        if (metric == "actions")
                            metric_values.insert(make_pair(permute_counter, metric_vals.first));
                        else
                            metric_values.insert(make_pair(permute_counter, metric_vals.second));
                        time_values.insert(
                          make_pair(permute_counter, chrono::duration<double>(diff).count()));
                    } else {

                        PLOGE << "Permute Counter = " << permute_counter << endl;
                        PLOGE << "PLAN WAS FOUND BUT VALIDATION FAILED!!\n";
                        switch (state.status) {
                            case validation_exception::EDGE_SKIP:
                            case validation_exception::VERTEX_SAME:
                            case validation_exception::MULTI_BLOCK:
                                PLOGE << "\t[REASON]: FAILED DUE TO RAIL BLOCK CONSTRAINTS!\n";
                                break;
                            default:
                                break;
                        }
                        switch (state.status) {
                            case validation_exception::EDGE_SKIP:
                            case validation_exception::VERTEX_SAME:
                            case validation_exception::PREC_FAIL:
                                PLOGE << "\t[REASON]: OFFENDING TOKEN -> "
                                      << state.failing_token.to_string() << endl;
                                break;
                            default:
                                break;
                        }
                        PLOGE << "The whole plan is - \n" << p.to_string() << endl;
                        stn.destroy();
                        /* assert(false); */
                    }
                }
                stn.destroy();
                permute_counter++;
            } while (next_permutation(requests_copy.begin(),
                                      requests_copy.end(),
                                      [](request a, request b) { return a.id < b.id; }));
        } else {
            int counter = 0, permute_counter = 0;
            do {
                auto start = chrono::steady_clock::now();
                stn.init();
                bool plan_found = false;
                Plan p = instantiate_plan();
                for (request r : requests_copy)
                    find_plan_for_request(r, &p, &plan_found, metric);
                auto end = chrono::steady_clock::now();
                auto diff = end - start;
                if (plan_found) {
                    validation_state state = plan_validator(&p);
                    if (state.status == validation_exception::NO_EXCEPTION) {
                        pair<double, double> metric_vals = compute_metrics(&p);
                        if (metric == "actions")
                            metric_values.insert(make_pair(permute_counter, metric_vals.first));
                        else
                            metric_values.insert(make_pair(permute_counter, metric_vals.second));
                        time_values.insert(
                          make_pair(permute_counter, chrono::duration<double>(diff).count()));
                        counter++;
                    } else {
                        PLOGE << "Counter = " << counter << endl;
                        PLOGE << "Permute Counter = " << permute_counter << endl;
                        PLOGE << "PLAN WAS FOUND BUT VALIDATION FAILED!!\n";
                        switch (state.status) {
                            case validation_exception::EDGE_SKIP:
                            case validation_exception::VERTEX_SAME:
                            case validation_exception::MULTI_BLOCK:
                                PLOGE << "\t[REASON]: FAILED DUE TO RAIL BLOCK CONSTRAINTS!\n";
                                break;
                            default:
                                break;
                        }
                        switch (state.status) {
                            case validation_exception::EDGE_SKIP:
                            case validation_exception::VERTEX_SAME:
                            case validation_exception::PREC_FAIL:
                                PLOGE << "\t[REASON]: OFFENDING TOKEN -> "
                                      << state.failing_token.to_string() << endl;
                                break;
                            default:
                                break;
                        }
                        PLOGE << "The whole plan is - \n" << p.to_string() << endl;
                        stn.destroy();
                        /* assert(false); */
                    }
                }
                stn.destroy();
                permute_counter++;
            } while (next_permutation(requests_copy.begin(),
                                      requests_copy.end(),
                                      [](request a, request b) { return a.id < b.id; }) &&
                     counter < MAX_PERMUTATIONS);
        }
    } else {
        auto start = chrono::steady_clock::now();
        stn.init();

        bool plan_found = false;
        Plan p = instantiate_plan();
        for (request r : requests) {
            find_plan_for_request(r, &p, &plan_found, metric);
        }
        auto end = chrono::steady_clock::now();
        auto diff = end - start;
        if (plan_found) {
            validation_state state = plan_validator(&p);
            if (state.status == validation_exception::NO_EXCEPTION) {
                PLOGV << p.to_string();
                pair<double, double> metric_values = compute_metrics(&p);
                cout << flush;
                cout << "Time(s): " << chrono::duration<double>(diff).count() << "\n";
                cout << "Total Actions: " << metric_values.first << "\n";
                cout << "Makespan: " << metric_values.second << "\n";
            } else {
                PLOGE << "PLAN WAS FOUND BUT VALIDATION FAILED!!\n";
                switch (state.status) {
                    case validation_exception::EDGE_SKIP:
                    case validation_exception::VERTEX_SAME:
                    case validation_exception::MULTI_BLOCK:
                        PLOGE << "\t[REASON]: FAILED DUE TO RAIL BLOCK CONSTRAINTS!\n";
                        break;
                    default:
                        break;
                }
                switch (state.status) {
                    case validation_exception::EDGE_SKIP:
                    case validation_exception::VERTEX_SAME:
                    case validation_exception::PREC_FAIL:
                        PLOGE << "\t[REASON]: OFFENDING TOKEN -> "
                              << state.failing_token.to_string() << endl;
                        break;
                    default:
                        break;
                }
                PLOGE << state.to_string() << endl;
                PLOGE << "The whole plan is - \n" << p.to_string() << endl;
                stn.destroy();
                /* assert(false); */
            }
        } else {
            PLOGV << "PLAN WAS NOT FOUND!!\n";
            stn.destroy();
            /* assert(false); */
        }
    }

    if (permute) {
        cout << "\t\tPERMUTATION BENCHMARKING RESULTS\n\n";
        cout << "Total permutations tested = " << std::to_string(time_values.size()) << endl;

        vector<request> time_requests = requests;
        vector<request> metric_requests = requests;
        map<int, double>::iterator t_it =
          min_element(time_values.begin(),
                      time_values.end(),
                      [](pair<int, double> a, pair<int, double> b) { return a.second < b.second; });
        int t_pos = t_it->first, counter = 0;
        double t_value = t_it->second;
        do {
            counter++;
        } while (next_permutation(time_requests.begin(),
                                  time_requests.end(),
                                  [](request a, request b) { return a.id < b.id; }) &&
                 counter < t_pos);
        cout << "Best Ordering based on Time(s) with value = " << std::to_string(t_value) << ": \n";
        for (request r : time_requests) {
            cout << r.id << " ";
        }
        cout << endl;

        map<int, double>::iterator m_it =
          min_element(metric_values.begin(),
                      metric_values.end(),
                      [](pair<int, double> a, pair<int, double> b) { return a.second < b.second; });
        counter = 0;
        int m_pos = m_it->first;
        double m_value = m_it->second;
        do {
            counter++;
        } while (next_permutation(metric_requests.begin(),
                                  metric_requests.end(),
                                  [](request a, request b) { return a.id < b.id; }) &&
                 counter < m_pos);
        cout << "Best Ordering based on metric ";
        if (metric == "actions")
            cout << "Total Actions";
        else
            cout << "Makespan";
        cout << " with value = " << std::to_string(m_value) << ": \n";
        for (request r : metric_requests) {
            cout << r.id << " ";
        }
        cout << endl;

        vector<double> times = vector<double>();
        vector<double> metrics = vector<double>();
        for (const pair<int, double> t : time_values)
            times.push_back(t.second);
        for (const pair<int, double> m : metric_values)
            metrics.push_back(m.second);

        // mean
        double mean_time = 0, mean_metric = 0;
        for (double val : times)
            mean_time += val;
        for (double val : metrics)
            mean_metric += val;
        mean_time /= (double)times.size();
        mean_metric /= (double)metrics.size();

        // std
        double std_time = 0, std_metric = 0;
        for (double val : times)
            std_time += pow(val - mean_time, 2);
        for (double val : metrics)
            std_metric += pow(val - mean_metric, 2);
        std_time /= (double)times.size();
        std_time = sqrt(std_time);
        std_metric /= (double)metrics.size();
        std_metric = sqrt(std_metric);

        cout << "\tBASIC METRICS\n";
        cout << "Time(s) : \n\tMean = " << std::to_string(mean_time)
             << "\n\tStd Dev = " << std::to_string(std_time) << "\n";
        if (metric == "actions")
            cout << "Total Actions :\n\tMean = ";
        else
            cout << "Makespan :\n\tMean = ";
        cout << std::to_string(mean_metric) << "\n\tStd Dev = " << std::to_string(std_metric)
             << "\n\n";
    }
}
