#include <stdio.h>

#include "libpairup.h"

sheet PairUP_Read (const char *path)
{
    return read_csv (path);
}

void PairUP_DefaultOption (options *opt)
{
    pairup_options_init (opt);
}

result *PairUP_Generate (sheet *worksheet, const options *opt, uint32_t seed)
{
    char err[256];

    if (seed != 0)
        shuffle_worksheet (worksheet, seed);

    result *r = pairup_run (worksheet, opt, err, sizeof(err));
    if (!r)
        debug_printf (DEBUG_ERROR, "[ ERROR   ] %s\n", err);
    return r;
}

cJSON *PairUP_ConvertToJSON (const result *r)
{
    return init_result_json_object (r);
}

char *PairUP_ConvertToString (const result *r)
{
    cJSON *root = init_result_json_object (r);
    char *str = get_result_json_string (root);
    free_result_json_object (root);
    return str;
}

void PairUP_Free (result *r)
{
    pairup_result_free (r);
}
