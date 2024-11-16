#include <stdio.h>
#include "pairup-formatter.h"
#include "pairup-types.h"

void
print_result (pair_result_t *result)
{
    printf("%s\n", GREETING);

    if (result->pairs == 0)
    {
        printf("No pairs available to display.\n");
    }
    else 
    {
        for (int i = 0; i < result->pairs; i++)
        {
            pair_t *pair = result->pair_list[i];
            printf("%s -- %s\n", pair->a->name, pair->b->name);
        }
    }
    
    printf("\n");
    printf("As for\n");

    if (result->singles != 0)
    {
        for (int i = 0; i < result->singles; i++)
        {
            member_t *member = result->single_list[i];
            printf("%s\n", member->name);
        }
    }

    printf("%s\n", ALTERNATIVES_FOR_NOT_MATCHED);
}

void
print_result_full (pair_result_t *result)
{
    // printf("Pairs: %d\n", result->pairs);
    // printf("Singles: %d\n", result->singles);
    // printf("Members: %d\n", result->member);
    // printf("Pair list:\n");
    // for (int i = 0; i < result->pairs; i++)
    // {
    //     printf("%s -- %s\n", result->pair_list[i].a, result->pair_list[i].b);
    // }
    // printf("Single list:\n");
    // for (int i = 0; i < result->singles; i++)
    // {
    //     printf("%s\n", result->single_list[i]);
    // }
}
