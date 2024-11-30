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
const pairup_algorithm a[] = {  // WHO WILL BE PAIRED UP FIRST?
    {
        "LEAST_AVAILABILITY_PRIORITY",            // Members with least time slots filled in.
         _pairup_least_availability_priority
    },
    {
        "MOST_AVAILABILITY_PRIORITY",             // Members with most time slots filled in.
        _pairup_most_availability_priority
    },
    {
        "SMALLEST_ROW_ID_PRIORITY",               // Members on the first row of the sheet.
        _pairup_smallest_row_id_priority
    },
    {
        "LARGEST_ROW_ID_PRIORITY",                // Members on the last row of the sheet.
        _pairup_largest_row_id_priority
    },
    {
        "EARLIEST_AVAILABLE_SLOT_PRIORITY",       // Members with the earliest time slot.
        _pairup_earliest_available_slot_priority
    },
    {
        "LEAST_REQUEST_PRIORITY",                 // Members with 'one' requests.
        _pairup_least_request_priority
    },
    {
        "LATEST_AVAILABLE_SLOT_PRIORITY",         // Members with the latest time slot.
        _pairup_latest_available_slot_priority
    },
    {
        "LEAST_PARTNER_PRIORITY",                 // Members with the least number of potential partners.
        _pairup_least_partner_priority
    },
    {
        "MOST_PARTNER_PRIORITY",                  // Members with the most number of potentail partners.
        _pairup_most_partner_priority
    },
    {
        "MOST_REQUEST_PRIORITY",                  // Members with 'two' requests.
        _pairup_most_request_priority
    },
};

/* Function aliases */
/* TODO: Rename the functions and remove the aliases */
static int (*_preprocess_fixed_memblist) (sheet *, member *[]) = _generate_member_list;
static void (*_preprocess_relation_graph) (sheet *, relation_graph *, member *[]) = _generate_relations;

/***************************  TOP LEVEL API (START)  ******************************/

/* The top-level pairup function */
/* This function will iterate through all the priority functions */
/* and choose the target result that has maximized matches */
pair_result *
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
    int id = -1;

    for (int i = 0; i < _MAX_PAIRUP_ALGORITHMS; i++)
    {
        pairup_internal algorithm = a[i].algorithm;

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
            id = i;
        }
        else
        {
            _free_pair_result(temp);
        }
    }

    debug_printf (DEBUG_SUMMARY, "Best Algorithm: %s", a[id].name);
    debug_action (DEBUG_SUMMARY, (callback)display_summary, (void*)best);
    debug_printf (DEBUG_SUMMARY, "Relation Graph:");
    debug_action (DEBUG_SUMMARY, (callback)display_graph, (void*)graph);

    _free_relation_graph (graph);

    return best;
}

graph *
pairup_graph (sheet *worksheet)
{
    relation_graph *graph = _new_relation_graph();
    member *member_list[_MAX_MEMBERS_LEN] = { NULL };

    /* Generate relations using the existing member_list */
    /* This will take in the empty member_list and fill it with the available members */
    _preprocess_fixed_memblist (worksheet, member_list);
    _preprocess_relation_graph (worksheet, graph, member_list);

    pairup_internal algorithm = a[0].algorithm;

    if (!algorithm) return NULL;

    /* Get the pairing result of current algorithm */
    algorithm(graph, member_list);

    // printf("Best algorithm: %d\n", best_id);
    return graph;
}
/***************************  TOP LEVEL API (END)  ******************************/

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
                       member *mlist[])
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

        mlist[i] = member;
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
    const relation *ra = *(const relation **)a;
    const relation *rb = *(const relation **)b;

    return (ra->candidates[0]->availability -
            rb->candidates[0]->availability);  // Tie-breaking by priority
}

static int
compare_availability_desc (const void *a,
                          const void *b)
{
    const relation *ra = *(const relation **)a;
    const relation *rb = *(const relation **)b;

    return (rb->candidates[0]->availability -
            ra->candidates[0]->availability);
}

static int
compare_row_count_asc (const void *a,
                       const void *b)
{
    const relation *ra = *(const relation **)a;
    const relation *rb = *(const relation **)b;

    return ra->count - rb->count;
}

static int
compare_row_count_desc (const void *a,
                        const void *b)
{
    const relation *ra = *(const relation **)a;
    const relation *rb = *(const relation **)b;

    return rb->count - ra->count;
}

static int
compare_id_asc (const void *a,
                const void *b)
{
    const relation *ra = *(const relation **)a;
    const relation *rb = *(const relation **)b;

    return (ra->candidates[0]->id - rb->candidates[0]->id);
}

static int
compare_id_desc (const void *a,
                 const void *b)
{
    const relation *ra = *(const relation **)a;
    const relation *rb = *(const relation **)b;

    return (rb->candidates[0]->id - ra->candidates[0]->id);
}

static int
compare_requests_asc (const void *a,
                      const void *b)
{
    const relation *ra = *(const relation **)a;
    const relation *rb = *(const relation **)b;

    return (ra->candidates[0]->requests - rb->candidates[0]->requests);
}

static int
compare_requests_desc (const void *a,
                       const void *b)
{
    const relation *ra = *(const relation **)a;
    const relation *rb = *(const relation **)b;

    return (rb->candidates[0]->requests - ra->candidates[0]->requests);
}

static int
compare_earliest_slot_asc (const void *a,
                           const void *b)
{
    const relation *ra = *(const relation **)a;
    const relation *rb = *(const relation **)b;

    return (ra->candidates[0]->earliest_slot -
            rb->candidates[0]->earliest_slot);
}

static int
compare_earliest_slot_desc (const void *a,
                            const void *b)
{
    const relation *ra = *(const relation **)a;
    const relation *rb = *(const relation **)b;

    return (rb->candidates[0]->earliest_slot -
            ra->candidates[0]->earliest_slot);
}

/* TODO: Record the modified time and implement this function */
static int
compare_first_modified_asc ()
{
    return 0;
}

static int
_find_member_id (graph *today,
                 member *member)
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
                     graph *today,
                     member *mlist[])
{
    int i, j, k;
    char cell[8];
    bool first = true;
    today->count = 0;

    /* Scan each row (members' name) */
    for (i = _FILED_ROW_START; i < worksheet->rows-1; i++)
    {
        first = true;
        relation *row = NULL;

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
                    row->candidates[0] = mlist[i];
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
                        row->candidates[row->count] = mlist[k];
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
    int i;
    for (i = 0; i < _MAX_MEMBERS_LEN; i++)
    {
        if (available_slot && available_slot[row][i] == value)
        {
            available_slot[row][i] = -1;
            // printf("Remove %d from row %d\n", value, row);
        }
    }
}

static int
_result_existed (pair_result *result,
                 member *a,
                 member *b)
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
_pairup_bfs (graph *today,
             member *members[],
             pair_result *result)
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
        /* If already paired, skip */
        if (remain[i] == 0)
        {
            continue;
        }

        relation *row = today->relations[i];
        member *a = row->candidates[0];  // himself/herself
        member *b = NULL;  // To be paired

        int j;
        /* Pair up the members */
        for (j = 1; j < row->count; j++)
        {
            int bi = _find_member_id(today, row->candidates[j]);

            if (remain[bi] <= 0 || 
                !_has_time_slot(available_slot, i, matched_slot[i][j]) ||
                !_has_time_slot(available_slot, bi, matched_slot[i][j]) ||
                !row->candidates[j])
            {
                continue;
            }

            b = row->candidates[j];

            if (_result_existed(result, b, a))
            {
                continue;
            }

            if (a && b)
            {
                remain[i]--;
                remain[bi]--;

                pair *pair = _new_pair();
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
static pair_result *
_pairup_with_priority (graph *today,
                       member *members[],
                       int (*compare_fn)(const void *, const void *))
{
    /* Initialize the result */
    pair_result *result = _new_pair_result(0, 0, 0);

    /* Sort the members based on the provided comparison function */
    qsort(today->relations, today->count, sizeof(relation *), compare_fn);

    debug_printf(DEBUG_INFO, "Sorted graph based on the priority:\n");
    debug_action(DEBUG_INFO, (callback)display_graph, (void*)today);

    /* Pair up the members */
    _pairup_bfs(today, members, result);

    debug_printf(DEBUG_INFO, "Pair result summary:\n");
    debug_action(DEBUG_INFO, (callback)display_summary, (void*)result);

    return result;
}

static pair_result *
_pairup_least_availability_priority (graph *graph,
                                     member *members[])
{
    return _pairup_with_priority(graph, members, compare_availability_asc);
}

static pair_result *
_pairup_most_availability_priority (graph *graph,
                                    member *members[])
{
    return _pairup_with_priority(graph, members, compare_availability_desc);
}

static pair_result *
_pairup_smallest_row_id_priority (graph *graph,
                                  member *members[])
{
    return _pairup_with_priority(graph, members, compare_id_asc);
}

static pair_result *
_pairup_largest_row_id_priority (graph *graph,
                                 member *members[])
{
    return _pairup_with_priority(graph, members, compare_id_desc);
}

static pair_result *
_pairup_earliest_available_slot_priority (graph *graph,
                                           member *members[])
{
    return _pairup_with_priority(graph, members, compare_earliest_slot_asc);
}

static pair_result *
_pairup_latest_available_slot_priority (graph *graph,
                                        member *members[])
{
    return _pairup_with_priority(graph, members, compare_earliest_slot_desc);
}

static pair_result *
_pairup_least_partner_priority (graph *graph,
                                member *members[])
{
    return _pairup_with_priority(graph, members, compare_row_count_asc);
}

static pair_result *
_pairup_most_partner_priority (graph *graph,
                               member *members[])
{
    return _pairup_with_priority(graph, members, compare_row_count_desc);
}

static pair_result *
_pairup_least_request_priority (graph *graph,
                                member *members[])
{
    return _pairup_with_priority(graph, members, compare_requests_asc);
}

static pair_result *
_pairup_most_request_priority (graph *graph,
                               member *members[])
{
    return _pairup_with_priority(graph, members, compare_requests_desc);
}
