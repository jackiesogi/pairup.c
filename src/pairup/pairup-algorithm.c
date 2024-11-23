#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pairup-algorithm.h"
#include "pairup-types.h"
#include "pairup-formatter.h"
#include "rw-csv.h"

/* Various pre-defined priority */
/* Technically, `pairup_algorithms[]` is an array with several function pointers */
/* pointing to corresponding priority functions */
#define _MAX_PAIRUP_ALGORITHMS 10
const pairup_fn pairup_algorithms[] = {        // WHO WILL BE PAIRED UP FIRST?
    _pairup_least_availability_priority,       // Members with least time slots filled in.
    _pairup_most_availability_priority,        // Members with most time slots filled in.
    _pairup_smallest_row_id_priority,          // Members on the first row of the sheet.
    _pairup_largest_row_id_priority,           // Members on the last row of the sheet.
    _pairup_earliest_available_slot_priority,  // Members filled in the earliest available time slot.
    _pairup_least_request_priority,            // Members with 'one' request.
    _pairup_latest_available_slot_priority,    // Members filled in the latest available time slot.
    _pairup_least_partner_priority,            // Members with least number of potential partners.
    _pairup_most_partner_priority,             // Members with most number of potential partners.
    _pairup_most_request_priority,             // Members with 'two' requests.
};

/* Member in ensure list are guaranteed to be paired up (if there is potential partners) */
// member_t * ensure_list[_MAX_MEMBERS_LEN] = { NULL };
// size_t ensure_count = 0;

/* Function aliases */
/* TODO: Rename the functions and remove the aliases */
static int (*_preprocess_fixed_memblist) (sheet *, member *[]) = _generate_member_list;
static void (*_preprocess_relation_graph) (sheet *, relation_graph *, member *[]) = _generate_relations;

/***************************  Public Access API  ******************************/

/* The top-level pairup function */
/* This function will iterate through all the priority functions */
/* and choose the target result that has maximized matches */
pair_result_t *
pairup (sheet *worksheet)
{
    relation_graph *graph = _new_relation_graph();
    member *member_list[_MAX_MEMBERS_LEN] = { NULL };

    /* Generate relations using the existing member_list */
    /* This will take in the empty member_list and fill it with the available members */
    _preprocess_fixed_memblist (worksheet, member_list);
    _preprocess_relation_graph (worksheet, graph, member_list);

    /* Initialize the best result and temporary result */
    result *best;
    result *temp;
    best = temp = NULL;
    int best_id = -1;

    for (int i = 0; i < _MAX_PAIRUP_ALGORITHMS; i++)
    {
        pairup_fn algorithm = pairup_algorithms[i];

        if (!algorithm) continue;

        /* Get the pairing result of current algorithm */
        temp = algorithm(graph, member_list);

        bool should_update_best = false;

        if (!best || temp->pairs > best->pairs)
        {
            should_update_best = true;
        }
        else if (temp->pairs == best->pairs)
        {
            int rand = _get_random_seed();
            if (rand % 2 == 0)
            {
                should_update_best = true;
            }
        }

        if (should_update_best)
        {
            if (best)
            {
                _free_pair_result(best);
            }
            best = temp;
            best_id = i;
        }
        else
        {
            _free_pair_result(temp);
        }
    }

    _free_relation_graph (graph);
    // printf("Best algorithm: %d\n", best_id);
    return best;
}
/***************************  Public Access API  ******************************/

/* Generate a random integer, ensuring srand is initialized only once */
int
_get_random_seed (void)
{
    static int initialized = 0; // Static variable to track initialization

    if (!initialized)
    {
        srand((unsigned int)time(NULL));
        initialized = 1; // Mark as initialized
    }

    return rand();
}

static int
_get_member_availability (sheet *worksheet,
                          int id)
{
    int j, count = 0;
    char cell[8];

    for (j = _FILED_COL_START; j <= _FILED_COL_END; j++)
    {
        get_cell (worksheet, id, j, cell, sizeof(cell));
        if (is_available(cell))
        {
            count++;
        }
    }

    return count;
}

static int
_get_member_earliest_slot (sheet *worksheet,
                           int id)
{
    int j;
    char cell[8];

    for (j = _FILED_COL_START; j <= _FILED_COL_END; j++)
    {
        get_cell (worksheet, id, j, cell, sizeof(cell));
        if (is_available(cell))
        {
            return j;
        }
    }

    return -1;
}

static int
_get_member_requests (sheet *worksheet,
                      int id)
{
    int j;
    char cell[8];

    for (j = _FILED_COL_START; j <= _FILED_COL_END; j++)
    {
        get_cell (worksheet, id, j, cell, sizeof(cell));
        if (is_once(cell))
        {
            return 1;
        }
        if (is_twice(cell))
        {
            return 2;
        }
    }

    return 0;
}

static char *
_get_member_name (sheet *worksheet,
                  int id)
{
    return worksheet->data[id][_FILED_COL_NAME];
}

static int
_generate_member_list (sheet *worksheet,
                       member *member_list[])
{
    int i, count = 0;

    for (i = _FILED_ROW_START; i < worksheet->rows; i++)
    {
        member *member = _new_member();
        member->id = i;
        member->requests = _get_member_requests(worksheet, i);
        member->availability = _get_member_availability(worksheet, i);
        member->earliest_slot = _get_member_earliest_slot(worksheet, i);

        char *name = _get_member_name (worksheet, i);
        size_t lastchar = strnlen (name, MAX_NAME_LEN);
        strncpy(member->name, name, lastchar);
        member->name[lastchar] = '\0';

        member_list[i] = member;
        count++;
    }

    return count;
}

char *
_get_time_slot (sheet *worksheet,
               int j)
{
    return worksheet->data[0][j];
}

/************************************************************************************/

/* Compare functions for qsort */
static int
compare_availability_asc (const void *a,
                          const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return (ra->candidates[0]->availability -
            rb->candidates[0]->availability);  // Tie-breaking by priority
}

static int
compare_availability_desc (const void *a,
                          const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return (rb->candidates[0]->availability -
            ra->candidates[0]->availability);
}

static int
compare_row_count_asc (const void *a,
                       const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return ra->count - rb->count;
}

static int
compare_row_count_desc (const void *a,
                        const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return rb->count - ra->count;
}

static int
compare_id_asc (const void *a,
                const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return (ra->candidates[0]->id - rb->candidates[0]->id);
}

static int
compare_id_desc (const void *a,
                 const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return (rb->candidates[0]->id - ra->candidates[0]->id);
}

static int
compare_requests_asc (const void *a,
                      const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return (ra->candidates[0]->requests - rb->candidates[0]->requests);
}

static int
compare_requests_desc (const void *a,
                       const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return (rb->candidates[0]->requests - ra->candidates[0]->requests);
}

static int
compare_earliest_slot_asc (const void *a,
                           const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return (ra->candidates[0]->earliest_slot -
            rb->candidates[0]->earliest_slot);
}

static int
compare_earliest_slot_desc (const void *a,
                            const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return (rb->candidates[0]->earliest_slot -
            ra->candidates[0]->earliest_slot);
}

// static int
// compare_ensure_list_asc (const void *a,
//                          const void *b)
// {
//     /* A is in ensure list but b isn't */
//     for (int i = 0; i < ensure_count; i++)
//     {
//         if (ensure_list[i] == *(member_t **)a)
//         {
//             return -1;
//         }
//         if (ensure_list[i] == *(member_t **)b)
//         {
//             return 1;
//         }
//     }
//     return 0;
// }

/* TODO: Record the modified time and implement this function */
static int
compare_first_modified_asc ()
{
    return 0;
}

/* Debug purpose */
void
print_graph (relation_graph_t *graph)
{
    for (int i = 0; i < graph->count; i++)
    {
        relation_t *row = graph->relations[i];
        printf("Relation %d: [ %s ] --> ", i, row->candidates[0]->name);
        for (int j = 1; j < row->count; j++)
        {
            int time = row->matched_slot[j];
            printf("%s(at %d) --> ", row->candidates[j]->name, time);
        }
        printf("\n");
    }
    printf("\n");
}

static int
_find_member_id (relation_graph_t *today,
                 member_t *member)
{
    for (int i = 0; i < today->count; i++)
    {
        if (today->relations[i]->candidates[0]->id == member->id)
        {
            return i;
        }
    }

    return -1;
}

#define DEFAULT_PRIORITY 0

static void 
_generate_relations (sheet *worksheet,
                     relation_graph_t *today,
                     member_t *member_list[])
{
    int i, j, k;
    char cell[8];
    bool first = true;
    today->count = 0;

    /* Scan each row (members' name) */
    for (i = _FILED_ROW_START; i < worksheet->rows-1; i++)
    {
        first = true;
        relation_t *row = NULL;

        /* Scan each column (time slots) */
        for (j = _FILED_COL_START; j <= _FILED_COL_END; j++)
        {
            get_cell(worksheet, i, j, cell, sizeof(cell));

            if (is_available(cell))
            {
                /* If this is the first available slot for this row, allocate a new member */
                if (first)
                {
                    /* Allocate memory for this member and initialize */
                    row = _new_relation();
                    row->availability = 0;
                    row->matched_slot[0] = -1;  // no one will be paired with himself/herself
                    row->candidates[0] = member_list[i];
                    row->count = 1;

                    today->relations[today->count] = row;
                    today->count++;

                    first = false;
                }
                row->available_slot[row->availability] = j;
                row->availability++;

                /* Search in the same column to find matching count */
                for (k = _FILED_ROW_START; k < worksheet->rows-1; k++)
                {
                    get_cell(worksheet, k, j, cell, sizeof(cell));

                    if (is_available(cell) && k != i)
                    {
                        if (row->count >= _MAX_MATCHES_LEN - 1)
                        {
                            break;
                        }

                        row->matched_slot[row->count] = j;
                        row->candidates[row->count] = member_list[k];
                        row->count++;
                    }
                }
            }
        }
    }
}

static int 
_has_time_slot (int available_slot[_MAX_MEMBERS_LEN][_MAX_MEMBERS_LEN],
                int row,
                int value)
{
    // printf("Checking row %d for time slot %d\n", row, value);
    for (int i = 0; i < _MAX_MEMBERS_LEN; i++)
    {
        if (available_slot[row][i] == value)
        {
            return 1;
        }
    }

    return 0;
}

static void
_remove_from_available_slot (int available_slot[_MAX_MEMBERS_LEN][_MAX_MEMBERS_LEN],
                             int row,
                             int value)
{
    for (int i = 0; i < _MAX_MEMBERS_LEN; i++)
    {
        if (available_slot[row][i] == value)
        {
            available_slot[row][i] = -1;
            // printf("Remove %d from row %d\n", value, row);
        }
    }
}

static int
_result_existed (pair_result_t *result,
                member_t *a,
                member_t *b)
{
    for (int i = 0; i < result->pairs; i++)
    {
        if (result->pair_list[i]->a == a &&
            result->pair_list[i]->b == b)
        {
            return 1;
        }
    }

    return 0;
}

static void 
_pairup_bfs (relation_graph_t *today,
             member_t *members[],
             pair_result_t *result)
{
    /* Array that records the remaining time requested by each member */
    int remain[_MAX_MEMBERS_LEN] = { 0 };
    int total_requests = 0;
    // printf("Original request of each member:\n");
    for (int i = 0; i < today->count; i++)
    {
        int req = today->relations[i]->candidates[0]->requests;
        total_requests += req;
        remain[i] = req;
        // printf("Member %s has %d requests\n",
                // today->relations[i]->candidates[0]->name,
                // remain[i]);
    }

    int available_slot[_MAX_MATCHES_LEN][_MAX_MATCHES_LEN];
    int matched_slot[_MAX_MATCHES_LEN][_MAX_MATCHES_LEN];

    for (int i = 0; i < today->count; i++)
    {
        int rsize = today->relations[i]->count;
        int asize = today->relations[i]->availability;
        for (int j = 0; j < rsize; j++)
        {
            matched_slot[i][j] = today->relations[i]->matched_slot[j];
        }
        for (int j = 0; j < asize; j++)
        {
            available_slot[i][j] = today->relations[i]->available_slot[j];
        }
    }

    /* Use BFS to pair up the members, starting from the first row */
    for (int i = 0; i < today->count; i++)
    {
        /* print the available_slot */
        // for (int i = 0; i < today->count; i++)
        // {
        //     printf("%d ", available_slot[0][0]);
        //     for (int j = 0; j < today->relations[i]->availability; j++)
        //     {
        //         printf("%d ", available_slot[i][j]);
        //     }
        //     printf("\n");
        // }

        // printf("Processing row %d: Member '%s'\n", i, today->relations[i]->candidates[0]->name);
        /* If already paired, skip */
        if (remain[i] == 0)
        {
            continue;
        }

        relation_t *row = today->relations[i];
        member_t *a = row->candidates[0];  // himself/herself
        member_t *b = NULL;  // To be paired

        int j;
        /* Pair up the members */
        for (j = 1; j < row->count; j++)
        {
            int bi = _find_member_id(today, row->candidates[j]);
            // printf("    Found member %s with id %d at column %d in this row\n", today->relations[i]->candidates[j]->name, bi, j);
            // printf("    The two member is trying the time slot at %d\n", row->matched_slot[j]);
            // printf("    The two member is trying the time slot at %d\n", matched_slot[i][j]);

            // int rbi = -1;
            // /* Find the row index of the b index */
            // for (int k = 0; k < today->count; k++)
            // {
            //     if (today->relations[k]->candidates[0]->id == bi)
            //     {
            //         rbi = k;
            //         printf("    Found member %s at row %d\n", today->relations[k]->candidates[0]->name, rbi);
            //         break;
            //     }
            // }

            // printf("    Try pairing with member %s ...... ", today->relations[i]->candidates[j]->name);
            if (remain[bi] <= 0 || 
                !_has_time_slot(available_slot, i, matched_slot[i][j]) ||
                !_has_time_slot(available_slot, bi, matched_slot[i][j]) ||
                !row->candidates[j])
            {
                // printf("Failed!\n");
                // if (remain[bi] <= 0)
                // {
                //     printf("    Member %s has no remaining requests\n", today->relations[i]->candidates[j]->name);
                // }
                // if (_has_time_slot(available_slot, bi, available_slot[i][j]))
                // {
                //     printf("    Member %s has already been paired with someone else at this time\n", today->relations[i]->candidates[j]->name);
                //     for (int k = 0; k < today->count; k++)
                //     {
                //         printf("    ");
                //         for (int l = 0; l < today->relations[k]->availability; l++)
                //         {
                //             printf("%d ", available_slot[k][l]);
                //         }
                //         printf("\n");
                //     }
                // }
                // if (!row->candidates[j])
                // {
                //     printf("    Member %s address is NULL\n", today->relations[i]->candidates[j]->name);
                // }
                continue;
            }

            // printf("Success!\n");
            b = row->candidates[j];

            if (_result_existed(result, b, a))
            {
                continue;
            }

            if (a && b)
            {
                remain[i]--;
                remain[bi]--;

                pair_t *pair = _new_pair();
                pair->a = a;
                pair->b = b;
                pair->time = row->matched_slot[j];
                result->pair_list[result->pairs] = pair;
                result->pairs++;

                _remove_from_available_slot(available_slot, i, matched_slot[i][j]);
                _remove_from_available_slot(available_slot, bi, matched_slot[i][j]);
            }

            result->total_requests = total_requests;
            break;
        }
    }

    /* Record the remaining singles */
    for (int i = 0; i < today->count; i++)
    {
        if (remain[i] != 0)
        {
            result->single_list[result->singles++] = today->relations[i]->candidates[0];
        }
    }

    /* Record the member list */
    for (int i = 0; i < today->count; i++)
    {
        result->member_list[result->member++] = today->relations[i]->candidates[0];
    }
}

/* TODO: Sort not only rows but also elements in that row */
static pair_result_t *
_pairup_least_availability_priority (relation_graph_t *today,
                                     member_t *members[])
{
    /* Initialize the result */
    pair_result_t *result = _new_pair_result(0, 0, 0);

    /* Sort the members based on the availability */
    qsort(today->relations, today->count, sizeof(relation_t *), compare_availability_asc);

    log_message (DEBUG_INFO, (debug_fn)print_graph, (void*)today);

    /* Pair up the members */
    _pairup_bfs (today, members, result);

    log_message (DEBUG_SUMMARY, (debug_fn)print_result_statistics, (void*)result);

    return result;
}

static pair_result_t *
_pairup_most_availability_priority (relation_graph_t *today,
                                    member_t *members[])
{
    /* Initialize the result */
    pair_result_t *result = _new_pair_result(0, 0, 0);

    /* Sort the members based on the availability */
    qsort(today->relations, today->count, sizeof(relation_t *), compare_availability_desc);

    log_message (DEBUG_INFO, (debug_fn)print_graph, (void*)today);

    /* Pair up the members */
    _pairup_bfs (today, members, result);

    log_message (DEBUG_SUMMARY, (debug_fn)print_result_statistics, (void*)result);

    return result;
}

static pair_result_t *
_pairup_smallest_row_id_priority (relation_graph_t *today,
                                  member_t *members[])
{
    /* Initialize the result */
    pair_result_t *result = _new_pair_result(0, 0, 0);

    /* Sort the members based on the availability */
    qsort(today->relations, today->count, sizeof(relation_t *), compare_id_asc);

    log_message (DEBUG_INFO, (debug_fn)print_graph, (void*)today);

    /* Pair up the members */
    _pairup_bfs (today, members, result);

    log_message (DEBUG_SUMMARY, (debug_fn)print_result_statistics, (void*)result);

    return result;
}

static pair_result_t *
_pairup_largest_row_id_priority (relation_graph_t *today,
                                 member_t *members[])
{
    /* Initialize the result */
    pair_result_t *result = _new_pair_result(0, 0, 0);

    /* Sort the members based on the availability */
    qsort(today->relations, today->count, sizeof(relation_t *), compare_id_desc);

    log_message (DEBUG_INFO, (debug_fn)print_graph, (void*)today);

    /* Pair up the members */
    _pairup_bfs (today, members, result);

    log_message (DEBUG_SUMMARY, (debug_fn)print_result_statistics, (void*)result);

    return result;
}

static pair_result_t *
_pairup_earliest_available_slot_priority (relation_graph_t *today,
                                           member_t *members[])
{
    /* Initialize the result */
    pair_result_t *result = _new_pair_result(0, 0, 0);

    /* Sort the members based on the availability */
    qsort(today->relations, today->count, sizeof(relation_t *), compare_earliest_slot_asc);

    log_message (DEBUG_INFO, (debug_fn)print_graph, (void*)today);

    /* Pair up the members */
    _pairup_bfs (today, members, result);

    log_message (DEBUG_SUMMARY, (debug_fn)print_result_statistics, (void*)result);

    return result;
}

static pair_result_t *
_pairup_latest_available_slot_priority (relation_graph_t *today,
                                        member_t *members[])
{
    /* Initialize the result */
    pair_result_t *result = _new_pair_result(0, 0, 0);

    /* Sort the members based on the availability */
    qsort(today->relations, today->count, sizeof(relation_t *), compare_earliest_slot_desc);

    log_message (DEBUG_INFO, (debug_fn)print_graph, (void*)today);

    /* Pair up the members */
    _pairup_bfs (today, members, result);

    log_message (DEBUG_SUMMARY, (debug_fn)print_result_statistics, (void*)result);

    return result;
}

static pair_result_t *
_pairup_least_partner_priority (relation_graph_t *today,
                                member_t *members[])
{
    /* Initialize the result */
    pair_result_t *result = _new_pair_result(0, 0, 0);

    /* Sort the members based on the availability */
    qsort(today->relations, today->count, sizeof(relation_t *), compare_row_count_asc);

    log_message (DEBUG_INFO, (debug_fn)print_graph, (void*)today);

    /* Pair up the members */
    _pairup_bfs (today, members, result);

    log_message (DEBUG_SUMMARY, (debug_fn)print_result_statistics, (void*)result);

    return result;
}

static pair_result_t *
_pairup_most_partner_priority (relation_graph_t *today,
                               member_t *members[])
{
    /* Initialize the result */
    pair_result_t *result = _new_pair_result(0, 0, 0);

    /* Sort the members based on the availability */
    qsort(today->relations, today->count, sizeof(relation_t *), compare_row_count_desc);

    log_message (DEBUG_INFO, (debug_fn)print_graph, (void*)today);

    /* Pair up the members */
    _pairup_bfs (today, members, result);

    return result;
}

static pair_result_t *
_pairup_least_request_priority (relation_graph_t *today,
                                member_t *members[])
{
    /* Initialize the result */
    pair_result_t *result = _new_pair_result(0, 0, 0);

    /* Sort the members based on the availability */
    qsort(today->relations, today->count, sizeof(relation_t *), compare_requests_asc);

    log_message (DEBUG_INFO, (debug_fn)print_graph, (void*)today);

    /* Pair up the members */
    _pairup_bfs (today, members, result);

    return result;
}

static pair_result_t *
_pairup_most_request_priority (relation_graph_t *today,
                               member_t *members[])
{
    /* Initialize the result */
    pair_result_t *result = _new_pair_result(0, 0, 0);

    /* Sort the members based on the availability */
    qsort(today->relations, today->count, sizeof(relation_t *), compare_requests_desc);

    log_message (DEBUG_INFO, (debug_fn)print_graph, (void*)today);

    /* Pair up the members */
    _pairup_bfs (today, members, result);

    return result;
}
