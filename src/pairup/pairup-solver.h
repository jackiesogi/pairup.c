#ifndef PAIRUP_SOLVER_H
#define PAIRUP_SOLVER_H

#include "pairup-types.h"

/*
 * Find a pairing that, in this order of priority:
 *   1. lets as many ensured members as possible get at least one partner,
 *   2. maximizes the number of pairs,
 *   3. prefers fixed time slots over the flex slot.
 *
 * Rules: a pair shares one slot both members filled in, a member joins at most
 * `requests` pairs, uses each slot at most once, and never meets the same
 * partner twice.
 *
 * The problem is copied into `result->problem` (unless it already lives there).
 * `time_limit` is in seconds (<= 0 means no limit). Returns 0 on success, or
 * -1 with a message in `err`.
 */
int
pairup_solve (const pairup_problem *problem,
              double time_limit,
              pairup_result *result,
              char *err,
              size_t err_size);

#endif  // PAIRUP_SOLVER_H
