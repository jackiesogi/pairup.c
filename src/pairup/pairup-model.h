#ifndef PAIRUP_MODEL_H
#define PAIRUP_MODEL_H

#include "pairup-types.h"

/*
 * The integer linear program that is handed to the solver.
 *
 * Variables (all binary)
 *   x[a,b,t]  a and b practice together in slot t (a < b, t shared)
 *   y[m]      ensured member m gets at least one partner
 *
 * Constraints (every row is "sum of terms <= upper")
 *   SLOT      sum_b x[m,b,t]       <= 1           each member uses each slot at most once
 *   REQUESTS  sum_{b,t} x[m,b,t]   <= requests_m  no more practices than requested
 *   PARTNER   sum_t x[a,b,t]       <= 1           never the same partner twice
 *   ENSURE    y[m] - sum x[m,.,.]  <= 0           y[m] can only be 1 if m is paired
 *
 * Objective (lexicographic, encoded with weights)
 *   max  E * sum y  +  P * sum x  +  sum_{t not flex} x
 * where P exceeds the largest possible fixed-slot bonus and E exceeds the
 * largest possible value of the other two terms.
 */

typedef enum pairup_col_kind
{
    PAIRUP_COL_PAIR,      /* x[a,b,slot] */
    PAIRUP_COL_ENSURE,    /* y[a]        */
} pairup_col_kind;

typedef struct pairup_col
{
    pairup_col_kind kind;
    int a, b, slot;       /* b and slot are -1 for PAIRUP_COL_ENSURE */
    double cost;
} pairup_col;

typedef enum pairup_row_kind
{
    PAIRUP_ROW_SLOT,      /* member a, slot */
    PAIRUP_ROW_REQUESTS,  /* member a */
    PAIRUP_ROW_PARTNER,   /* members a and b */
    PAIRUP_ROW_ENSURE,    /* member a */
} pairup_row_kind;

typedef struct pairup_row
{
    pairup_row_kind kind;
    int a, b, slot;       /* unused fields are -1 */
    double upper;
} pairup_row;

typedef struct pairup_model
{
    int         n_cols;
    pairup_col *cols;        /* all PAIRUP_COL_PAIR columns come before PAIRUP_COL_ENSURE */
    int         n_rows;
    pairup_row *rows;

    /* Constraint matrix in compressed sparse column form */
    int         nnz;
    int        *col_start;   /* [n_cols + 1] */
    int        *row_index;   /* [nnz] */
    double     *value;       /* [nnz] */

    double      weight_pair;     /* P */
    double      weight_ensure;   /* E */
} pairup_model;

/* Returns 0 on success, or -1 with a message in `err`. */
int
pairup_model_build (const pairup_problem *problem,
                    pairup_model *model,
                    char *err,
                    size_t err_size);

void
pairup_model_free (pairup_model *model);

/*
 * Row-wise copy of the constraint matrix, for printing.
 * Arrays are malloc'ed; free them with `free`. Returns 0 on success.
 */
int
pairup_model_rowwise (const pairup_model *model,
                      int **row_start,
                      int **col_index,
                      double **value);

#endif  // PAIRUP_MODEL_H
