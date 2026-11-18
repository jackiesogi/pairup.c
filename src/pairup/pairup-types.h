#ifndef PAIRUP_TYPES_H
#define PAIRUP_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*********************************  Limits  ******************************************/

#define  PAIRUP_MAX_MEMBERS   256
#define  PAIRUP_MAX_SLOTS     32     /* availability is stored as a 32-bit mask */
#define  PAIRUP_MAX_NAME      256
#define  PAIRUP_MAX_LABEL     32
#define  PAIRUP_MAX_RANGE     64     /* "1900~2030" style merged range label */
#define  PAIRUP_MAX_ENSURE    64
/* Each member requests at most two practices, so pairs <= members */
#define  PAIRUP_MAX_PAIRS     PAIRUP_MAX_MEMBERS

/****************************  Google Sheet layout  **********************************/
/*
 * Row 0 is the header. From FIELD_COL_START on, the column whose header is
 * "FLEX" is the flex column and every other non-empty header (e.g. "1900~1930")
 * is a time slot. Member rows start at FIELD_ROW_START and the last row of the
 * sheet is a summary row.
 */
#define  FIELD_COL_NAME   0
#define  FIELD_ROW_START  1
#define  FIELD_COL_START  2
#define  FIELD_COL_END    16   /* only used to format `--show-csv` output */

/*********************************  Problem  *****************************************/

/* A member who filled in at least one usable cell on the sheet */
typedef struct pairup_member
{
    char     name[PAIRUP_MAX_NAME];
    int      row;          /* row on the (possibly shuffled) sheet */
    int      requests;     /* requested practices today: 1 or 2 */
    uint32_t avail;        /* bit t set -> available in problem slot t */
    bool     ensured;      /* listed with --ensure */
} pairup_member;

typedef struct pairup_slot
{
    char label[PAIRUP_MAX_LABEL];  /* "1900~1930", or "flex" for the flex slot */
    int  col;                      /* column on the sheet */
    bool is_flex;
} pairup_slot;

/* Everything the solver needs, detached from the CSV representation */
typedef struct pairup_problem
{
    int           n_members;
    pairup_member members[PAIRUP_MAX_MEMBERS];
    int           n_slots;
    pairup_slot   slots[PAIRUP_MAX_SLOTS];
} pairup_problem;

/**********************************  Result  *****************************************/

typedef struct pairup_pair
{
    int a;      /* index into problem.members */
    int b;      /* index into problem.members */
    int slot;   /* index into problem.slots */
} pairup_pair;

typedef enum pairup_status
{
    PAIRUP_OPTIMAL = 0,    /* proven maximum */
    PAIRUP_FEASIBLE,       /* time limit hit; best solution found so far */
} pairup_status;

/* The result owns a copy of the problem, so it can be rendered on its own */
typedef struct pairup_result
{
    pairup_problem problem;
    pairup_status  status;
    int            total_requests;
    int            n_pairs;
    pairup_pair    pairs[PAIRUP_MAX_PAIRS];
    int            n_singles;                  /* members with unmet requests */
    int            singles[PAIRUP_MAX_MEMBERS];
} pairup_result;

/* Portable popcount (MSVC has no __builtin_popcount) */
static inline int
pairup_popcount (uint32_t x)
{
    int n = 0;
    for (; x; x &= x - 1)
        n++;
    return n;
}

/*********************************  Options  *****************************************/

struct pairup_options
{
    bool   show_csv;
    bool   json_output;
    bool   generate_graph;
    char   graph_output[1024];
    bool   enable_flex;
    size_t n_ensure;
    char   ensure[PAIRUP_MAX_ENSURE][PAIRUP_MAX_NAME];
    double time_limit;     /* seconds; the solver returns its best solution when hit */
};

void
pairup_options_init (struct pairup_options *x);

/* Returns false if the list is full */
bool
pairup_options_add_ensure (struct pairup_options *x,
                           const char *name);

/****************************  Cell value helpers  ***********************************/

extern const char *zero_sign[];
extern const char *once_sign[];
extern const char *twice_sign[];

bool is_zero (const char *sign);
bool is_once (const char *sign);
bool is_twice (const char *sign);
bool is_available (const char *sign);

#endif  // PAIRUP_TYPES_H
