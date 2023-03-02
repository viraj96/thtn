#ifndef __UTILS

#define __UTILS

#include <plog/Log.h>

#include "planner.hpp"
#include "timelines.hpp"

double
compute_makespan(Plan* p);

double
compute_total_actions(Plan* p);

pair<bool, Token>
plan_validator(Plan* p);

pair<double, double>
compute_metrics(Plan* p);

#endif
