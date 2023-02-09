#include <getopt.h>

#include <map>
#include <cstdio>
#include <vector>
#include <cassert>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <iostream>

#include "cwa.hpp"
#include "domain.hpp"
#include "output.hpp"
#include "parsetree.hpp"
#include "properties.hpp"

#include "hddlWriter.hpp"

using namespace std;

// Declare parser function manually
void run_parser_on_file(FILE *f, char *filename);

// Parsed domain data structures
string domain_name = dummy_domain_name;
string problem_name = dummy_problem_name;
vector<method> methods = vector<method>();
string metric_target = dummy_function_type;
vector<task> abstract_tasks = vector<task>();
vector<request> requests = vector<request>();
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

int main(int argc, char **argv) {
    /* cin.sync_with_stdio(false); */
    /* cout.sync_with_stdio(false); */

    int dfile = -1;
    int pfile = -1;

    bool compileConditionalEffects = true;
    bool linearConditionalEffectExpansion = false;
    bool encodeDisjunctivePreconditionsInMethods = false;

    int verbosity = 6;
    bool hddlOutput = false;
    bool verboseOutput = false;

    bool showProperties = false;

    struct option options[] = {

        {"keep-conditional-effects", no_argument, NULL, 'k'},
        {"linear-conditional-effect", no_argument, NULL, 'L'},
        {"encode-disjunctive_preconditions-in-htn", no_argument, NULL, 'D'},
        {"hddl", no_argument, NULL, 'h'},

        {"debug", optional_argument, NULL, 'd'},
        {"properties", optional_argument, NULL, 'p'},

        {NULL, 0, NULL, 0},

    };

    // Parsing command line options
    bool optionsValid = true;
    while (true) {
        int c = getopt_long_only(argc, argv, "kLDhdp", options, NULL);
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
        else if (c == 'h')
            hddlOutput = true;
        else if (c == 'p')
            showProperties = true;
        else if (c == 'd') {
            verboseOutput = true;
            if (optarg) verbosity = atoi(optarg);
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

    // Parse the domain file and problem files
    run_parser_on_file(domain_file, argv[dfile]);
    cout << "Parser ran correctly on the domain file!\n";
    run_parser_on_file(problem_file, argv[pfile]);
    cout << "Parser ran correctly on the problem file!\n";

    // Add the equality check to predicates
    predicate_definition equalsPredicate = predicate_definition();
    equalsPredicate.functional = false;
    equalsPredicate.name = dummy_equal_literal;
    equalsPredicate.argument_sorts.push_back("object");
    equalsPredicate.argument_sorts.push_back("object");
    predicate_definitions.push_back(equalsPredicate);

    if (showProperties) {
        cout << "Printing properties of the domain and problem files!\n";
        printProperties();
    }

    // Add constants to all sorts
    cout << "Expanding the sorts!\n";
    expand_sorts();

    cout << "Compiling goals into actions!\n";
    compile_goal_into_action();

    // Flatten all primitive tasks
    cout << "Flattening all the primitive tasks!\n";
    flatten_tasks(compileConditionalEffects, linearConditionalEffectExpansion,
                  encodeDisjunctivePreconditionsInMethods);

    // Flatten the goal
    cout << "Flattening the goal!\n";
    flatten_goal();

    // Create appropriate methods and expand method preconditions
    cout << "Creating appropriate methods and expanding the method "
            "preconditions!\n";
    parsed_method_to_data_structures(compileConditionalEffects,
                                     linearConditionalEffectExpansion,
                                     encodeDisjunctivePreconditionsInMethods);

    // Closed World Assumption (CWA), but only if we actually want to compile
    // negative preconditions
    cout << "Computing the Closed World Assumption!\n";
    compute_cwa();

    // Simplify constraints as far as possible
    cout << "Simplifying constraints as far as possible!\n";
    reduce_constraints();

    cout << "Cleaning up the sorts!\n";
    clean_up_sorts();

    cout << "Removing unnecessary predicates!\n";
    remove_unnecessary_predicates();

    cout << "Compiling requests!\n";
    compile_requests();

    // Write to output
    cout << "Writing the output!\n";
    if (verboseOutput)
        verbose_output(verbosity);
    else if (hddlOutput) {
        ostream *dout = &cout;
        ostream *pout = &cout;
        hddl_output(*dout, *pout, false);
        hddl_output(*dout, *pout, true);

    } else {
        ostream *dout = &cout;
        simple_hddl_output(*dout);
    }

    for (auto m : methods) cout << m.to_string() << endl;
    for (auto pt : primitive_tasks) cout << pt.to_string() << endl;
    for (pair<string, set<string> > s : sorts) {
        cout << s.first << " - ";
        for (string ss : s.second) cout << ss << " ";
        cout << endl;
    }
    for (pair<string, set<map<string, var_declaration> > > cs : csorts) {
        cout << cs.first << " - ";
        for (map<string, var_declaration> s : cs.second)
            for (pair<string, var_declaration> ss : s) {
                cout << ss.first << " - " << ss.second.to_string() << "\n";
            }
        cout << endl;
    }
    for (sort_definition sd : sort_definitions) cout << sd.to_string() << endl;
}
