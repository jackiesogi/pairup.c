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

pair *
_new_pair (void)
{
    return (pair *) malloc (sizeof(pair));
}

void
_free_pair (pair *p)
{
    if (!p)
    {
        return;
    }

    free (p);
}

relation *
_new_relation (void)
{
    return (relation *) malloc (sizeof(relation));
}

void
_free_relation (relation *r)
{
    if (!r)
    {
        return;
    }
    // printf("relation address: %p\n", r);
    free (r);
}

relation_graph *
_new_relation_graph (void)
{
    return (relation_graph *) malloc (sizeof(relation_graph));
}

void
_free_relation_graph (relation_graph *graph)
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

pair_result *
_new_pair_result (int pairs,
                  int singles,
                  int members)
{
    pair_result *result = (pair_result *)malloc(sizeof(pair_result));
    if (result == NULL)
    {
        fprintf(stderr, "Memory allocation failed for pair_result\n");
        return NULL;
    }

    result->member = members;
    result->singles = singles;
    result->pairs = pairs;

    /* Currently the member graph, single_list, pair_list are not dynamically allocated */
    return result;
}

void
_free_pair_result (pair_result *result)
{
    if (!result)
    {
        return;
    }

    free(result);
}

