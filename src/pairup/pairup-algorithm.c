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
_generate_matches_graph (sheet_t *sheet,
                         adjmatrix_t *records)
{
    int i, j, k;
    char cell[8];

    /* Build the adjacency matrix representing the match condition */
    /* Scan each row (members' name) */
    for (i = _FILED_ROW_START; i < sheet->rows; i++)
    {
        int count = 0;
        bool first = false;
        adjmatrix_row_t *member = &records->members[i];

        /* Scan each column (time slots) */
        for (j = _FILED_COL_START; j <= _FILED_COL_END; j++)
        {
            get_cell (sheet, i, j, cell, sizeof (cell));

            if (is_available (cell))
            {
                /* Record the first available time slot */
                if (!first)
                {
                    records->rows++;
                    member->earliest_time = j;
                    first = true;
                }

                member->idx = i;
                member->read_order = i;
                member->availability++;

                /* Search in same column to see which row is also available */
                for (k = _FILED_ROW_START; k < sheet->rows; k++)
                {
                    get_cell (sheet, k, j, cell, sizeof (cell));

                    if (is_available (cell))
                    {
                        member->matches[count++] = k;
                    }
                }
            }
        }

        /* Important: Set the last element to -1 */
        records->members[i].matches[count] = -1;
        records->members[i].count = count;
        records->name_map[i] = &sheet->data[i][_FILED_COL_NAME];
    }
}

/* TODO: Add time dimension to the pair_result */
static pair_result_t *
_pairup_least_availability_first (const adjmatrix_t *graph)
{
    int i, j;  // Loop index
    int count = 0;  // Calculate the total number of pairs
    int partner_idx;  // The index of the partner in the original sheet
    int remain[_MAX_MATCHES_LEN] = {0};  // The remaining practice time (either 0, 1, or 2)
    
    pair_result_t *result = _new_pair_result(0, 0, 0);

    /* Initialize the array to store the ramaining practice time */
    for (i = 0; i < graph->rows; i++)
    {
        const adjmatrix_row_t me = graph->members[i];
        remain[me.idx] = me.availability;
    }

    /* Start traversing the graph using BFS and match the members */
    for (i = 0; i < graph->rows; i++)
    {
        const adjmatrix_row_t me = graph->members[i];
        int row_size = me.count;

        for (j = 0; j < row_size; j++)
        {
            partner_idx = me.matches[j];

            /* Check if the current member and its potential partner are both available */
            if (remain[i] > 0 && remain[partner_idx] > 0)
            {
                /* Retrieve the name of the current member and its partner */
                char *a = _idx_to_name(graph, me.idx);
                char *b = _idx_to_name(graph, partner_idx);

                /* Copy the names into the result */
                strncpy(result->pair_list[count]->a, a, MAX_NAME_LEN - 1);
                strncpy(result->pair_list[count]->b, b, MAX_NAME_LEN - 1);

                /* Ensure the string is null-terminated */
                result->pair_list[count]->a[MAX_NAME_LEN - 1] = '\0';
                result->pair_list[count]->b[MAX_NAME_LEN - 1] = '\0';

                /* Increment the counter and substract the remaining time by 1 */
                ++count;
                --remain[me.idx];
                --remain[partner_idx];
            }
        }
    }

    /* Update the result metadata */
    result->member = graph->rows;
    result->pairs = count;
    result->singles = 0;

    /* Handle the singles */
    for (i = 0; i < _MAX_MATCHES_LEN; i++)
    {
        if (remain[i] > 0)
        {
            char *name = _idx_to_name(graph, i);
            strncpy(result->single_list[result->singles], name, MAX_NAME_LEN - 1);
            result->single_list[result->singles][MAX_NAME_LEN - 1] = '\0';
            ++result->singles;
        }
    }

    return result;
}

