#ifndef PAIRUP_PROBLEM_H
#define PAIRUP_PROBLEM_H

#include "pairup-types.h"
#include "rw-csv.h"

/*
 * Build a solver problem from the worksheet.
 *
 * Only members with at least one usable cell are kept. The flex column is
 * treated as one more time slot when `opt->enable_flex` is set, and ignored
 * otherwise. Returns 0 on success, or -1 with a message in `err`.
 */
int
pairup_problem_load (const sheet_t *worksheet,
                     const struct pairup_options *opt,
                     pairup_problem *problem,
                     char *err,
                     size_t err_size);

/*
 * Merge a member's consecutive regular slots into ranges such as "1900~2030".
 * The flex slot is not included. Returns the number of ranges written.
 */
int
pairup_member_ranges (const pairup_problem *problem,
                      int member,
                      char ranges[][PAIRUP_MAX_RANGE],
                      int max_ranges);

/* Index of the flex slot, or -1 if flex is disabled */
int
pairup_flex_slot (const pairup_problem *problem);

#endif  // PAIRUP_PROBLEM_H
