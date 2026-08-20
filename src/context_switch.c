#include "context_switch.h"

void cs_init(ContextSwitchState *cs, int cost) {
    cs->last_run_pid = CS_NONE;
    cs->cost = cost;
}

void cs_mark_idle(ContextSwitchState *cs) {
    cs->last_run_pid = CS_NONE;
}

int cs_needs_switch(const ContextSwitchState *cs, int next_pid) {
    return cs->last_run_pid != next_pid;
}

void cs_set_running(ContextSwitchState *cs, int next_pid) {
    cs->last_run_pid = next_pid;
}

