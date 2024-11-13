#include <stdio.h>
#include "pairup-formatter.h"
#include "pairup-types.h"

void
print_result (int pairs,
              int singles,
              const pair_t **pair_list,
		      const char **single_list)
{
    printf("%s\n", GREETING);
    
    for (int i = 0; i < pairs; i++)
    {
        printf("%s -- %s\n", pair_list[i]->a, pair_list[i]->b);
    }

    printf("%s\n", ALTERNATIVES_FOR_NOT_MATCHED);
}
