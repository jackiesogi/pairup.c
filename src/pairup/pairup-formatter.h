#ifndef PAIRUP_FORMATTER_H
#define PAIRUP_FORMATTER_H

#include "pairup-types.h"
#include "rw-csv.h"

#define GREETING "Enjoy the chat :D"

#define ALTERNATIVES_FOR_NOT_MATCHED " \n\
you can choose to \n\
1) take a day off (count out) \n\
2) requesting for partners \n\
3) leave a 4-minute up voice message and answer questions \n\
related to weekly topic. ONLY on Monday can talk about \n\
your last weekend or sharing something interesting."

void
print_worksheet (sheet_t *worksheet);

void
print_result (sheet_t *worksheet,
              pair_result_t *result);

void
generate_graph_output_image (sheet_t *worksheet,
                             const char *filename);

void
print_graph_to_file (sheet_t *worksheet,
                     const char *filename);
void
print_digraph_to_file (sheet_t *worksheet,
                     const char *filename);

void
print_result_statistics (pair_result_t *result);

/* Debug levels */
#define DEBUG_NONE     0
#define DEBUG_ERROR    1
#define DEBUG_WARNING  2
#define DEBUG_SUMMARY  3
#define DEBUG_INFO     4
#define DEBUG_ALL      5

extern int debug_level;

typedef void (*debug_fn)(void *context);

void
log_message (int level,
             debug_fn fptr,
             void *context);

#endif  // PAIRUP_FORMATTER_H
