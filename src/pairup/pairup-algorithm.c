#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pairup-algorithm.h"
#include "pairup-types.h"
#include "pairup-formatter.h"
#include "rw-csv.h"

/* Pairup algorithm (internal) */
typedef pair_result *
(*pairup_internal) (relation_graph *today,
                    member_t *member_list[]);

typedef struct pairup_algorithm pairup_algorithm_t;
typedef struct pairup_algorithm pairup_algorithm;  // Recommended
typedef struct pairup_algorithm algorithm_t;
typedef struct pairup_algorithm algorithm;

struct pairup_algorithm
{
    const char *name;
    const pairup_internal algorithm;
};

/* Various pre-defined priority */
/* Technically, `pairup_algorithms[]` is an array with several function pointers */
/* pointing to corresponding priority functions */
#define MAX_PAIRUP_ALGORITHMS 10
const pairup_algorithm a[] = {                    // WHO WILL BE PAIRED UP FIRST?
    {
        "LEAST_AVAILABILITY_PRIORITY",            // Members with least time slots filled in.
         pairup_least_availability_priority
    },
    {
        "MOST_AVAILABILITY_PRIORITY",             // Members with most time slots filled in.
        pairup_most_availability_priority
    },
    {
        "SMALLEST_ROW_ID_PRIORITY",               // Members on the first row of the sheet.
        pairup_smallest_row_id_priority
    },
    {
        "LARGEST_ROW_ID_PRIORITY",                // Members on the last row of the sheet.
        pairup_largest_row_id_priority
    },
    {
        "EARLIEST_AVAILABLE_SLOT_PRIORITY",       // Members with the earliest time slot.
        pairup_earliest_available_slot_priority
    },
    {
        "LEAST_REQUEST_PRIORITY",                 // Members with 'one' requests.
        pairup_least_request_priority
    },
    {
        "LATEST_AVAILABLE_SLOT_PRIORITY",         // Members with the latest time slot.
        pairup_latest_available_slot_priority
    },
    {
        "LEAST_PARTNER_PRIORITY",                 // Members with the least number of potential partners.
        pairup_least_partner_priority
    },
    {
        "MOST_PARTNER_PRIORITY",                  // Members with the most number of potentail partners.
        pairup_most_partner_priority
    },
    {
        "MOST_REQUEST_PRIORITY",                  // Members with 'two' requests.
        pairup_most_request_priority
    },
};

/********************  INTERNAL FUNCTIONS DECLARATION (START)  *********************/
/* Generate a random integer, only for internal use */
static int
preprocess_fixed_memblist (sheet *worksheet,
                           member *mlist[]);

static void
preprocess_relation_graph (sheet *worksheet,
                           graph *today,
                           member *mlist[]);

static int
get_random_seed (void);

static int
get_member_availability (sheet *worksheet,
                         int id);

static int
get_member_earliest_slot (sheet *worksheet,
                          int id);

static int
get_member_requests (sheet *worksheet,
                     int id);

static char *
get_member_name (sheet *worksheet,
                 int id);

static int
compare_availability_asc (const void *a,
                          const void *b);

static int
compare_availability_desc (const void *a,
                           const void *b);

static int
compare_row_count_asc (const void *a,
                       const void *b);

static int
compare_row_count_desc (const void *a,
                        const void *b);

static int
compare_id_asc (const void *a,
                const void *b);

static int
compare_id_desc (const void *a,
                 const void *b);

static int
compare_requests_asc (const void *a,
                      const void *b);

static int
compare_requests_desc (const void *a,
                       const void *b);

static int
compare_earliest_slot_asc (const void *a,
                           const void *b);

static int
compare_earliest_slot_desc (const void *a,
                            const void *b);

static int
find_member_id (graph *today,
                member *member);

static int
has_time_slot (int available_slot[MAX_MEMBERS_LEN][MAX_MEMBERS_LEN],
               int row,
               int value);

static void
remove_from_available_slot (int available_slot[MAX_MEMBERS_LEN][MAX_MEMBERS_LEN],
                            int row,
                            int value);

static int
result_existed (pair_result *result,
                member *a,
                member *b);

static void
pairup_bfs (graph *today,
            member *members[],
            pair_result *result);

static pair_result *
pairup_with_priority (graph *today,
                      member *members[],
                      int (*compare_fn)(const void *, const void *));

/*******************  INTERNAL FUNCTIONS DECLARATION (END)  ***********************/

/***************************  TOP LEVEL API (START)  ******************************/

/* The top-level pairup function */
/* This function will iterate through all the priority functions */
/* and choose the target result that has maximized matches */
pair_result *
pairup (sheet *worksheet)
{
    relation_graph *graph = new_relation_graph();
    member *member_list[MAX_MEMBERS_LEN] = { NULL };

    /* Generate relations using the existing member_list */
    /* This will take in the empty member_list and fill it with the available members */
    preprocess_fixed_memblist (worksheet, member_list);
    preprocess_relation_graph (worksheet, graph, member_list);

    /* Initialize the best result and temporary result */
    result *best;
    result *temp;
    best = temp = NULL;
    int id = -1;

    for (int i = 0; i < MAX_PAIRUP_ALGORITHMS; i++)
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
            int rand = get_random_seed();
            if (rand % 2 == 0)
            {
                should_update_best = true;
            }
        }

        if (should_update_best)
        {
            if (best)
            {
                free_pair_result(best);
            }
            best = temp;
            id = i;
        }
        else
        {
            free_pair_result(temp);
        }
    }

    debug_action (DEBUG_INFO, (callback)print_worksheet, (void*)worksheet);
    debug_printf (DEBUG_SUMMARY, "Best Algorithm: %s", a[id].name);
    debug_action (DEBUG_SUMMARY, (callback)display_summary, (void*)best);
    debug_printf (DEBUG_SUMMARY, "Relation Graph:");
    debug_action (DEBUG_SUMMARY, (callback)display_graph, (void*)graph);

    free_relation_graph (graph);

    return best;
}

graph *
pairup_graph (sheet *worksheet)
{
    relation_graph *graph = new_relation_graph();
    member *member_list[MAX_MEMBERS_LEN] = { NULL };

    /* Generate relations using the existing member_list */
    /* This will take in the empty member_list and fill it with the available members */
    preprocess_fixed_memblist (worksheet, member_list);
    preprocess_relation_graph (worksheet, graph, member_list);

    pairup_internal algorithm = a[0].algorithm;

    if (!algorithm) return NULL;

    /* Get the pairing result of current algorithm */
    algorithm(graph, member_list);

    // printf("Best algorithm: %d\n", best_id);
    return graph;
}
/***************************  TOP LEVEL API (END)  ******************************/

/* Generate a random integer, ensuring srand is initialized only once */
static int
get_random_seed (void)
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
get_member_availability (sheet *worksheet,
                         int id)
{
    int j, count = 0;
    char cell[8];

    for (j = FILED_COL_START; j <= FILED_COL_END; j++)
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
get_member_earliest_slot (sheet *worksheet,
                           int id)
{
    int j;
    char cell[8];

    for (j = FILED_COL_START; j <= FILED_COL_END; j++)
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
get_member_requests (sheet *worksheet,
                     int id)
{
    int j;
    char cell[8];

    for (j = FILED_COL_START; j <= FILED_COL_END; j++)
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
get_member_name (sheet *worksheet,
                 int id)
{
    return worksheet->data[id][FILED_COL_NAME];
}

static int
preprocess_fixed_memblist (sheet *worksheet,
                           member *mlist[])
{
    int i, count = 0;

    for (i = FILED_ROW_START; i < worksheet->rows - 1; i++)
    {
        member *member = new_member();
        member->id = i;
        member->requests = get_member_requests(worksheet, i);
        member->availability = get_member_availability(worksheet, i);
        member->earliest_slot = get_member_earliest_slot(worksheet, i);

        char *name = get_member_name (worksheet, i);
        size_t lastchar = strnlen (name,MAX_NAME_LEN);
        strncpy(member->name, name, lastchar);
        member->name[lastchar] = '\0';

        mlist[i] = member;
        count++;
    }

    return count;
}

/********************  COMPARISON FUNCTIONS FOR QSORT (START)  ********************/

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
// static int
// compare_first_modified_asc ()
// {
//     return 0;
// }

/*********************  COMPARISON FUNCTIONS FOR QSORT (END)  *********************/

static int
find_member_id (graph *today,
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
preprocess_relation_graph (sheet *worksheet,
                           graph *today,
                           member *mlist[])
{
    int i, j, k;
    char cell[8];
    bool first = true;
    today->count = 0;

    /* Scan each row (members' name) */
    for (i = FILED_ROW_START; i < worksheet->rows-1; i++)
    {
        first = true;
        relation *row = NULL;

        /* Scan each column (time slots) */
        for (j = FILED_COL_START; j <= FILED_COL_END; j++)
        {
            get_cell(worksheet, i, j, cell, sizeof(cell));

            if (is_available(cell))
            {
                /* If this is the first available slot for this row, allocate a new member */
                if (first)
                {
                    /* Allocate memory for this member and initialize */
                    row = new_relation();
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
                for (k = FILED_ROW_START; k < worksheet->rows-1; k++)
                {
                    get_cell(worksheet, k, j, cell, sizeof(cell));

                    if (is_available(cell) && k != i)
                    {
                        if (row->count >= MAX_MATCHES_LEN - 1)
                        {
                            break;
                        }

                        row->matched_slot[row->count] = j;
                        row->candidates[row->count] = mlist[k];
                        row->count++;
                    }
                }

                // printf("Row %d has %zu candidates\n", i, row->count);
            }
        }
    }
}

static int 
has_time_slot (int available_slot[MAX_MEMBERS_LEN][MAX_MEMBERS_LEN],
                int row,
                int value)
{
    // printf("Checking row %d for time slot %d\n", row, value);
    for (int i = 0; i < MAX_MEMBERS_LEN; i++)
    {
        if (available_slot[row][i] == value)
        {
            return 1;
        }
    }

    return 0;
}

static void
remove_from_available_slot (int available_slot[MAX_MEMBERS_LEN][MAX_MEMBERS_LEN],
                             int row,
                             int value)
{
    int i;
    for (i = 0; i < MAX_MEMBERS_LEN; i++)
    {
        if (available_slot && available_slot[row][i] == value)
        {
            available_slot[row][i] = -1;
            // printf("Remove %d from row %d\n", value, row);
        }
    }
}

static int
result_existed (pair_result *result,
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
pairup_bfs (graph *today,
             member *members[],
             pair_result *result)
{
    /* Array that records the remaining time requested by each member */
    int remain[MAX_MEMBERS_LEN] = { 0 };
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

    int available_slot[MAX_MATCHES_LEN][MAX_MATCHES_LEN];
    int matched_slot[MAX_MATCHES_LEN][MAX_MATCHES_LEN];

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
            int bi = find_member_id(today, row->candidates[j]);

            if (remain[bi] <= 0 || 
                !has_time_slot(available_slot, i, matched_slot[i][j]) ||
                !has_time_slot(available_slot, bi, matched_slot[i][j]) ||
                !row->candidates[j])
            {
                continue;
            }

            b = row->candidates[j];

            if (result_existed(result, b, a))
            {
                continue;
            }

            if (a && b)
            {
                remain[i]--;
                remain[bi]--;

                pair *pair = new_pair();
                pair->a = a;
                pair->b = b;
                pair->time = row->matched_slot[j];
                result->pair_list[result->pairs] = pair;
                result->pairs++;

                remove_from_available_slot(available_slot, i, matched_slot[i][j]);
                remove_from_available_slot(available_slot, bi, matched_slot[i][j]);
            }

            break;
        }
    }

    result->total_requests = total_requests;

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
pairup_with_priority (graph *today,
                       member *members[],
                       int (*compare_fn)(const void *, const void *))
{
    /* Initialize the result */
    pair_result *result = new_pair_result(0, 0, 0);

    /* Sort the members based on the provided comparison function */
    qsort(today->relations, today->count, sizeof(relation *), compare_fn);

    debug_printf(DEBUG_INFO, "Sorted graph based on the priority:\n");
    debug_action(DEBUG_INFO, (callback)display_graph, (void*)today);

    /* Pair up the members */
    pairup_bfs(today, members, result);

    debug_printf(DEBUG_INFO, "Pair result summary:\n");
    debug_action(DEBUG_INFO, (callback)display_summary, (void*)result);

    return result;
}

pair_result *
pairup_least_availability_priority (graph *graph,
                                    member *members[])
{
    return pairup_with_priority(graph, members, compare_availability_asc);
}

pair_result *
pairup_most_availability_priority (graph *graph,
                                   member *members[])
{
    return pairup_with_priority(graph, members, compare_availability_desc);
}

pair_result *
pairup_smallest_row_id_priority (graph *graph,
                                 member *members[])
{
    return pairup_with_priority(graph, members, compare_id_asc);
}

pair_result *
pairup_largest_row_id_priority (graph *graph,
                                member *members[])
{
    return pairup_with_priority(graph, members, compare_id_desc);
}

pair_result *
pairup_earliest_available_slot_priority (graph *graph,
                                         member *members[])
{
    return pairup_with_priority(graph, members, compare_earliest_slot_asc);
}

pair_result *
pairup_latest_available_slot_priority (graph *graph,
                                       member *members[])
{
    return pairup_with_priority(graph, members, compare_earliest_slot_desc);
}

pair_result *
pairup_least_partner_priority (graph *graph,
                               member *members[])
{
    return pairup_with_priority(graph, members, compare_row_count_asc);
}

pair_result *
pairup_most_partner_priority (graph *graph,
                              member *members[])
{
    return pairup_with_priority(graph, members, compare_row_count_desc);
}

pair_result *
pairup_least_request_priority (graph *graph,
                               member *members[])
{
    return pairup_with_priority(graph, members, compare_requests_asc);
}

pair_result *
pairup_most_request_priority (graph *graph,
                              member *members[])
{
    return pairup_with_priority(graph, members, compare_requests_desc);
}
