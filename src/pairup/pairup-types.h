#ifndef PAIRUP_TYPES_H
#define PAIRUP_TYPES_H

#include <stddef.h>
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
#define  _MAX_MEMBERS_LEN  64 

/*********************  New data types  *************************/
struct member
{
    int    id;                     // Row number on the google sheet
    char   name[MAX_NAME_LEN];     // Name of the member
    size_t requests;               // Number of requested practice time
    size_t availability;           // Number of time slots available
    int    earliest_slot;          // Earliest time slot on the sheet
};

typedef struct member member_t;

struct pair
{
    struct member *a;    // Member A
    struct member *b;    // Member B
    int time;
};

typedef struct pair pair_t;

struct relation
{
    size_t count;                                 // Number of candidates for this member
    int priority;                                 // Priority of this member
    int matched_slot[_MAX_MATCHES_LEN];           // Matched time slot for this member
    struct member *candidates[_MAX_MATCHES_LEN];  // Candidates for this member
    
    int available_slot_count;                     // Number of available time slot for this member
    int available_slot[_MAX_MATCHES_LEN];         // Available time slot for this member
};

typedef struct relation relation_t;

struct relation_graph
{
    size_t count;   // Number of relations
    struct relation *relations[_MAX_MATCHES_LEN];  // Relations
};

typedef struct relation_graph relation_graph_t;

// For example:
// today.relations[i].name
// today.relations[i].candidates[j]

/*********************  New data types  *************************/

struct pair_result
{
    int member;
    int singles;
    int pairs;
    int total_requests;
    
    int matched_slot[_MAX_MATCHES_LEN];
    member_t *member_list[_MAX_MATCHES_LEN];
    member_t *single_list[_MAX_MATCHES_LEN];
    struct pair *pair_list[_MAX_MATCHES_LEN];
};

typedef struct pair pair_t;
typedef struct pair_result pair_result_t;

/* Allocator and deallocator for the data types */
member_t *
_new_member (void);

void
_free_member (member_t *member);

pair_t *
_new_pair (void);

void
_free_pair (pair_t *pair);

relation_t *
_new_relation (void);

void
_free_relation (relation_t *relation);

relation_graph_t *
_new_relation_graph (void);

void 
_free_relation_graph (relation_graph_t *today);

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

