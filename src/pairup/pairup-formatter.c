#include <stdio.h>
#include "pairup-formatter.h"
#include "pairup-types.h"

void
print_result(int pairs,
             int singles,
             const pair_t *pair_list[],
             const char *single_list[])
{
    // Print the first pair if available
    if (pairs > 0 && pair_list[0] != NULL)
    {
        printf("%s -- %s\n", pair_list[0]->a, pair_list[0]->b);
    }
    else
    {
        printf("No pairs available to display.\n");
    }
}

void
print_result_full (pair_result_t *result)
{
    printf("Pairs: %d\n", result->pairs);
    printf("Singles: %d\n", result->singles);
    printf("Members: %d\n", result->member);
    printf("Pair list:\n");
    for (int i = 0; i < result->pairs; i++)
    {
        printf("%s -- %s\n", result->pair_list[i].a, result->pair_list[i].b);
    }
    printf("Single list:\n");
    for (int i = 0; i < result->singles; i++)
    {
        printf("%s\n", result->single_list[i]);
    }
}
