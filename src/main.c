#include <stdio.h>
#include <stdlib.h>

#include "pairup/pairup-formatter.h"
#include "pairup/pairup.h"
#include "rw-csv.h"

int
main(int argc, char *argv[])
{
    sheet_t worksheet = read_csv ("data.csv");

    printf("rows: %d\n", worksheet.rows);
    printf("cols: %d\n", worksheet.cols);
    printf("path: %s\n", worksheet.path);

    print_worksheet(&worksheet);

    printf("\nBelow are the pairing results:\n");

    pair_result_t *result = pairup(&worksheet);

    print_result (&worksheet, result);

    return 0;
}
