#include <stdio.h>
#include <stdlib.h>

#include "pairup/pairup.h"
#include "rw-csv.h"

int
main(int argc, char *argv[])
{
    sheet_t worksheet = read_csv ("data.csv");

    printf("rows: %d\n", worksheet.rows);
    printf("cols: %d\n", worksheet.cols);
    printf("path: %s\n", worksheet.path);

    printf("\nBelow are the pairing results:\n");

    pair_result_t *result = pairup(&worksheet);

    print_result (&worksheet, result);

    return 0;
}


    // char buf[1024];
    // for (int i = 1; i <= worksheet.rows; i++)
    // {
    //     for (int j = 1; j <= sheet.cols; j++)
    //     {
    //         get_cell_from_one(&worksheet, i, j, buf, sizeof(buf));
    //         printf("%-9s ", buf);
    //     }
    //     printf("\n");
    // }
