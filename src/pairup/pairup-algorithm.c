#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pairup-algorithm.h"
#include "pairup-types.h"
#include "rw-csv.h"

/* Public Access API */
/* Note that the result should be freed by the caller */
/* TODO: Implement all the algorithms and choose the best one */
pair_result_t *
pairup (sheet_t *sheet)
{
    adjmatrix_t *graph = _new_matches_graph ();
    _generate_matches_graph (sheet, graph);

    /* Priority: member-availability-based algorithm */
    pair_result_t *result = _pairup_least_availability_first (graph);

    _free_matches_graph (graph);
    return result;
}

/* Generate random seed based on the current time */
int
_get_random_seed (void)
{
    time_t now;
    time (&now);
    return (int) now;
}

static char *
_idx_to_name(const adjmatrix_t *graph, int idx)
{
    return graph->name_map[idx];
}

// void _count_availablity (adjmatrix_t *graph)
// {
//     int i, j;
//     int count = 0;
//     int partner;
//     
//     for (i = 0; i < graph->rows; i++)
//     {
//         for (j = 0; j < _MAX_MATCHES_LEN; j++)
//         {
//             partner = graph->members[i].matches[j];
//
//             if (partner == -1)
//             {
//                 break;
//             }
//
//             ++count;
//         }
//         graph->members[i].availability = count;
//     }
// }

static void 
_generate_matches_graph(sheet_t *sheet, adjmatrix_t *records)
{
    int i, j, k;
    char cell[8];

    /* Initialize the adjacency matrix's member count and rows */
    records->rows = 0;
    int member_capacity = 10;  // Initial capacity for members array
    records->members = (adjmatrix_row_t **) malloc(member_capacity * sizeof(adjmatrix_row_t *));

    /* Scan each row (members' name) */
    for (i = _FILED_ROW_START; i < sheet->rows; i++)
    {
        int count = 0;
        bool first = true;
        adjmatrix_row_t *member = NULL;

        /* Scan each column (time slots) */
        for (j = _FILED_COL_START; j <= _FILED_COL_END; j++)
        {
            get_cell(sheet, i, j, cell, sizeof(cell));

            if (is_available(cell))
            {
                /* If this is the first available slot for this row, allocate a new member */
                if (first)
                {
                    /* Resize the members array if capacity is reached */
                    if (records->rows >= member_capacity)
                    {
                        member_capacity *= 2;
                        records->members = (adjmatrix_row_t **) realloc(records->members, member_capacity * sizeof(adjmatrix_row_t *));
                    }

                    /* Allocate memory for this member and initialize */
                    records->members[records->rows] = (adjmatrix_row_t *) malloc(sizeof(adjmatrix_row_t));
                    member = records->members[records->rows];

                    /* Initialize the member data */
                    member->idx = i;
                    member->read_order = i;
                    member->earliest_time = j;
                    member->availability = 1;
                    member->count = 0;
                    member->remain = 1;
                    records->rows++;
                    if (is_twice(cell))
                    {
                        member->remain = 2;
                    }
                    first = false;
                }

                /* Update availability and match data */
                member->availability++;

                /* Search in the same column to find matching rows */
                for (k = _FILED_ROW_START; k < sheet->rows; k++)
                {
                    get_cell(sheet, k, j, cell, sizeof(cell));

                    /* Check if k has not been added */
                    for (int l = 0; l < count; l++)
                    {
                        if (member->matches[l] == k)
                        {
                            break;
                        }
                    }

                    if (is_available(cell) && k != i)
                    {
                        if (count >= _MAX_MATCHES_LEN - 1)
                        {
                            break;
                        }
                        member->matches[count++] = k;
                    }
                }
            }
        }

        /* Set the last element of matches to -1 to mark the end, if member was allocated */
        if (member)
        {
            member->matches[count] = -1;
            member->count = count;
            records->name_map[member->idx] = sheet->data[i][_FILED_COL_NAME];
        }
    }
}

/* TODO: Add time dimension to the pair_result */
static pair_result_t *
_pairup_least_availability_first(const adjmatrix_t *graph)
{
    int i, j, count = 0, partner_idx;
    int remain[_MAX_MATCHES_LEN] = {0};
    pair_result_t *result = _new_pair_result(0, 0, 0);

    // Initialize remain array
    for (i = 0; i < graph->rows; i++)
    {
        const adjmatrix_row_t *me = graph->members[i];
        if (me->idx < _MAX_MATCHES_LEN)
        {
            remain[me->idx] = me->remain;
        }
    }

    // Traverse graph for pairing
    for (i = 0; i < graph->rows; i++)
    {
        const adjmatrix_row_t *me = graph->members[i];

        if (me == NULL || remain[me->idx] == 0) continue; // Skip if unavailable
        int row_size = me->count;

        for (j = 0; j < row_size; j++)
        {
            partner_idx = me->matches[j];
            if (partner_idx < _MAX_MATCHES_LEN && remain[me->idx] > 0 && remain[partner_idx] > 0)
            {
                char *a = _idx_to_name(graph, me->idx);
                char *b = _idx_to_name(graph, partner_idx);

                if (count < _MAX_MATCHES_LEN)
                {
                    strncpy(result->pair_list[count].a, a, MAX_NAME_LEN - 1);
                    strncpy(result->pair_list[count].b, b, MAX_NAME_LEN - 1);
                    result->pair_list[count].a[MAX_NAME_LEN - 1] = '\0';
                    result->pair_list[count].b[MAX_NAME_LEN - 1] = '\0';
                    count++;
                }

                remain[me->idx]--;
                remain[partner_idx]--;
            }
        }
    }

    // Update result metadata
    result->member = graph->rows;
    result->pairs = count;
    result->singles = 0;

    /* Process singles */
    for (i = 0; i < _MAX_MATCHES_LEN; i++)
    {
        if (remain[i] > 0 && result->singles < _MAX_MATCHES_LEN)
        {
            char *name = _idx_to_name(graph, i);
            strncpy(result->single_list[result->singles], name, MAX_NAME_LEN - 1);
            result->single_list[result->singles][MAX_NAME_LEN - 1] = '\0';
            result->singles++;
        }
    }

    return result;
}
