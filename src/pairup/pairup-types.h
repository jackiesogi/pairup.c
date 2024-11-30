#ifndef PAIRUP_TYPES_H
#define PAIRUP_TYPES_H

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

/* Maximum length of the name */
#define  MAX_NAME_LEN     4096
// #define  MAX_MATCHES_LEN  4096
#define  _MAX_MATCHES_LEN  64 
#define  _MAX_MEMBERS_LEN  64 

/* Boundaries for the sheet that containing real data */
#define  _FILED_COL_NAME   0 
#define  _FILED_ROW_START  1
#define  _FILED_COL_START  2 
#define  _FILED_COL_END    16

/********************************  Types aliasess  ************************************/

/* Slot */
typedef int slot;

/* Member */
typedef struct member member_t;
typedef struct member member;  // Recommended
typedef struct member memb;

/* Pair */
typedef struct pair pair_t;
typedef struct pair pair;  // Recommended

/* Relation */
typedef struct relation relation_t;
typedef struct relation relation;

/* Relation graph */
typedef struct relation_graph relation_graph_t;
typedef struct relation_graph relation_graph;
typedef struct relation_graph graph_t;
typedef struct relation_graph graph;  // Recommended

/* Pair result */
typedef struct pair_result pair_result_t;
typedef struct pair_result pair_result;  // Recommended
typedef struct pair_result result_t;
typedef struct pair_result result;

/* Pairup algorithm */
typedef pair_result *
(*pairup_internal) (relation_graph *today,
                    member_t *member_list[]);

typedef struct pairup_algorithm pairup_algorithm_t;
typedef struct pairup_algorithm pairup_algorithm;  // Recommended
typedef struct pairup_algorithm algorithm_t;
typedef struct pairup_algorithm algorithm;

/**********************************  Data types  **************************************/

/* Member's info and their willingness to practice on that day */
struct member
{
    int    id;                 // Row number on the google sheet
    char   name[MAX_NAME_LEN]; // Name of the member
    size_t requests;           // Number of requested practices (0, 1 or 2)
    size_t availability;       // Number of slots available on that day
    slot   earliest_slot;      // Earliest time slot on the sheet
};

/* A successful pair will contain two members and a matched time slot */
struct pair
{
    member *a;           // Member A's information
    member *b;           // Member B's information
    slot time;           // Available time slot for a and b
};

/* A relation is a member and his/her pairing candidates */
/* Technically, it's a row in adjacency list representation (See next struct) */
struct relation
{
    size_t count;                                 // Number of candidates for this member
    member *candidates[_MAX_MATCHES_LEN];         // Candidates for this member
    slot matched_slot[_MAX_MATCHES_LEN];          // Matched time slot for this member

    size_t availability;                          // Number of slots available on that day
    slot available_slot[_MAX_MATCHES_LEN];        // Available time slot for this member
};

/* A graph represents today's matching relations between members */
/* Technically, it's a graph data type using adjacency list representation */
/* Adjacency list will be more convinient when using DFS or BFS to walk through the graph */
struct relation_graph
{
    size_t count;                                  // Number of relations
    relation *relations[_MAX_MATCHES_LEN];  // Relations
};

/* A pair result contains the successful pairs and the remaining singles */
struct pair_result
{
    size_t member;
    size_t singles;
    size_t pairs;
    size_t total_requests;
    slot matched_slot[_MAX_MATCHES_LEN];
    struct member *member_list[_MAX_MATCHES_LEN];
    struct member *single_list[_MAX_MATCHES_LEN];
    pair *pair_list[_MAX_MATCHES_LEN];
};

struct pairup_algorithm
{
    const char *name;
    const pairup_internal algorithm;
};

/* For example: */
// relation_graph today;           --> A graph named 'today'
// today.relations[i].name;          --> Name of the i-th member in the graph
// today.relations[i].candidates[j]  --> The j-th candidate of the i-th member

struct pairup_options
{
    bool show_csv;
    bool generate_graph;
    char graph_output[1024];
    bool ensure;
    char ensure_member[1024];
    bool priority;
    char priority_func[1024];
    int debug_level;
};

/********************************  Number of practices  *********************************/

extern const char *zero_sign[];

extern const char *once_sign[];

extern const char *twice_sign[];

/**********************************  Helper functions  **********************************/

bool is_zero (const char *sign);

bool is_once (const char *sign);

bool is_twice (const char *sign);

bool is_available (const char *sign);

/****************************  Allocator and Deallocator  ********************************/

void
pairup_options_init (struct pairup_options *x);

member_t *
_new_member (void);

void
_free_member (member_t *member);

pair *
_new_pair (void);

void
_free_pair (pair *pair);

relation *
_new_relation (void);

void
_free_relation (relation *relation);

relation_graph *
_new_relation_graph (void);

void 
_free_relation_graph (relation_graph *today);

pair_result *
_new_pair_result (int member,
                  int pairs,
                  int singles);

void
_free_pair_result (pair_result *result);

#endif  // PAIRUP_TYPES_H

