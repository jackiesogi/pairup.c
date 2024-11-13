#ifndef PAIRUP_ALGORITHM_H
#define PAIRUP_ALGORITHM_H

#include <string.h>

#include "pairup-types.h"
#include "rw-csv.h"

/* Public Access API */
/* Note that the result should be freed by the caller */
/* TODO: Implement all the algorithms and choose the best one */
pair_result_t *
pairup (sheet_t *sheet);

/* Generate random seed based on the current time */
int
_get_random_seed (void);

static char *
_idx_to_name(const adjmatrix_t *graph, int idx);

static void 
_generate_matches_graph (sheet_t *sheet,
                         adjmatrix_t *records);

/* Member-availability-based algorithms for probing optimized results */
/* The member filled with the least/most time slots will be paired up first */
static pair_result_t *
_pairup_least_availability_first (const adjmatrix_t *graph);

static pair_result_t *
_pairup_most_availability_first (const adjmatrix_t *graph);

/* Sheet read-order-based algorithms for probing optimized results */
/* The member on the top/bottom of the sheet will be paired up first */
static pair_result_t *
_pairup_random_read_first (const adjmatrix_t *graph,
                           int seed);

static pair_result_t *
_pairup_first_read_first_serve (const adjmatrix_t *graph);

static pair_result_t *
_pairup_last_read_first_serve (const adjmatrix_t *graph);

/* Time-order-based algorithms for probing optimized results */
/* The member who has the earliest/latest time slot will be paired up first */
static pair_result_t *
_pairup_earliest_time_first (const adjmatrix_t *graph);

static pair_result_t *
_pairup_latest_time_first (const adjmatrix_t *graph);

#endif  // PAIRUP_ALGORITHM_H
