#include <string.h>
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
pair_result_t *
_new_pair_result (int member,
                  int pairs,
                  int singles)
{
    pair_result_t *result = (pair_result_t *) malloc(sizeof(pair_result_t));

    result->member = member;
    result->pairs = pairs;
    result->singles = singles;

    result->member_list = (char **) malloc(member * sizeof(char *));
    for (int i = 0; i < member; i++)
    {
        result->member_list[i] = (char *) malloc(MAX_NAME_LEN * sizeof(char));
    }

    result->pair_list = (pair_t **) malloc(pairs * sizeof(pair_t *));
    for (int i = 0; i < pairs; i++)
    {
        result->pair_list[i] = (pair_t *) malloc(sizeof(pair_t));
    }

    result->single_list = (char **) malloc(singles * sizeof(char *));
    for (int i = 0; i < singles; i++)
    {
        result->single_list[i] = (char *) malloc(MAX_NAME_LEN * sizeof(char));
    }

    return result;
}

// Deallocator for pair_result_t
void
_free_pair_result (pair_result_t *result)
{
    if (!result)
        return;

    for (int i = 0; i < result->member; i++)
    {
        free(result->member_list[i]);
    }
    free(result->member_list);

    for (int i = 0; i < result->pairs; i++)
    {
        free(result->pair_list[i]);
    }
    free(result->pair_list);

    for (int i = 0; i < result->singles; i++)
    {
        free(result->single_list[i]);
    }
    free(result->single_list);

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
