#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pairup-types.h"

/********************************  Number of practices  *********************************/

const char *zero_sign[] = {"0", " "};

const char *once_sign[] = {"1", "V", "v", "X", "x", "once"};

const char *twice_sign[] = {"2", "twice"};

/**********************************  Helper functions  **********************************/

/* Check if the sign represents 'practice zero time' */
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

/* Check if the sign represents 'practice one time' */
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

/* Check if the sign represents 'practice two times' */
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

/* Check if the sign represents 'available' */
bool is_available (const char *sign)
{
    // return (!is_zero(sign) && (is_once(sign) || is_twice(sign)));
    return (is_once(sign) || is_twice(sign));
}

/****************************  Allocator and Deallocator  ********************************/

void
pairup_options_init (struct pairup_options *x)
{
    x->show_csv = false;
    x->generate_graph = false;
    x->ensure = false;
    x->priority = false;
    x->debug_level = 2;
}

member_t *
_new_member (void)
{
    return (member_t *) malloc (sizeof(member_t));
}

void
_free_member (member_t *m)
{
    if (!m)
    {
        return;
    }

    free (m);
}

pair_t *
_new_pair (void)
{
    return (pair_t *) malloc (sizeof(pair_t));
}

void
_free_pair (pair_t *p)
{
    if (!p)
    {
        return;
    }

    free (p);
}

relation_t *
_new_relation (void)
{
    return (relation_t *) malloc (sizeof(relation_t));
}

void
_free_relation (relation_t *r)
{
    if (!r)
    {
        return;
    }
    // printf("relation address: %p\n", r);
    free (r);
}

relation_graph_t *
_new_relation_graph (void)
{
    return (relation_graph_t *) malloc (sizeof(relation_graph_t));
}

void
_free_relation_graph (relation_graph_t *graph)
{
    // printf("graph address: %p\n", graph);
    if (!graph)
    {
        return;
    }

    for (int i = 0; i < graph->count; i++)
    {
        _free_relation (graph->relations[i]);
        graph->relations[i] = NULL;
    }

    free (graph);
}

pair_result_t *
_new_pair_result (int pairs,
                  int singles,
                  int members)
{
    pair_result_t *result = (pair_result_t *)malloc(sizeof(pair_result_t));
    if (result == NULL)
    {
        fprintf(stderr, "Memory allocation failed for pair_result_t\n");
        return NULL;
    }

    result->member = members;
    result->singles = singles;
    result->pairs = pairs;

    /* Currently the member graph, single_list, pair_list are not dynamically allocated */
    return result;
}

void
_free_pair_result (pair_result_t *result)
{
    if (!result)
    {
        return;
    }

    free(result);
}

