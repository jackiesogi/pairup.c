#include <stdio.h>
#include <stdlib.h>

#include "pairup/pairup-formatter.h"
#include "pairup/pairup.h"
#include "rw-csv.h"

int
main(int argc, char *argv[])
{
    sheet_t worksheet = read_csv ("data.csv");
    print_worksheet(&worksheet);

    pair_result_t *result = pairup(&worksheet);

    print_result (&worksheet, result);

    return 0;
}
