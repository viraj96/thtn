#include <getopt.h>

#include <map>
#include <cstdio>
#include <chrono>
#include <cassert>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iostream>

#define ATTEMPTS 1
#define MAX_PERMUTATIONS 10
/* #define USERDEBUG */

#include "parser.hpp"
#include "search.hpp"
#include "planner.hpp"
#include "timelines.hpp"

using namespace std;

STN stn = STN();
graph rail_network = graph();
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
map<string, set<string> > sorts = map<string, set<string> >();
map<string, set<string> > constants = map<string, set<string> >();
vector<sort_definition> sort_definitions = vector<sort_definition>();
vector<predicate_definition> predicate_definitions =
    vector<predicate_definition>();
map<string, vector<parsed_method> > parsed_methods =
    map<string, vector<parsed_method> >();
vector<pair<predicate_definition, string> > parsed_functions =
    vector<pair<predicate_definition, string> >();
map<string, set<map<string, var_declaration> > > csorts =
    map<string, set<map<string, var_declaration> > >();

pair<double, double> compute_metrics(Plan *p) {
    double total_tokens = 0;
    double makespan = -1.0 * std::numeric_limits<double>::infinity();
    vector<Timeline> tls = p->get_timelines();
    for (Timeline tl : tls) {
        if (tl.get_resource() == "robot") {
            vector<Token> tks = tl.get_tokens();
            total_tokens += (double)(tks.size()) - 2;
            Token last = tks.end()[-2];
            if (last.get_name() != "head" && last.get_name() != "tail") {
                tuple<double, double> last_end_bounds =
                    stn.get_feasible_values(last.get_end());
                if (abs(get<0>(last_end_bounds)) >= makespan)
                    makespan = abs(get<0>(last_end_bounds));
            }
        }
    }
    return make_pair(total_tokens, makespan);
}

void find_plan_for_request(request r, Plan *p, bool *plan_found,
                           string metric) {
    if (task_name_map.find(r.demand.name) == task_name_map.end()) {
        DEBUG("Request " << r.id << " cannot be met!\n");
    }

    search_tree r_tree = create_search_tree(r);
    DEBUG(r_tree.to_string() << endl);

    search_vertex root = r_tree.adj_list[0];
    vector<vector<string> > leafs = get_leafs(root, r_tree);

    *(plan_found) = false;
    for (vector<string> cand : leafs) {
        set<search_vertex> candidate = get_candidate(cand, r_tree);

        search_vertex root = search_vertex();
        for (search_vertex s : candidate) {
            if (s.parent == nullptr || (s.parent->or_node && s.parent->parent == nullptr))
                root = s;
        }

        pair<task_network, vector<arg_and_type> > ret =
            instantiate_task_network(&root, candidate, r);
        task_network r_net = ret.first;
        vector<arg_and_type> open = ret.second;

        vector<vector<string> > assignments = vector<vector<string> >();
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
            map<string, arg_and_type> all_assignments =
                find_assignments(assign[0], open);

            task_network test_net =
                assign_open_variables(all_assignments, r_net);

            // Try to schedule this network and find the first feasible plan
            pq sol = find_feasible_slots(test_net, *p, ATTEMPTS, metric);
            if ((int)sol.size() > 0) solutions.push(sol.top());
        }

        if (solutions.top().metric_value !=
            std::numeric_limits<double>::infinity()) {
            commit_slots(p, &solutions);
            *(plan_found) = true;
        }

        if (*(plan_found)) break;
    }
}

int main(int argc, char **argv) {
    int dfile = -1;
    int pfile = -1;

    string metric = "makespan";
    bool permute = false;
    bool showProperties = false;
    bool compileConditionalEffects = true;
    bool linearConditionalEffectExpansion = false;
    bool encodeDisjunctivePreconditionsInMethods = false;

    struct option options[] = {

        {"keep-conditional-effects", no_argument, NULL, 'k'},
        {"linear-conditional-effect", no_argument, NULL, 'L'},
        {"encode-disjunctive_preconditions-in-htn", no_argument, NULL, 'D'},
        {"properties", optional_argument, NULL, 'p'},
        {"metric", required_argument, NULL, 'm'},
        {"permute_benchmark", no_argument, NULL, 'b'},
        {NULL, 0, NULL, 0},

    };

    // Parsing command line options
    bool optionsValid = true;
    while (true) {
        int c = getopt_long_only(argc, argv, "kLDpm:b", options, NULL);
        if (c == -1) break;
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
        } else if (c == 'b') {
            permute = true;
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
        cout << "You need to provide a domain and problem file as input."
             << endl;
        return 1;
    }

    // Open c-style file handle
    FILE *domain_file = fopen(argv[dfile], "r");
    FILE *problem_file = fopen(argv[pfile], "r");

    if (!domain_file) {
        cout << "Can't open " << argv[dfile] << "!" << endl;
        return 2;
    }

    if (!problem_file) {
        cout << "Can't open " << argv[pfile] << "!" << endl;
        return 2;
    }

    /* cout << "Running the parser on the given domain and problem files!\n"; */
    run_parser(domain_file, problem_file, argv[dfile], argv[pfile],
               showProperties, compileConditionalEffects,
               linearConditionalEffectExpansion,
               encodeDisjunctivePreconditionsInMethods);

    for (pair<string, set<map<string, var_declaration>>> cs : csorts) {
        for (map<string, var_declaration> csm : cs.second) {
            for (pair<string, var_declaration> m : csm) {
                object_state obj = object_state();
                obj.parent = cs.first; // type information
                obj.object_name = m.first; // object name
                obj.attribute_states = m.second; // object attributes defined under parent type
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
                    cerr << "Do not identify attribute " << args.first << " for object type " << states.second.parent << "\n";
            }
            rail_network = construct_network(vertices, edges);
            rail_network.id = states.first;
        }
    }

    map<string, fmethod> fm1 = {{"m_clear_and_move", &clear_and_move}};
    map<string, fmethod> fm2 = {{"m_reachable_location", &reachable_location}};
    map<string, fpredicate> fp = {{"clear", &clear}};
    assign_func_impl(fp, fm1);
    assign_func_impl(fm2);

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
                    pair<double, double> metric_vals = compute_metrics(&p);
                    if (metric == "actions")
                        metric_values.insert(
                            make_pair(permute_counter, metric_vals.first));
                    else
                        metric_values.insert(
                            make_pair(permute_counter, metric_vals.second));
                    time_values.insert(
                        make_pair(permute_counter,
                                  chrono::duration<double>(diff).count()));
                }
                stn.destroy();
                permute_counter++;
            } while (next_permutation(
                requests_copy.begin(), requests_copy.end(),
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
                    pair<double, double> metric_vals = compute_metrics(&p);
                    if (metric == "actions")
                        metric_values.insert(
                            make_pair(permute_counter, metric_vals.first));
                    else
                        metric_values.insert(
                            make_pair(permute_counter, metric_vals.second));
                    time_values.insert(
                        make_pair(permute_counter,
                                  chrono::duration<double>(diff).count()));
                    counter++;
                }
                stn.destroy();
                permute_counter++;
            } while (next_permutation(
                         requests_copy.begin(), requests_copy.end(),
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
            DEBUG(p.to_string());
            pair<double, double> metric_values = compute_metrics(&p);
            cout << flush;
            cout << "Time(s): " << chrono::duration<double>(diff).count()
                 << "\n";
            cout << "Total Actions: " << metric_values.first << "\n";
            cout << "Makespan: " << metric_values.second << "\n";
        } else {
            DEBUG("PLAN WAS NOT FOUND!!\n");
            assert(false);
        }
    }

    if (permute) {
        cout << "\t\tPERMUTATION BENCHMARKING RESULTS\n\n";
        cout << "Total permutations tested = "
             << std::to_string(time_values.size()) << endl;

        vector<request> time_requests = requests;
        vector<request> metric_requests = requests;
        map<int, double>::iterator t_it =
            min_element(time_values.begin(), time_values.end(),
                        [](pair<int, double> a, pair<int, double> b) {
                            return a.second < b.second;
                        });
        int t_pos = t_it->first, counter = 0;
        double t_value = t_it->second;
        do {
            counter++;
        } while (next_permutation(
                     time_requests.begin(), time_requests.end(),
                     [](request a, request b) { return a.id < b.id; }) &&
                 counter < t_pos);
        cout << "Best Ordering based on Time(s) with value = "
             << std::to_string(t_value) << ": \n";
        for (request r : time_requests) {
            cout << r.id << " ";
        }
        cout << endl;

        map<int, double>::iterator m_it =
            min_element(metric_values.begin(), metric_values.end(),
                        [](pair<int, double> a, pair<int, double> b) {
                            return a.second < b.second;
                        });
        counter = 0;
        int m_pos = m_it->first;
        double m_value = m_it->second;
        do {
            counter++;
        } while (next_permutation(
                     metric_requests.begin(), metric_requests.end(),
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
        for (const pair<int, double> t : time_values) times.push_back(t.second);
        for (const pair<int, double> m : metric_values)
            metrics.push_back(m.second);

        // mean
        double mean_time = 0, mean_metric = 0;
        for (double val : times) mean_time += val;
        for (double val : metrics) mean_metric += val;
        mean_time /= (double)times.size();
        mean_metric /= (double)metrics.size();

        // std
        double std_time = 0, std_metric = 0;
        for (double val : times) std_time += pow(val - mean_time, 2);
        for (double val : metrics) std_metric += pow(val - mean_metric, 2);
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
        cout << std::to_string(mean_metric)
             << "\n\tStd Dev = " << std::to_string(std_metric) << "\n\n";
    }
}
