#define BUILD_LIB_PAIRUP 1

#if defined (BUILD_LIB_PAIRUP)

#ifndef LIB_PAIRUP_H
#define LIB_PAIRUP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pairup/pairup-types.h"
#include "pairup/pairup-algorithm.h"
#include "pairup/pairup-formatter.h"
#include "rw-csv.h"

/* Essential Types */
typedef sheet_t sheet;
typedef result_t result;
typedef struct pairup_options options;

#define PairUP_FreeJSONObject free_result_json_object
#define PairUP_FreeJSONString free_result_json_string

/* Read CSV source data */
sheet PairUP_Read(const char *path);

void PairUP_DefaultOption (options *opt);

result *PairUP_Generate(sheet *worksheet, options *opt, uint32_t seed);

cJSON *PairUP_ConvertToJSON(sheet *worksheet, result *result);

char *PairUP_ConvertToString(sheet *worksheet, result *result);

#ifdef __cplusplus
}
#endif
#endif /* LIB_PAIRUP_H */
#endif
