#ifndef __UTILS

#define __UTILS

#include <plog/Log.h>

struct validation_state; // forward declaration of this struct

#include "domain.hpp"
#include "timelines.hpp"

extern int globalMinDuration;
extern world_state initial_state;

enum validation_exception
{
    NO_EXCEPTION = 0, // Everything is correct
    EDGE_SKIP = 1,    // When the source - target edge is not in the network
    VERTEX_SAME = 2,  // When the source and target vertices are same
    PREC_FAIL = 3,    // When a precondition of a token fails against the current state of the world
    MULTI_BLOCK = 4,  // When a robot is occupying multiple rail blocks at a time
    UNKNOWN = 5       // Completely unknown error
};

struct validation_state
{

    Token failing_token;
    literal precondition;
    vertex source, sink;
    world_state current_state;
    validation_exception status;

    validation_state()
    {
        failing_token = Token();
        precondition = literal();
        source = vertex();
        sink = vertex();
        current_state = initial_state;
        status = validation_exception::NO_EXCEPTION;
    }

    validation_state(Token* failing_token,
                     literal precondition,
                     vertex source,
                     vertex sink,
                     world_state* current_state,
                     validation_exception status)
      : failing_token(*failing_token)
      , precondition(precondition)
      , source(source)
      , sink(sink)
      , current_state(*current_state)
      , status(status)
    {}

    string to_string() const;
};

double
compute_makespan(Plan* p);

double
compute_total_actions(Plan* p);

validation_state
plan_validator(Plan* p);

pair<double, double>
compute_metrics(Plan* p);

#endif
