#include <stdio.h>
#include <time.h>
#include "pairup/pairup-algorithm.h"
#include "pairup/pairup-formatter.h"
#include "pairup/pairup-types.h"
#include "rw-csv.h"
#include "libpairup.h"
#include "cJSON.h"

/* Read CSV source data */
sheet PairUP_Read(const char *path) {
    return read_csv(path);
}

void PairUP_DefaultOption(options *opt) {
    pairup_options_init (opt);
}

result *PairUP_Generate(sheet *worksheet, options *opt, uint32_t seed) {
    if (opt == NULL)
        PairUP_DefaultOption (opt);

    if (seed != 0)
        shuffle_worksheet (worksheet, seed);

    return __pairup__ (worksheet, opt);
}

cJSON *PairUP_ConvertToJSON(sheet *worksheet, result *result) {
    return init_result_json_object (worksheet, result);
}

char *PairUP_ConvertToString(sheet *worksheet, result *result) {
    cJSON *root = init_result_json_object(worksheet, result);
    return get_result_json_string(root);

}
