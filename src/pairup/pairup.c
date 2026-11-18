#include <stdio.h>
#include <stdlib.h>

#include "pairup.h"

pairup_result *
pairup_run (const sheet_t *worksheet,
            const struct pairup_options *opt,
            char *err,
            size_t err_size)
{
    struct pairup_options defaults;
    if (!opt)
    {
        pairup_options_init (&defaults);
        opt = &defaults;
    }

    pairup_result *result = calloc (1, sizeof(*result));
    if (!result)
    {
        snprintf (err, err_size, "out of memory");
        return NULL;
    }

    if (pairup_problem_load (worksheet, opt, &result->problem, err, err_size) != 0 ||
        pairup_solve (&result->problem, opt->time_limit, result, err, err_size) != 0)
    {
        free (result);
        return NULL;
    }

    debug_action (DEBUG_INFO, (callback) print_worksheet, (void *) worksheet);
    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Algorithm: ILP (HiGHS), %s\n",
                  result->status == PAIRUP_OPTIMAL ? "proven optimal" : "time limit reached");
    debug_action (DEBUG_SUMMARY, (callback) display_summary, (void *) result);
    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Relation Graph:\n");
    debug_action (DEBUG_SUMMARY, (callback) display_graph, (void *) &result->problem);

    return result;
}

void
pairup_result_free (pairup_result *result)
{
    free (result);
}
