#ifndef PAIRUP_TYPES_H
#define PAIRUP_TYPES_H

#include <stdlib.h>
#include <stdbool.h>

/* Maximum length of the name */
#define  MAX_NAME_LEN     4096
#define  MAX_MATCHES_LEN  4096

/* Boundaries for the sheet that containing real data */
#define  _FILED_COL_NAME   0 
#define  _FILED_ROW_START  1
#define  _FILED_COL_START  2 
#define  _FILED_COL_END    16
#define  _MAX_MATCHES_LEN  64 

struct pair
{
    char a[MAX_NAME_LEN];
    char b[MAX_NAME_LEN];
};

struct pair_result
{
    int member;
    int pairs;
    int singles;
    
    char member_list[_MAX_MATCHES_LEN][MAX_NAME_LEN];
    struct pair pair_list[_MAX_MATCHES_LEN];
    char single_list[_MAX_MATCHES_LEN][MAX_NAME_LEN];
};

typedef struct pair pair_t;
typedef struct pair_result pair_result_t;

/* Adjacency matrix for representing the availability graph */
struct adjmatrix_row
{
    int idx;
    int count;  // Number of matches
    int remain;  // Number of practice time
    int earliest_time;
    int read_order;
    int availability;
    int matches[_MAX_MATCHES_LEN];
};

struct adjmatrix
{
    int rows;
    char *name_map[_MAX_MATCHES_LEN];
    struct adjmatrix_row **members;
};

typedef struct adjmatrix_row adjmatrix_row_t;
typedef struct adjmatrix adjmatrix_t;
// For example:
// graph->members[i].matches[j]
// graph->members[i].availability

adjmatrix_t *
_new_matches_graph (void);

void 
_free_matches_graph (adjmatrix_t *graph);

pair_result_t *
_new_pair_result (int member,
                  int pairs,
                  int singles);

void
_free_pair_result (pair_result_t *result);

extern const char *once_sign[];

extern const char *twice_sign[];

bool is_zero (const char *sign);

bool is_once (const char *sign);

bool is_twice (const char *sign);

bool is_available (const char *sign);

#endif  // PAIRUP_TYPES_H

