#ifndef LIB_PAIRUP_H
#define LIB_PAIRUP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "pairup/pairup.h"
#include "cJSON.h"

/* Essential Types */
typedef pairup_result          result;
typedef struct pairup_options  options;

#define PairUP_FreeJSONObject free_result_json_object
#define PairUP_FreeJSONString free_result_json_string

/* Read CSV source data */
sheet PairUP_Read (const char *path);

void PairUP_DefaultOption (options *opt);

/*
 * Compute the optimal pairing. `opt` may be NULL for defaults. A non-zero
 * `seed` shuffles the rows first, which changes how ties are broken.
 * Returns NULL on error; free the result with `PairUP_Free`.
 */
result *PairUP_Generate (sheet *worksheet, const options *opt, uint32_t seed);

/* The result is self-contained: no worksheet is needed to render it */
cJSON *PairUP_ConvertToJSON (const result *r);
char *PairUP_ConvertToString (const result *r);

void PairUP_Free (result *r);

#ifdef __cplusplus
}
#endif

#endif /* LIB_PAIRUP_H */
