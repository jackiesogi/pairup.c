#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pairup-types.h"

const char *zero_sign[] = {"0", " "};
const char *once_sign[] = {"1", "V", "v", "X", "x", "once"};
const char *twice_sign[] = {"2", "twice"};

bool is_zero (const char *sign)
{
    for (int i = 0; i < sizeof(zero_sign) / sizeof(zero_sign[0]); i++)
    {
        if (strcmp(sign, zero_sign[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

bool is_once (const char *sign)
{
    for (int i = 0; i < sizeof(once_sign) / sizeof(once_sign[0]); i++)
    {
        if (strcmp(sign, once_sign[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

bool is_twice (const char *sign)
{
    for (int i = 0; i < sizeof(twice_sign) / sizeof(twice_sign[0]); i++)
    {
        if (strcmp(sign, twice_sign[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

bool is_available (const char *sign)
{
    // return (!is_zero(sign) && (is_once(sign) || is_twice(sign)));
    return (is_once(sign) || is_twice(sign));
}

/* Allocator and deallocator */
/* Allocator for pair_result_t */
pair_result_t *
_new_pair_result(int pairs, int singles, int members)
{
    pair_result_t *result = (pair_result_t *)malloc(sizeof(pair_result_t));
    if (result == NULL)
    {
        fprintf(stderr, "Memory allocation failed for pair_result_t\n");
        return NULL;
    }

    result->pairs = pairs;
    result->singles = singles;
    result->member = members;

    // Initialize pair_list and single_list arrays with empty strings
    for (int i = 0; i < _MAX_MATCHES_LEN; i++)
    {
        result->pair_list[i].a[0] = '\0';
        result->pair_list[i].b[0] = '\0';
        result->single_list[i][0] = '\0';
    }

    return result;
}

/* Deallocator for pair_result_t */
void
_free_pair_result(pair_result_t *result)
{
    if (!result)
        return;

    // Since member_list, pair_list, and single_list are statically allocated in struct,
    // we only need to free the result itself
    free(result);
}

adjmatrix_t *
_new_matches_graph (void)
{
    adjmatrix_t *graph = (adjmatrix_t *) malloc (sizeof (adjmatrix_t));
    if (graph == NULL)
    {
        return NULL;
    }

    graph->rows = 0;

    for (int i = 0; i < _MAX_MATCHES_LEN; i++)
    {
        graph->name_map[i] = NULL;
    }

    graph->members = (struct adjmatrix_row **) malloc (_MAX_MATCHES_LEN * sizeof(struct adjmatrix_row *));
    if (graph->members == NULL)
    {
        free(graph);
        return NULL;
    }

    for (int i = 0; i < _MAX_MATCHES_LEN; i++)
    {
        graph->members[i] = NULL;
    }

    return graph;
}

void
_free_matches_graph (adjmatrix_t *graph)
{
    for (int i = 0; i < _MAX_MATCHES_LEN; i++)
    {
        free(graph->name_map[i]);
    }

    free(graph);
}
