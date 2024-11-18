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

char *
_get_time_slot (sheet_t *worksheet,
                int j);

static char *
_idx_to_name(const relation_graph_t *today, int id);

static void 
_generate_relations (sheet_t *sheet,
                     relation_graph_t *records,
                     member_t *member_list[]);

/* Member-availability-based algorithms for probing optimized results */
/* The member filled with the least/most time slots will be paired up first */
static pair_result_t *
_pairup_least_availability_priority (relation_graph_t *today,
                                     member_t *member_list[]);

static pair_result_t *
_pairup_most_availability_priority (relation_graph_t *today,
                                    member_t *member_list[]);

/* Sheet read-order-based algorithms for probing optimized results */
/* The member on the top/bottom of the sheet will be paired up first */
// static pair_result_t *
// _pairup_random_read_first (const relation_graph_t *today,
//                            int seed);

static pair_result_t *
_pairup_smallest_row_id_priority (relation_graph_t *today,
                                  member_t *member_list[]);

static pair_result_t *
_pairup_largest_row_id_priority (relation_graph_t *today,
                                 member_t *member_list[]);

/* Time-order-based algorithms for probing optimized results */
/* The member who has the earliest/latest time slot will be paired up first */
static pair_result_t *
_pairup_earliest_available_slot_priority (relation_graph_t *today,
                                          member_t *member_list[]);

static pair_result_t *
_pairup_latest_available_slot_priority (relation_graph_t *today,
                                        member_t *member_list[]);

/* Potential-partner-based algorithms for probing optimized results */
static pair_result_t *
_pairup_least_potential_partner (relation_graph_t *today,
                                 member_t *members[]);
    
static pair_result_t *
_pairup_most_potential_partner (relation_graph_t *today,
                                member_t *members[]);

typedef pair_result_t *
(*pairup_fn) (relation_graph_t *today,
              member_t *member_list[]);

extern const pairup_fn pairup_algorithms[];

static void 
_generate_relations (sheet_t *worksheet,
                     relation_graph_t *today,
                     member_t *member_list[]);

static int
_generate_member_list (sheet_t *worksheet,
                       member_t *member_list[]);

#endif  // PAIRUP_ALGORITHM_H
