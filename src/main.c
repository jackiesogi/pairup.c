#include <stdio.h>
#include <stdlib.h>

#include "pairup/pairup.h"
#include "rw-csv.h"

int
main(int argc, char *argv[])
{
    sheet_t sheet = read_csv ("data.csv");

    printf("rows: %d\n", sheet.rows);
    printf("cols: %d\n", sheet.cols);
    printf("path: %s\n", sheet.path);

    char buf[1024];
    for (int i = 1; i <= sheet.rows; i++)
    {
        for (int j = 1; j <= sheet.cols; j++)
        {
            get_cell_from_one(&sheet, i, j, buf, sizeof(buf));
            printf("%s ", buf);
        }
        printf("\n");
    }

    printf("\nBelow are the pairing results:\n");

    pair_result_t *result = pairup(&sheet);
    if (!result)
    {
        fprintf(stderr, "Pairing operation failed.\n");
        return 1;
    }

    print_result (&sheet, result);

    return 0;
}
