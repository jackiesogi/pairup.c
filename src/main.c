#include <stdio.h>
#include <stdlib.h>

#include "pairup/pairup.h"
#include "rw-csv.h"
#include "pairup/pairup.h"

int
main (int argc, char *argv[])
{
    sheet_t sheet = read_csv("data.csv");

    printf("rows: %d\n", sheet.rows);
    printf("cols: %d\n", sheet.cols);
    printf("path: %s\n", sheet.path);

    char buf[1024];

    for (int i = 1; i <= sheet.rows; i++) {
        for (int j = 1; j <= sheet.cols; j++) {
            get_cell (&sheet, i, j, buf, sizeof(buf));
            printf("%s ", buf);
        }
        printf("\n");
    }

    printf("Below are the pairing results:\n");

    pair_result_t *result = pairup (&sheet);

    print_result (result->pairs, result->singles, (const pair_t **)result->pair_list, (const char **)result->single_list);

    return 0;
}
