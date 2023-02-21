#ifndef __PARSER

#define __PARSER

#include <cstring>
#include <fstream>
#include <plog/Log.h>
#include <boost/function.hpp>

#include "cwa.hpp"
#include "parsetree.hpp"
#include "properties.hpp"
#include "planner.hpp"

void run_parser_on_file(FILE *f, char *filename);
void assign_func_impl(map<string, fmethod> fmethods);
void update_tasks(vector<task> *tasks, string name, fpredicate f);
void update_literals(vector<literal> *l, string name, fpredicate f);
void update_methods(vector<method> *methods, string method_name, fmethod fm);

void update_conditional_effects(vector<conditional_effect> *ceff, string name,
                                fpredicate f);

void assign_func_impl(map<string, fpredicate> fpredicates,
                      map<string, fmethod> fmethods);

void update_methods(vector<method> *methods, string predicate_name,
                    string method_name, fpredicate fp, fmethod fm);

void run_parser(FILE *domain_file, FILE *problem_file, char *domain_name,
                char *problem_name, bool showProperties,
                bool compileConditionalEffects,
                bool linearConditionalEffectExpansion,
                bool encodeDisjunctivePreconditionsInMethods);

#endif
