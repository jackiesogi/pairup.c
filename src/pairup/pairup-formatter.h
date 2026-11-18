#ifndef PAIRUP_FORMATTER_H
#define PAIRUP_FORMATTER_H

#include "pairup-types.h"
#include "rw-csv.h"
#include "cJSON.h"

#define GREETING "Enjoy the chat with your partner!💬"

#define NOPAIRS_SUGGESTION "Dear all, there were no successful pairs today.💤 \
Maybe you can review your available time again!"

#define ALTERNATIVES_FOR_NOT_MATCHED " \n\
You can choose to \n\
👉 Take a day off (count out) \n\
👉 Requesting for partners \n\
👉 Leave a 4-minute up voice message and answer questions \
related to weekly topic. ONLY on Monday can talk about \
your last weekend or sharing something interesting."

/* Name of the algorithm reported in the JSON output */
#define PAIRUP_ALGORITHM_NAME "HIGHS_ILP"

/* Raw worksheet (--show-csv) */
void
print_worksheet (sheet *worksheet);

/* Pairing result as the chat message */
void
print_result (const pairup_result *result);

/* Pairing result as JSON; free with `free_result_json_object` */
cJSON *
init_result_json_object (const pairup_result *result);

void free_result_json_object (cJSON *root);
void free_result_json_string (char *str);
#define get_result_json_string cJSON_Print

/* Relation graph (--graph): who can practice with whom, plus the ILP model */
void
generate_graph_output_image (const pairup_problem *problem,
                             const char *filename);

/* Writes the DOT source, including every constraint of the ILP. Returns 0 on success. */
int
print_graph_to_file (const pairup_problem *problem,
                     const char *filename);

/* Debug helpers, used as `callback`s */
void
display_summary (const pairup_result *result);

void
display_graph (const pairup_problem *problem);

/* Debug levels */
// None -> Error -> Warning -> Summary -> Info -> All
#define DEBUG_NONE     0
#define DEBUG_ERROR    1
#define DEBUG_WARNING  2
#define DEBUG_SUMMARY  3
#define DEBUG_INFO     4
#define DEBUG_ALL      5

/* Macros to log messages or perform actions based on debug level */
#define debug_printf(level, fmt, ...) \
    do_if_debug_level_is_greater(level, NULL, NULL, fmt, ##__VA_ARGS__)

#define debug_action(level, callback, context) \
    do_if_debug_level_is_greater(level, callback, context, NULL)

extern int debug_level;

/* Callback function type */
typedef void (*callback)(void *context);

/* Function to handle conditional logging or callback execution based on debug level */
void do_if_debug_level_is_greater (int level,
                                  callback fptr,
                                  void *context,
                                  const char *fmt, ...);

#endif  // PAIRUP_FORMATTER_H
