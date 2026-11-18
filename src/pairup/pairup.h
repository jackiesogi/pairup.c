#ifndef PAIRUP_H
#define PAIRUP_H

#include "pairup-types.h"
#include "pairup-problem.h"
#include "pairup-solver.h"
#include "pairup-formatter.h"
#include "rw-csv.h"

/*
 * Load the worksheet and find the optimal pairing.
 * Returns a heap-allocated result (free with `pairup_result_free`), or NULL
 * with a message in `err`.
 */
pairup_result *
pairup_run (const sheet_t *worksheet,
            const struct pairup_options *opt,
            char *err,
            size_t err_size);

void
pairup_result_free (pairup_result *result);

#endif // PAIRUP_H
