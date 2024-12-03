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

/* Member-availability-based algorithms for probing optimized results */
/* The member filled with the least/most time slots will be paired up first */
pair_result_t *
pairup_least_availability_priority (relation_graph *today,
                                    member *mlist[]);

pair_result_t *
pairup_most_availability_priority (relation_graph *today,
                                    member *mlist[]);

/* Sheet read-order-based algorithms for probing optimized results */
/* The member on the top/bottom of the sheet will be paired up first */
pair_result_t *
pairup_smallest_row_id_priority (relation_graph *today,
                                 member *mlist[]);

pair_result_t *
pairup_largest_row_id_priority (relation_graph *today,
                                member *mlist[]);

/* Time-order-based algorithms for probing optimized results */
/* The member who has the earliest/latest time slot will be paired up first */
pair_result_t *
pairup_earliest_available_slot_priority (relation_graph *today,
                                         member *mlist[]);

pair_result_t *
pairup_latest_available_slot_priority (relation_graph *today,
                                       member *mlist[]);

/* Potential-partner-based algorithms for probing optimized results */
pair_result_t *
pairup_least_partner_priority (relation_graph *today,
                               member *members[]);

pair_result_t *
pairup_most_partner_priority (relation_graph *today,
                              member *members[]);

pair_result_t *
pairup_least_request_priority (relation_graph *today,
                               member *members[]);

pair_result_t *
pairup_most_request_priority (relation_graph *today,
                              member *members[]);

#endif  // PAIRUP_ALGORITHM_H
