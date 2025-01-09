#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pairup-types.h"

/********************************  Number of practices  *********************************/

const char *zero_sign[] = {"0", "０", " "};

const char *once_sign[] = {"1", "１", "V", "v", "X", "x", "Ｖ", "ｖ", "Ｘ", "ｘ", "once"};

const char *twice_sign[] = {"2", "２", "twice"};

/**********************************  Helper functions  **********************************/

/* Remove spaces from the string */
void remove_spaces (const char *str, char *result)
{
    while (*str != '\0')
    {
        if (*str != ' ')
        {
            *result++ = *str;
        }
        str++;
    }
    *result = '\0';
}

/* Convert the string to lower case */
void to_upper (const char *str, char *result)
{
    while (*str != '\0')
    {
        *result++ = toupper(*str);
        str++;
    }
    *result = '\0';
}

/* Normalize the string */
void normalize (const char *str, char *result)
{
    remove_spaces(str, result);
    to_upper(result, result);
}

/* Check if the sign represents 'practice zero time' */
bool is_zero (const char *sign)
{
    char src[10], dst[10];
    for (int i = 0; i < sizeof(zero_sign) / sizeof(zero_sign[0]); i++)
    {
        normalize(sign, src);
        normalize(zero_sign[i], dst);
        if (strcmp(src, dst) == 0)
        {
            return true;
        }
    }
    return false;
}

/* Check if the sign represents 'practice one time' */
bool is_once (const char *sign)
{
    char src[10], dst[10];
    for (int i = 0; i < sizeof(once_sign) / sizeof(once_sign[0]); i++)
    {
        normalize(sign, src);
        normalize(once_sign[i], dst);
        if (strcmp(src, dst) == 0)
        {
            return true;
        }
    }
    return false;
}

/* Check if the sign represents 'practice two times' */
bool is_twice (const char *sign)
{
    char src[10], dst[10];
    for (int i = 0; i < sizeof(twice_sign) / sizeof(twice_sign[0]); i++)
    {
        normalize(sign, src);
        normalize(twice_sign[i], dst);
        if (strcmp(src, dst) == 0)
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
new_member (void)
{
    return (member_t *) malloc (sizeof(member_t));
}

void
free_member (member_t *m)
{
    if (!m)
    {
        return;
    }

    free (m);
}

pair *
new_pair (void)
{
    return (pair *) malloc (sizeof(pair));
}

void
free_pair (pair *p)
{
    if (!p)
    {
        return;
    }

    free (p);
}

relation *
new_relation (void)
{
    return (relation *) malloc (sizeof(relation));
}

void
free_relation (relation *r)
{
    if (!r)
    {
        return;
    }
    // printf("relation address: %p\n", r);
    free (r);
}

relation_graph *
new_relation_graph (void)
{
    return (relation_graph *) malloc (sizeof(relation_graph));
}

void
free_relation_graph (relation_graph *graph)
{
    // printf("graph address: %p\n", graph);
    if (!graph)
    {
        return;
    }

    for (int i = 0; i < graph->count; i++)
    {
        free_relation (graph->relations[i]);
        graph->relations[i] = NULL;
    }

    free (graph);
}

pair_result *
new_pair_result (int pairs,
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
free_pair_result (pair_result *result)
{
    if (!result)
    {
        return;
    }

    free(result);
}
