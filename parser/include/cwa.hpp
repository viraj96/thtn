#ifndef __CWA

#define __CWA

#include <string>
#include <vector>

#include "parsetree.hpp"

using namespace std;

struct ground_literal {
    bool positive;
    timed_type timed;
    string predicate;
    vector<string> args;
    temporal_qualifier_type temp_qual;

    ground_literal() {
        timed = START;
        temp_qual = AT;
        predicate = "";
        positive = false;
        args = vector<string>();
    }

    string to_string() const;
};

bool operator<(const ground_literal &lhs, const ground_literal &rhs);

extern vector<ground_literal> init;
extern vector<ground_literal> goal;
extern general_formula *goal_formula;
extern vector<pair<ground_literal, int> > init_functions;

void compute_cwa();
void flatten_goal();
void fully_instantiate(int *inst, unsigned int pos, vector<string> &arg_sorts,
                       vector<vector<int> > &result);

#endif
