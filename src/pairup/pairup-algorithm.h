#ifndef PAIRUP_ALGORITHM_H
#define PAIRUP_ALGORITHM_H

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
_idx_to_name(const relation_graph_t *today, int id);

static void 
_generate_relations (sheet_t *sheet,
                     relation_graph_t *records,
                     member_t *member_list[]);

/* Member-availability-based algorithms for probing optimized results */
/* The member filled with the least/most time slots will be paired up first */
static pair_result_t *
_pairup_least_availability_first (relation_graph_t *today,
                                  member_t *member_list[]);

// static pair_result_t *
// _pairup_most_availability_first (relation_graph_t *today,
//                                  member_t *member_list[]);

/* Sheet read-order-based algorithms for probing optimized results */
/* The member on the top/bottom of the sheet will be paired up first */
// static pair_result_t *
// _pairup_random_read_first (const relation_graph_t *today,
//                            int seed);
//
// static pair_result_t *
// _pairup_first_read_first_serve (relation_graph_t *today,
//                                 member_t *member_list[]);
//
// static pair_result_t *
// _pairup_last_read_first_serve (relation_graph_t *today,
//                                member_t *member_list[]);

/* Time-order-based algorithms for probing optimized results */
/* The member who has the earliest/latest time slot will be paired up first */
// static pair_result_t *
// _pairup_earliest_time_first (relation_graph_t *today,
//                              member_t *member_list[]);
//
// static pair_result_t *
// _pairup_latest_time_first (relation_graph_t *today,
//                            member_t *member_list[]);

typedef pair_result_t *
(*pairup_fn) (relation_graph_t *today,
              member_t *member_list[]);

#define _MAX_PAIRUP_ALGORITHMS 1 
const pairup_fn pairup_algorithms[] = {
    // _pairup_random_read_first,
    _pairup_least_availability_first,
    // _pairup_most_availability_first,
    // _pairup_first_read_first_serve,
    // _pairup_last_read_first_serve,
    // _pairup_earliest_time_first,
    // _pairup_latest_time_first,
};

#endif  // PAIRUP_ALGORITHM_H
