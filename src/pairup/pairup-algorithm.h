#ifndef PAIRUP_ALGORITHM_H
#define PAIRUP_ALGORITHM_H

#include "pairup-types.h"
#include "rw-csv.h"

/* Public Access API */
/* Note that the result should be freed by the caller */
/* TODO: Implement all the algorithms and choose the best one */
pair_result_t *
pairup (sheet *sheet);

relation_graph *
pairup_graph (sheet *worksheet);

/* Generate random seed based on the current time */
int
_get_random_seed (void);

char *
_get_time_slot (sheet *worksheet,
                int j);

static char *
_idx_to_name(const relation_graph *today, int id);

static void 
_generate_relations (sheet *sheet,
                     relation_graph *records,
                     member *mlist[]);

/* Member-availability-based algorithms for probing optimized results */
/* The member filled with the least/most time slots will be paired up first */
static pair_result_t *
_pairup_least_availability_priority (relation_graph *today,
                                     member *mlist[]);

static pair_result_t *
_pairup_most_availability_priority (relation_graph *today,
                                    member *mlist[]);

/* Sheet read-order-based algorithms for probing optimized results */
/* The member on the top/bottom of the sheet will be paired up first */
// static pair_result_t *
// _pairup_random_read_first (const relation_graph *today,
//                            int seed);

static pair_result_t *
_pairup_smallest_row_id_priority (relation_graph *today,
                                  member *mlist[]);

static pair_result_t *
_pairup_largest_row_id_priority (relation_graph *today,
                                 member *mlist[]);

/* Time-order-based algorithms for probing optimized results */
/* The member who has the earliest/latest time slot will be paired up first */
static pair_result_t *
_pairup_earliest_available_slot_priority (relation_graph *today,
                                          member *mlist[]);

static pair_result_t *
_pairup_latest_available_slot_priority (relation_graph *today,
                                        member *mlist[]);

/* Potential-partner-based algorithms for probing optimized results */
static pair_result_t *
_pairup_least_partner_priority (relation_graph *today,
                                member *members[]);
    
static pair_result_t *
_pairup_most_partner_priority (relation_graph *today,
                               member *members[]);

static pair_result_t *
_pairup_least_request_priority (relation_graph *today,
                                member *members[]);

static pair_result_t *
_pairup_most_request_priority (relation_graph *today,
                               member *members[]);


extern const algorithm pairup_algorithms[];

static void 
_generate_relations (sheet *worksheet,
                     relation_graph *today,
                     member *mlist[]);

static int
_generate_member_list (sheet *worksheet,
                 member *mlist[]);

void
print_graph (void *context);

#endif  // PAIRUP_ALGORITHM_H
