#ifndef PAIRUP_FORMATTER_H
#define PAIRUP_FORMATTER_H

#include "pairup-types.h"
#include "rw-csv.h"

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

/* Currently not used */
#define DEBUG_LOG_FORMATTED_MESSAGE(format, args) \
    do { \
        if (format != NULL) { \
            vprintf (format, args); \
            printf ("\n"); \
        } \
    } while (0)

void
print_worksheet (sheet *worksheet);

void
print_result (sheet *worksheet,
              pair_result *result);

void
generate_graph_output_image (sheet *worksheet,
                             const char *filename);

void
print_graph_to_file (sheet *worksheet,
                     const char *filename);
void
print_digraph_to_file (sheet *worksheet,
                       const char *filename);

void
display_summary (pair_result *result);

void
display_graph (void *graph);

/* Debug levels */
// None -> Error -> Warning -> Summary -> Info -> All
#define DEBUG_NONE     0
#define DEBUG_ERROR    1
#define DEBUG_WARNING  2
#define DEBUG_SUMMARY  3
#define DEBUG_INFO     4
#define DEBUG_ALL      5

/* Macros to log messages */
#define log_message(file, fmt, ...) \
    do { \
        fprintf (file, fmt, ##__VA_ARGS__); \
    } while (0)


/* Macros to log messages or perform actions based on debug level */
#define debug_printf(level, fmt, ...) \
    do_if_debug_level_is_greater(level, NULL, NULL, fmt, ##__VA_ARGS__)

#define debug_action(level, callback, context) \
    do_if_debug_level_is_greater(level, callback, context, NULL)

#define debug_task(level, callback, context, fmt, ...) \
    do_if_debug_level_is_greater(level, callback, context, fmt, ##__VA_ARGS__)

extern int debug_level;

/* Callback function type */
typedef void (*callback)(void *context);

/* Macro for output string */
#define RESULT_SUMMARY_MESSAGE "Displaying result statistics...\n"
#define GRAPH_STATS_MESSAGE    "Displaying relation graph...\n"

/* Function to handle conditional logging or callback execution based on debug level */
void do_if_debug_level_is_greater (int level,
                                  callback fptr,
                                  void *context,
                                  const char *fmt, ...);

#endif  // PAIRUP_FORMATTER_H
