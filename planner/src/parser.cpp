#include "parser.hpp"

void
run_parser(FILE* domain_file,
           FILE* problem_file,
           char* domain_name,
           char* problem_name,
           bool showProperties,
           bool compileConditionalEffects,
           bool linearConditionalEffectExpansion,
           bool encodeDisjunctivePreconditionsInMethods)
{
    // Parse the domain file and problem files
    // argv[dfile], argv[pfile]
    run_parser_on_file(domain_file, domain_name);
    run_parser_on_file(problem_file, problem_name);

    // Add the equality check to predicates
    predicate_definition equalsPredicate = predicate_definition();
    equalsPredicate.functional = false;
    equalsPredicate.name = dummy_equal_literal;
    equalsPredicate.argument_sorts.push_back("object");
    equalsPredicate.argument_sorts.push_back("object");
    predicate_definitions.push_back(equalsPredicate);

    if (showProperties)
        printProperties();

    // Add constants to all sorts
    expand_sorts();

    compile_goal_into_action();

    // Flatten all primitive tasks
    flatten_tasks(compileConditionalEffects,
                  linearConditionalEffectExpansion,
                  encodeDisjunctivePreconditionsInMethods);

    // Flatten the goal
    flatten_goal();

    // Create appropriate methods and expand method preconditions
    parsed_method_to_data_structures(compileConditionalEffects,
                                     linearConditionalEffectExpansion,
                                     encodeDisjunctivePreconditionsInMethods);

    // Closed World Assumption (CWA), but only if we actually want to compile
    // negative preconditions
    compute_cwa();

    // Simplify constraints as far as possible
    reduce_constraints();
    clean_up_sorts();
    remove_unnecessary_predicates();

    compile_requests();
}

void
update_literals(vector<literal>* l, string name, fpredicate f)
{
    for (auto it = (*l).begin(); it != (*l).end(); it++)
        if ((*it).functional && (*it).predicate == name)
            (*it).func_literal = f;
}

void
update_conditional_effects(vector<conditional_effect>* ceff, string name, fpredicate f)
{
    for (auto it = (*ceff).begin(); it != (*ceff).end(); it++) {
        if ((*it).effect.functional && (*it).effect.predicate == name)
            (*it).effect.func_literal = f;

        for (auto itce = (*it).condition.begin(); itce != (*it).condition.end(); itce++)
            if ((*itce).functional && (*itce).predicate == name)
                (*itce).func_literal = f;
    }
}

void
update_tasks(vector<task>* tasks, string name, fpredicate f)
{
    for (auto it = (*tasks).begin(); it != (*tasks).end(); it++) {
        update_literals(&(*it).eff, name, f);
        update_literals(&(*it).prec, name, f);
        update_literals(&(*it).constraints, name, f);
        update_literals(&(*it).costExpression, name, f);
        update_conditional_effects(&(*it).ceff, name, f);
    }
}

void
update_methods(vector<method>* methods,
               string predicate_name,
               string method_name,
               fpredicate fp,
               fmethod fm)
{
    for (auto it = (*methods).begin(); it != (*methods).end(); it++) {
        if ((*it).functional && (*it).name == method_name)
            (*it).func_method = fm;
        update_literals(&(*it).constraints, predicate_name, fp);
    }
}

void
update_methods(vector<method>* methods, string method_name, fmethod fm)
{
    for (auto it = (*methods).begin(); it != (*methods).end(); it++)
        if ((*it).functional && (*it).name == method_name)
            (*it).func_method = fm;
}

void
assign_func_impl(map<string, fpredicate> fpredicates, map<string, fmethod> fmethods)
{
    for (pair<string, fpredicate> fp : fpredicates) {
        update_tasks(&abstract_tasks, fp.first, fp.second);
        update_tasks(&primitive_tasks, fp.first, fp.second);
        for (pair<string, fmethod> fm : fmethods)
            update_methods(&methods, fp.first, fm.first, fp.second, fm.second);
    }
}

void
assign_func_impl(map<string, fmethod> fmethods)
{
    for (pair<string, fmethod> fm : fmethods)
        update_methods(&methods, fm.first, fm.second);
}
