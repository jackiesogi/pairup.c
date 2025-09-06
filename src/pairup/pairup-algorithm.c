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

/* Various pre-defined priority */
/* Technically, `pairup_algorithms[]` is an array with several function pointers */
/* pointing to corresponding priority functions */
const pairup_algorithm a[] = {                    // WHO WILL BE PAIRED UP FIRST?
    {
        "LEAST_AVAILABILITY_PRIORITY",            // Members with least time slots filled in.
         pairup_least_availability_priority,
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
    {
        NULL,                                     // Terminating condition
        NULL
    },
};

/********************  INTERNAL FUNCTIONS DECLARATION (START)  *********************/
/* Generate a random integer, only for internal use */
static int
preprocess_fixed_memblist (sheet *worksheet,
                           member *mlist[],
                           void *elist);

static void
preprocess_relation_graph (sheet *worksheet,
                           graph *today,
                           member *mlist[]);

static int
get_random_int (void);

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

static
pairup_internal
get_algorithm_by_name (const char *target)
{
    for (int i = 0; a[i].algorithm != NULL; i++)
    {
        if (strncmp(a[i].name, target, 1024) == 0)
        {
            return a[i].algorithm;
        }
    }
    return NULL;
}

pair_result *
pairup_ensure_list_priority (graph *graph,
                             member *members[]);

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
compare_ensure_list (const void *a,
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
__pairup__ (sheet *worksheet,
        struct pairup_options *x)
{
    relation_graph *graph = new_relation_graph ();
    member *member_list[MAX_MEMBERS_LEN] = { NULL };

    /* Generate relations using the existing member_list */
    /* This will take in the empty member_list and fill it with the available members */
    preprocess_fixed_memblist (worksheet, member_list, (void *)x->ensure_member_list);
    preprocess_relation_graph (worksheet, graph, member_list);

    /* Initialize the best result and temporary result */
    result *best = NULL, *temp = NULL;
    int id = -1;

    int max_success_rate = 0;
    int current_success_rate = 0;
    int current_total_requests = (temp && temp->total_requests > 0) ? temp->total_requests : 1;
    for (int i = 0; a[i].algorithm != NULL; i++)
    {
        pairup_internal algorithm;

        current_success_rate = (temp) ? (temp->pairs * 200 / current_total_requests) : current_success_rate;
        if (current_success_rate > max_success_rate)
            max_success_rate = current_success_rate;

        debug_printf (DEBUG_INFO, "\
[ INFO    ] Current successful request rate is at %d\%, trying next one ...\n",
max_success_rate);

        /* If --ensure={MEMBER} is used, then we do not care  */
        /* about the pre-defined algorithm. Instead, we tried */ 
        /* the `pairup_ensure_list_priority` several times to */
        /* prioritize {MEMBER}.                               */
        if (x->ensure == true)
        {
            algorithm = pairup_ensure_list_priority;
        }
        else if (x->priority == true)
        {
            algorithm = get_algorithm_by_name (x->priority_func);
            if (!algorithm)
            {
                debug_printf (DEBUG_WARNING,
                              "[ WARNING ] No pairup algorithm called '%s', fallback to default.\n",
                              x->priority_func);
                x->priority = false;
            }
            else
            {
                debug_printf (DEBUG_INFO,
                              "[ INFO    ] Set '%s' as the pairup algorithm.\n",
                              x->priority_func);
            }
        }
        else
        {
            algorithm = a[i].algorithm;
        }

        if (!algorithm) continue;

        /* Get the pairing result of current algorithm */
        temp = algorithm (graph, member_list);
        temp->algorithm_applied = &a[i];

        debug_printf (DEBUG_INFO, "[ INFO    ] Applying the priority '%s' ...\n",
                      temp->algorithm_applied->name);
        debug_action (DEBUG_INFO, (callback)display_graph, (void*)graph);

        debug_printf (DEBUG_INFO, "[ SUMMARY ] Generating result summary ...\n");
        debug_action (DEBUG_INFO, (callback)display_summary, (void*)temp);

        bool should_update_best = false;

        if (!best || temp->pairs > best->pairs)
        {
            should_update_best = true;
        }
        else if (temp->pairs == best->pairs)
        {
            int rand = get_random_int ();
            if (rand % 2 == 0)
            {
                should_update_best = true;
            }
        }

        current_total_requests = (temp && temp->total_requests > 0) ? temp->total_requests : 1;
        if (should_update_best)
        {
            debug_printf (DEBUG_INFO, "\
[ INFO    ] %s has better successful request rate (%3d\%) than previous one, updating ...\n",
temp->algorithm_applied->name,
(temp->pairs * 200 / current_total_requests));
            if (best)
            {
                free_pair_result (best);
            }
            best = temp;
            id = i;
        }
        else
        {
            debug_printf (DEBUG_INFO, "\
[ INFO    ] %s does not have a better successful request rate (%3d\%), skipping ...\n",
temp->algorithm_applied->name,
(temp->pairs * 200 / current_total_requests));

            free_pair_result (temp);
        }

        if (best->total_requests == (best->pairs << 1))
        {
            debug_printf (DEBUG_INFO, "[ INFO    ] Found the maximum matches, stop searching!\n");
            break;
        }
    }
    debug_printf (DEBUG_INFO, "[ INFO    ] No more method to try.\n");
    debug_printf (DEBUG_INFO, "[ INFO    ] Found best method: %s.\n", best->algorithm_applied->name);

    debug_action (DEBUG_INFO, (callback)print_worksheet, (void*)worksheet);
    if (x->ensure == true || x->priority == true)
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Best Algorithm: None (Prioritized specific member(s))\n");
    else
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Best Algorithm: %s\n", a[id].name);
    debug_action (DEBUG_SUMMARY, (callback)display_summary, (void*)best);
    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Relation Graph:\n");
    debug_action (DEBUG_SUMMARY, (callback)display_graph, (void*)graph);

    free_relation_graph (graph);

    return best;
}

graph *
pairup_graph (sheet *worksheet)
{
    relation_graph *graph = new_relation_graph ();
    member *member_list[MAX_MEMBERS_LEN] = { NULL };

    /* Generate relations using the existing member_list */
    /* This will take in the empty member_list and fill it with the available members */
    preprocess_fixed_memblist (worksheet, member_list, NULL);
    preprocess_relation_graph (worksheet, graph, member_list);

    pairup_internal algorithm = a[0].algorithm;

    if (!algorithm) return NULL;

    /* Get the pairing result of current algorithm */
    algorithm (graph, member_list);

    // printf("Best algorithm: %d\n", best_id);
    return graph;
}
/***************************  TOP LEVEL API (END)  ******************************/

/* Generate a random integer, ensuring srand is initialized only once */
static int
get_random_int (void)
{
    static int initialized = 0; // Static variable to track initialization

    if (!initialized)
    {
        srand ((unsigned int)time(NULL));
        initialized = 1; // Mark as initialized
    }

    return rand ();
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
    if (!worksheet || !worksheet->data ||
        !worksheet->data[id] ||
        !worksheet->data[id][FILED_COL_NAME])
    {
        return NULL;
    }
    return worksheet->data[id][FILED_COL_NAME];
}

/* New feature under development */
static int
get_row_id_by_name (sheet *worksheet,
                    const char *name)
{
    for (int i = FILED_ROW_START; i < worksheet->rows; i++)
    {
        if (strcmp(name, worksheet->data[i][FILED_COL_NAME]) == 0)
        {
            return i;
        }
    }
    return -1;
}

/* Calculate the ensure score    */
static int
get_member_ensure_score (sheet *worksheet,
                         void *elist,
                         int row_id)
{
/* Windows has unkown hang issue on calculating ensure score*/
#if (defined(_WIN32) || defined(_WIN64))
    return 0;
#endif

    /* return the index in the ensure list and save to cache */
    static bool initialized = false;
    static int score_cache[MAX_MEMBERS_LEN];

    if (initialized == false)
    {
        udel *ensure = (udel *)elist;
        size_t highest = ensure->ensure_list_size + 1;
        for (int i = 0; i < ensure->ensure_list_size; i++)
        {
            debug_printf (DEBUG_INFO,
                          "[ INFO    ] %s will be prioritized, with score = %d.\n",
                          ensure->ensure_list_content[i],
                          highest
            );

            int id = get_row_id_by_name (worksheet, ensure->ensure_list_content[i]);

            if (id >= 0) {
                score_cache[id] = highest--;
            } else {
                debug_printf(DEBUG_WARNING, "[ WARNING ] Warning: '%s' not found in worksheet\n", ensure->ensure_list_content[i]);
            }
        }
        // Optional: Add a loop to fill priority "1" to the rest of the row id
        initialized = true;
    }
    return score_cache[row_id];
}

static int
preprocess_fixed_memblist (sheet *worksheet,
                           member *mlist[],
                           void *elist)
{
    int i, count = 0;

    debug_printf(DEBUG_INFO, "[ INFO    ] Generating fixed member list ...\n");

    for (i = FILED_ROW_START; i < worksheet->rows - 1; i++)
    {
        member *member = new_member ();

        char *name = get_member_name (worksheet, i);
        if (name == NULL)
            strncpy(name, " -- ", 5);
        size_t lastchar = strnlen (name, MAX_NAME_LEN);
        strncpy (member->name, name, lastchar);
        member->name[lastchar] = '\0';

        member->id = i;
        member->requests = get_member_requests (worksheet, i);
        member->availability = get_member_availability (worksheet, i);
        member->earliest_slot = get_member_earliest_slot (worksheet, i);

        /* New */
        member->ensure_score = (elist) ? get_member_ensure_score (worksheet, elist, i) : 0;

        mlist[i] = member;
        count++;

        debug_printf(DEBUG_ALL, "\
[ ALL     ] On row %2d, found member '%s' with availability=%d, request=%d, \
earliest_slot=%d and ensure_score=%d.\n",
member->id,
member->name,
member->availability,
member->requests,
member->earliest_slot,
member->ensure_score);

    }

    debug_printf(DEBUG_INFO, "[ INFO    ] Finshed generating fixed member list.\n");
    return count;
}

// static int
// preprocess_fixed_memblist(sheet *worksheet,
//                           member *mlist[],
//                           void *elist)
// {
//     int i, count = 0;

//     debug_printf(DEBUG_INFO, "[ INFO    ] Generating fixed member list ...\n");

//     printf("[DEBUG   ] worksheet->rows=%d, worksheet->cols=%d\n", worksheet->rows, worksheet->cols);

//     for (i = FILED_ROW_START; i < worksheet->rows - 1; i++)
//     {
//         printf("[DEBUG   ] Processing row %d...\n", i);

//         // 建立 member
//         member *member = new_member();
//         if (!member)
//         {
//             fprintf(stderr, "[ERROR   ] new_member() failed at row %d\n", i);
//             continue;
//         }

//         // 取得姓名
//         char *name = get_member_name(worksheet, i);
//         if (!name)
//         {
//             fprintf(stderr, "[ERROR   ] get_member_name returned NULL for row %d\n", i);
//             free(member);
//             continue;
//         }

//         size_t lastchar = strnlen(name, MAX_NAME_LEN);
//         size_t copy_len = (lastchar < MAX_NAME_LEN - 1) ? lastchar : MAX_NAME_LEN - 1;
//         strncpy(member->name, name, copy_len);
//         member->name[copy_len] = '\0';

//         printf("[DEBUG   ] Member name: '%s' (len=%zu, copy_len=%zu)\n",
//                member->name, lastchar, copy_len);

//         member->id = i;

//         // 其他欄位
//         printf("[DEBUG] Getting requests for row %d...\n", i);
//         member->requests = get_member_requests(worksheet, i);
//         printf("[DEBUG] requests=%d\n", member->requests);

//         printf("[DEBUG] Getting availability for row %d...\n", i);
//         member->availability = get_member_availability(worksheet, i);
//         printf("[DEBUG] availability=%d\n", member->availability);

//         printf("[DEBUG] Getting earliest_slot for row %d...\n", i);
//         member->earliest_slot = get_member_earliest_slot(worksheet, i);
//         printf("[DEBUG] earliest_slot=%d\n", member->earliest_slot);

//         // 處理 ensure_score，如果 elist 不為 NULL
//         member->ensure_score = 0;
//         if (elist)
//         {
//             printf("[DEBUG] Getting ensure_score for row %d...\n", i);

//             int iter_count = 0;
//             int max_iter = 1000;  // 避免死循環
//             member->ensure_score = -1; // 預設 -1 表示未找到

//             while (iter_count < max_iter)
//             {
//                 member->ensure_score = get_member_ensure_score(worksheet, elist, i);
//                 if (member->ensure_score >= 0)
//                     break;

//                 iter_count++;
//             }

//             if (iter_count == max_iter)
//                 printf("[WARN    ] ensure_score loop exceeded max iterations at row %d\n", i);

//             printf("[DEBUG] ensure_score=%d\n", member->ensure_score);
//         }

//         printf("[DEBUG] jump out of ensure_score basic block\n");
//         // 放入列表
//         mlist[i] = member;
//         count++;
//         printf("[DEBUG] finished incrementing count, the debug level is %d\n", debug_level);


//         debug_printf(DEBUG_ALL, "[ALL     ] Finished processing row %2d: '%s'\n",
//                      member->id, member->name);
//     }

//     debug_printf(DEBUG_INFO, "[ INFO    ] Finished generating fixed member list. Total members=%d\n", count);
//     return count;
// }

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

static int
compare_ensure_list (const void *a,
                     const void *b)
{
    const relation *ra = *(const relation **)a;
    const relation *rb = *(const relation **)b;

    /* Note that higher ensure score means higher priority, */
    /* so we do a substraction in a reverse order.          */
    int delta = rb->candidates[0]->ensure_score -
                ra->candidates[0]->ensure_score;

    if (delta == 0)
    {
        if (get_random_int () % 2 == 0)
        {
            return -1;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return delta;
    }
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
            get_cell (worksheet, i, j, cell, sizeof(cell));

            if (is_available(cell))
            {
                /* If this is the first available slot for this row, allocate a new member */
                if (first)
                {
                    /* Allocate memory for this member and initialize */
                    row = new_relation ();
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
                    get_cell (worksheet, k, j, cell, sizeof(cell));

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
            int bi = find_member_id (today, row->candidates[j]);

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

                pair *pair = new_pair ();
                pair->a = a;
                pair->b = b;
                pair->time = row->matched_slot[j];
                result->pair_list[result->pairs] = pair;
                result->pairs++;

                remove_from_available_slot (available_slot, i, matched_slot[i][j]);
                remove_from_available_slot (available_slot, bi, matched_slot[i][j]);
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

static pair_result *
pairup_with_priority (graph *today,
                      member *members[],
                      int (*compare_fn)(const void *, const void *))
{
    /* Initialize the result */
    pair_result *result = new_pair_result (0, 0, 0);

    /* Sort the members based on the provided comparison function */
    qsort (today->relations, today->count, sizeof(relation *), compare_fn);

    /*debug_printf (DEBUG_INFO, "[INFO   ] Sorted graph based on the priority.\n");*/
    /*debug_action (DEBUG_INFO, (callback)display_graph, (void*)today);*/

    /* Pair up the members */
    pairup_bfs (today, members, result);

    /*debug_printf (DEBUG_INFO, "[SUMMARY] Pair result summary:\n");*/
    /*debug_action (DEBUG_INFO, (callback)display_summary, (void*)result);*/

    return result;
}

/* Ensure List: ['Alice', 'Bob', 'Jackie'] */
pair_result *
pairup_ensure_list_priority (graph *graph,
                             member *members[])
{
    return pairup_with_priority (graph, members, compare_ensure_list);
}

pair_result *
pairup_least_availability_priority (graph *graph,
                                    member *members[])
{
    return pairup_with_priority (graph, members, compare_availability_asc);
}

pair_result *
pairup_most_availability_priority (graph *graph,
                                   member *members[])
{
    return pairup_with_priority (graph, members, compare_availability_desc);
}

pair_result *
pairup_smallest_row_id_priority (graph *graph,
                                 member *members[])
{
    return pairup_with_priority (graph, members, compare_id_asc);
}

pair_result *
pairup_largest_row_id_priority (graph *graph,
                                member *members[])
{
    return pairup_with_priority (graph, members, compare_id_desc);
}

pair_result *
pairup_earliest_available_slot_priority (graph *graph,
                                         member *members[])
{
    return pairup_with_priority (graph, members, compare_earliest_slot_asc);
}

pair_result *
pairup_latest_available_slot_priority (graph *graph,
                                       member *members[])
{
    return pairup_with_priority (graph, members, compare_earliest_slot_desc);
}

pair_result *
pairup_least_partner_priority (graph *graph,
                               member *members[])
{
    return pairup_with_priority (graph, members, compare_row_count_asc);
}

pair_result *
pairup_most_partner_priority (graph *graph,
                              member *members[])
{
    return pairup_with_priority (graph, members, compare_row_count_desc);
}

pair_result *
pairup_least_request_priority (graph *graph,
                               member *members[])
{
    return pairup_with_priority (graph, members, compare_requests_asc);
}

pair_result *
pairup_most_request_priority (graph *graph,
                              member *members[])
{
    return pairup_with_priority (graph, members, compare_requests_desc);
}
