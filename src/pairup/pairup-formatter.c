#include <stdio.h>
#include "pairup-formatter.h"
#include "pairup-types.h"
#include "pairup-algorithm.h"
#include "rw-csv.h"

void
print_result (sheet_t *worksheet,
              pair_result_t *result)
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
            char *time = _get_time_slot(worksheet, pair->time);
            printf("%s -- %s (%s)\n", pair->a->name, pair->b->name, time);
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
print_result_statistics (pair_result_t *result)
{
    int M = result->member;
    int P = result->pairs;
    int NP = result->singles;
    int REQ = result->total_requests;
    int RATIO_P = (P * 100) / M;
    int RATIO_NP = (NP * 100) / M;

    printf("=======================  STATISTICS  =======================\n");
    printf("Number of members today: %d\n", result->member);
    printf("Number of successful pairs: %d (%d%%)\n", result->pairs, RATIO_P);
    printf("Number of members not fully paired: %d (%d%%)\n", result->singles, RATIO_NP);
    printf("Total requests of today %d\n", REQ);
    printf("Number of fully paired requests %d (%d%%)\n", P * 2, (P * 2 * 100) / REQ);
    printf("Number of not fully paired requests %d (%d%%)\n", NP, (NP * 100) / REQ);
    printf("Pair list:\n");
    for (int i = 0; i < result->pairs; i++)
    {
        printf("%s -- %s\n", result->pair_list[i]->a->name, result->pair_list[i]->b->name);
    }
    printf("Single list:\n");
    for (int i = 0; i < result->singles; i++)
    {
        printf("%s\n", result->single_list[i]->name);
    }
    printf("=============================================================\n");
}
