#ifndef __HDDLWRITER

#define __HDDLWRITER

#include "parsetree.hpp"

void hddl_output(ostream &dout, ostream &pout, bool problem);
void print_indent(ostream &out, int indent, bool end = false);
void print_var_and_const(ostream &out, var_and_const &vars);
void print_formula(ostream &out, general_formula *f, int indent);
void print_formula_for(ostream &out, general_formula *f, string topic);

tuple<vector<string>, vector<int>, map<string, string>, map<int, int> >
compute_local_type_hierarchy();

bool replacement_type_dfs(int cur, int end, set<int> &visited,
                          vector<set<int> > &parents);

pair<int, set<int> > get_replacement_type(int type_to_replace,
                                          vector<set<int> > &parents);

#endif
