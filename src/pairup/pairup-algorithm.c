#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "pairup-algorithm.h"
#include "pairup-types.h"
#include "rw-csv.h"

static void 
_generate_relations (sheet_t *worksheet,
                     relation_graph_t *today,
                     member_t *member_list[]);

static int
_generate_member_list (sheet_t *worksheet,
                       member_t *member_list[]);

/***************************  Public Access API  ******************************/
pair_result_t *
pairup (sheet_t *worksheet)
{
    relation_graph_t *graph = _new_relation_graph();
    member_t *member_list[_MAX_MEMBERS_LEN] = { NULL };

    /* Generate relations using the existing member_list */
    /* This will take in the empty member_list and fill it with the available members */
    _generate_member_list (worksheet, member_list);
    _generate_relations (worksheet, graph, member_list);

    /* Initialize the best result and temporary result */
    pair_result_t *best, *temp;
    best = temp = NULL;

    for (int i = 0; i < _MAX_PAIRUP_ALGORITHMS; i++)
    {
        pairup_fn algorithm = pairup_algorithms[i];

        if (!algorithm)  continue;

        /* Get the pairing result of current algorithm */
        temp = algorithm (graph, member_list);

        if (!best || temp->pairs > best->pairs)
        {
            if (best)
            {
                _free_pair_result (best);
            }
            best = temp;
        }
        else
        {
            _free_pair_result (temp);
        }
    }

    // _free_relation_graph (graph);
    return best;
}
/***************************  Public Access API  ******************************/

/* Generate random seed based on the current time */
int
_get_random_seed (void)
{
    time_t now;
    time (&now);
    return (int) now;
}

// static char *
// _idx_to_name(const relation_list_t *today, int idx)
// {
//     return today->name_map[idx];
// }

// void _count_availablity (relation_list_t *today)
// {
//     int i, j;
//     int count = 0;
//     int partner;
//     
//     for (i = 0; i < today->count; i++)
//     {
//         for (j = 0; j < _MAX_MATCHES_LEN; j++)
//         {
//             partner = today->members[i].matches[j];
//
//             if (partner == -1)
//             {
//                 break;
//             }
//
//             ++count;
//         }
//         today->members[i].availability = count;
//     }
// }

static int
_get_member_availability (sheet_t *worksheet,
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
_get_member_earliest_slot (sheet_t *worksheet,
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
_get_member_requests (sheet_t *worksheet,
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
_get_member_name (sheet_t *worksheet,
                  int id)
{
    return worksheet->data[id][_FILED_COL_NAME];
}

static int
_generate_member_list (sheet_t *worksheet,
                       member_t *member_list[])
{
    int i, count = 0;

    for (i = _FILED_ROW_START; i < worksheet->rows; i++)
    {
        member_t *member = _new_member();
        member->id = i;
        strncpy(member->name, _get_member_name(worksheet, i), MAX_NAME_LEN - 1);
        member->name[MAX_NAME_LEN - 1] = '\0';
        member->requests = _get_member_requests(worksheet, i);
        member->availability = _get_member_availability(worksheet, i);
        member->earliest_slot = _get_member_earliest_slot(worksheet, i);

        member_list[i] = member;
        count++;
    }

    return count;
}

/************************************************************************************/

#define DEFAULT_PRIORITY 0

static void 
_generate_relations (sheet_t *worksheet,
                     relation_graph_t *today,
                     member_t *member_list[])
{
    int i, j, k, l;
    char cell[8];
    bool first = true;
    today->count = 0;

    /* Scan each row (members' name) */
    for (i = _FILED_ROW_START; i < worksheet->rows; i++)
    {
        first = true;
        relation_t *row = NULL;

        /* Scan each column (time slots) */
        for (j = _FILED_COL_START; j <= _FILED_COL_END; j++)
        {
            get_cell(worksheet, i, j, cell, sizeof(cell));
            // printf("get_cell() at row: %d, col: %d. gets %s\n", i, j, cell);

            if (is_available(cell))
            {
                // member_t *partner = NULL;

                /* If this is the first available slot for this row, allocate a new member */
                if (first)
                {
                    /* Allocate memory for this member and initialize */
                    row = _new_relation();
                    row->priority = DEFAULT_PRIORITY;
                    row->candidates[0] = member_list[i];
                    row->count = 1;

                    today->relations[today->count] = row;
                    today->count++;

                    first = false;
                }

                /* Search in the same column to find matching count */
                for (k = _FILED_ROW_START; k < worksheet->rows; k++)
                {
                    bool already_added = false;

                    /* Check if k has not been added */
                    for (l = 1; l < row->count; l++)
                    {
                        if (row->candidates[l]->id == k)
                        {
                            already_added = true; // Mark as already added
                            break;                // Exit inner loop
                        }
                    }

                    if (already_added)
                    {
                        continue; // Skip the rest of this iteration in the outer loop
                    }

                    get_cell(worksheet, k, j, cell, sizeof(cell));

                    if (is_available(cell) && k != i)
                    {
                        if (row->count >= _MAX_MATCHES_LEN - 1)
                        {
                            break;
                        }

                        row->candidates[row->count] = member_list[k];
                        row->count++;
                    }
                }
            }
        }
    }
}

/* Compare functions for qsort */
static int
compare_availability (const void *a,
                      const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return (ra->candidates[0]->availability -
            rb->candidates[0]->availability);  // Tie-breaking by priority
}

static int
compare_row_count (const void *a,
               const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return ra->count - rb->count;  // Tie-breaking by priority
}

static int
compare_id (const void *a,
            const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return (ra->candidates[0]->id - rb->candidates[0]->id);
}

static int
compare_requests (const void *a,
                  const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return (ra->candidates[0]->requests - rb->candidates[0]->requests);
}

static int
compare_earliest_slot (const void *a,
                       const void *b)
{
    const relation_t *ra = *(const relation_t **)a;
    const relation_t *rb = *(const relation_t **)b;

    return (ra->candidates[0]->earliest_slot -
            rb->candidates[0]->earliest_slot);
}

/* Debug purpose */
static void
print_graph (relation_graph_t *graph)
{
    for (int i = 0; i < graph->count; i++)
    {
        relation_t *row = graph->relations[i];
        printf("Relation %d: [ %s ] --> ", i, row->candidates[0]->name);
        for (int j = 1; j < row->count; j++)
        {
            printf("%s --> ", row->candidates[j]->name);
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

static void *
_pairup_bfs (relation_graph_t *today,
             member_t *members[],
             pair_result_t *result)
{
    /* Array that records the remaining time requested by each member */
    int remain[_MAX_MEMBERS_LEN] = { 0 };
    printf("Original request of each member:\n");
    for (int i = 0; i < today->count; i++)
    {
        remain[i] = today->relations[i]->candidates[0]->requests;
        printf("Member %s has %d requests\n", today->relations[i]->candidates[0]->name, remain[i]);
    }

    printf("Today's count: %zu\n", today->count);
    /* Use BFS to pair up the members, starting from the first row */
    for (int i = 0; i < today->count; i++)
    {
        printf("Processing row %d: Member '%s'\n", i, today->relations[i]->candidates[0]->name);
        /* If already paired, skip */
        if (remain[i] == 0)
        {
            continue;
        }

        relation_t *row = today->relations[i];
        member_t *a = row->candidates[0];  // himself/herself
        member_t *b = NULL;  // To be paired

        /* Pair up the members */
        for (int j = 1; j < row->count; j++)
        {
            int bi = _find_member_id(today, row->candidates[j]);

            printf("    Try pairing with member %s ...... ", today->relations[i]->candidates[j]->name);
            if (remain[bi] <= 0 || !row->candidates[j])
            {
                printf("Failed!\n");
                continue;
            }

            printf("Success!\n");
            b = row->candidates[j];

            remain[i]--;
            remain[bi]--;

            break;
        }

        if (a && b)
        {
            pair_t *pair = _new_pair();
            pair->a = a;
            pair->b = b;
            result->pair_list[result->pairs++] = pair;
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

/* TODO: Add time dimension to the pair_result */
/* TODO: Sort not only rows but also elements in that row */
static pair_result_t *
_pairup_least_availability_first (relation_graph_t *today,
                                  member_t *members[])
{
    /* Initialize the result */
    pair_result_t *result = _new_pair_result(0, 0, 0);

    print_graph(today);
    /* Sort the members based on the availability */
    qsort(today->relations, today->count, sizeof(relation_t *), compare_availability);
    print_graph(today);

    /* Pair up the members */
    _pairup_bfs (today, members, result);

    return result;
}
