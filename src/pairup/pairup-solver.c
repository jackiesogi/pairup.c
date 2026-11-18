/*
 * Integer linear program solved with HiGHS.
 *
 * Variables
 *   x[a,b,t] in {0,1}   a and b practice together in slot t (a < b, t shared)
 *   y[m]     in {0,1}   ensured member m gets at least one partner
 *
 * Constraints
 *   sum_b x[m,b,t]        <= 1           each member uses each slot at most once
 *   sum_{b,t} x[m,b,t]    <= requests_m  no more practices than requested
 *   sum_t x[a,b,t]        <= 1           never the same partner twice
 *   y[m] - sum x[m,.,.]   <= 0           y[m] can only be 1 if m is paired
 *
 * Objective (lexicographic, encoded with weights)
 *   max  E * sum y  +  P * sum x  +  sum_{t not flex} x
 * where P exceeds the largest possible fixed-slot bonus and E exceeds the
 * largest possible value of the other two terms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interfaces/highs_c_api.h"

#include "pairup-solver.h"
#include "pairup-problem.h"
#include "pairup-formatter.h"

typedef struct
{
    int a, b, slot;
} candidate_t;

/* Row upper bounds; capacity is reserved up front so adding never fails */
typedef struct
{
    HighsInt n_rows;
    double  *upper;
} row_list_t;

static HighsInt
add_row (row_list_t *rows,
         double upper)
{
    rows->upper[rows->n_rows] = upper;
    return rows->n_rows++;
}

static int
pair_cmp (const void *lhs,
          const void *rhs)
{
    const pairup_pair *p = lhs, *q = rhs;
    if (p->slot != q->slot) return p->slot - q->slot;
    if (p->a != q->a) return p->a - q->a;
    return p->b - q->b;
}

/* Defensive check that the solver output obeys every rule */
static int
validate (const pairup_result *r,
          char *err,
          size_t err_size)
{
    const pairup_problem *p = &r->problem;
    int used[PAIRUP_MAX_MEMBERS] = { 0 };
    uint32_t busy[PAIRUP_MAX_MEMBERS] = { 0 };

    for (int k = 0; k < r->n_pairs; k++)
    {
        const pairup_pair *x = &r->pairs[k];
        uint32_t bit = (uint32_t) 1 << x->slot;

        if (!(p->members[x->a].avail & bit) || !(p->members[x->b].avail & bit) ||
            (busy[x->a] & bit) || (busy[x->b] & bit))
        {
            snprintf (err, err_size, "solver returned an invalid time slot");
            return -1;
        }
        busy[x->a] |= bit;
        busy[x->b] |= bit;

        if (++used[x->a] > p->members[x->a].requests || ++used[x->b] > p->members[x->b].requests)
        {
            snprintf (err, err_size, "solver exceeded a member's requests");
            return -1;
        }

        for (int j = 0; j < k; j++)
        {
            if (r->pairs[j].a == x->a && r->pairs[j].b == x->b)
            {
                snprintf (err, err_size, "solver paired the same members twice");
                return -1;
            }
        }
    }
    return 0;
}

static void
fill_singles (pairup_result *r)
{
    int used[PAIRUP_MAX_MEMBERS] = { 0 };

    for (int k = 0; k < r->n_pairs; k++)
    {
        used[r->pairs[k].a]++;
        used[r->pairs[k].b]++;
    }

    r->n_singles = 0;
    for (int m = 0; m < r->problem.n_members; m++)
    {
        if (used[m] < r->problem.members[m].requests)
            r->singles[r->n_singles++] = m;
    }
}

int
pairup_solve (const pairup_problem *problem,
              double time_limit,
              pairup_result *result,
              char *err,
              size_t err_size)
{
    if (&result->problem != problem)
        result->problem = *problem;

    const pairup_problem *p = &result->problem;
    const int n = p->n_members;
    const int S = p->n_slots;
    const int flex = pairup_flex_slot (p);

    result->status = PAIRUP_OPTIMAL;
    result->n_pairs = 0;
    result->n_singles = 0;
    result->total_requests = 0;
    for (int m = 0; m < n; m++)
        result->total_requests += p->members[m].requests;

    /* Enumerate candidate (a, b, slot) triples */
    size_t n_cand = 0;
    for (int a = 0; a < n; a++)
        for (int b = a + 1; b < n; b++)
            n_cand += (size_t) pairup_popcount (p->members[a].avail & p->members[b].avail);

    if (n_cand == 0)
    {
        fill_singles (result);
        debug_printf (DEBUG_INFO, "[ INFO    ] Nobody shares a time slot, nothing to solve.\n");
        return 0;
    }

    int status = -1;
    candidate_t *cand = malloc (n_cand * sizeof(*cand));
    HighsInt *row_slot = malloc ((size_t) n * S * sizeof(HighsInt));
    HighsInt row_cap[PAIRUP_MAX_MEMBERS], row_ens[PAIRUP_MAX_MEMBERS];
    /* Rows: one per (member, slot), per member cap, per pair, per ensured member */
    row_list_t rows = { 0, malloc (((size_t) n * S + 2 * (size_t) n + n_cand) * sizeof(double)) };

    /* At most 7 non-zeros per x column, 1 per y column */
    size_t max_cols = n_cand + (size_t) n;
    HighsInt *starts = malloc (max_cols * sizeof(HighsInt));
    HighsInt *index = malloc ((7 * n_cand + n) * sizeof(HighsInt));
    double *value = malloc ((7 * n_cand + n) * sizeof(double));
    double *cost = malloc (max_cols * sizeof(double));
    double *lower = calloc (max_cols, sizeof(double));
    double *upper = malloc (max_cols * sizeof(double));
    HighsInt *integrality = malloc (max_cols * sizeof(HighsInt));
    double *col_value = NULL, *col_dual = NULL, *row_value = NULL, *row_dual = NULL;
    void *highs = NULL;

    if (!cand || !row_slot || !rows.upper || !starts || !index || !value || !cost || !lower || !upper || !integrality)
    {
        snprintf (err, err_size, "out of memory");
        goto cleanup;
    }

    n_cand = 0;
    for (int a = 0; a < n; a++)
    {
        for (int b = a + 1; b < n; b++)
        {
            uint32_t common = p->members[a].avail & p->members[b].avail;
            for (int t = 0; t < S; t++)
            {
                if ((common >> t) & 1u)
                    cand[n_cand++] = (candidate_t) { a, b, t };
            }
        }
    }

    /* Objective weights, see the comment at the top of this file */
    const double max_pairs = result->total_requests / 2;
    const double P = max_pairs + 1;
    const double E = max_pairs * (P + 1) + 1;

    for (int i = 0; i < n * S; i++) row_slot[i] = -1;
    for (int m = 0; m < n; m++) row_cap[m] = row_ens[m] = -1;

    HighsInt n_cols = 0, nnz = 0, row_pair = -1;
    for (size_t k = 0; k < n_cand; k++)
    {
        const candidate_t *c = &cand[k];
        bool first_of_pair = (k == 0 || cand[k - 1].a != c->a || cand[k - 1].b != c->b);
        const int ends[2] = { c->a, c->b };

        starts[n_cols] = nnz;
        cost[n_cols] = P + (c->slot == flex ? 0 : 1);
        upper[n_cols] = 1;

        for (int e = 0; e < 2; e++)
        {
            int m = ends[e];
            HighsInt *rs = &row_slot[m * S + c->slot];
            if (*rs < 0) *rs = add_row (&rows, 1);
            if (row_cap[m] < 0) row_cap[m] = add_row (&rows, p->members[m].requests);
            index[nnz] = *rs;        value[nnz++] = 1;
            index[nnz] = row_cap[m]; value[nnz++] = 1;

            if (p->members[m].ensured)
            {
                if (row_ens[m] < 0) row_ens[m] = add_row (&rows, 0);
                index[nnz] = row_ens[m]; value[nnz++] = -1;
            }
        }

        /* Same partner twice is only possible when they share 2+ slots */
        if (first_of_pair)
        {
            uint32_t common = p->members[c->a].avail & p->members[c->b].avail;
            row_pair = pairup_popcount (common) >= 2 ? add_row (&rows, 1) : -1;
        }
        if (row_pair >= 0)
        {
            index[nnz] = row_pair; value[nnz++] = 1;
        }

        n_cols++;
    }

    HighsInt n_x = n_cols;
    for (int m = 0; m < n; m++)
    {
        if (row_ens[m] < 0)
            continue;
        starts[n_cols] = nnz;
        cost[n_cols] = E;
        upper[n_cols] = 1;
        index[nnz] = row_ens[m]; value[nnz++] = 1;
        n_cols++;
    }

    for (HighsInt j = 0; j < n_cols; j++)
        integrality[j] = kHighsVarTypeInteger;

    highs = Highs_create ();
    if (!highs)
    {
        snprintf (err, err_size, "failed to create the HiGHS solver");
        goto cleanup;
    }

    Highs_setBoolOptionValue (highs, "output_flag", debug_level >= DEBUG_ALL);
    Highs_setDoubleOptionValue (highs, "mip_rel_gap", 0.0);
    if (time_limit > 0)
        Highs_setDoubleOptionValue (highs, "time_limit", time_limit);

    double inf = Highs_getInfinity (highs);
    double *row_lower = malloc ((size_t) rows.n_rows * sizeof(double));
    if (!row_lower)
    {
        snprintf (err, err_size, "out of memory");
        goto cleanup;
    }
    for (HighsInt i = 0; i < rows.n_rows; i++)
        row_lower[i] = -inf;

    HighsInt rc = Highs_changeObjectiveSense (highs, kHighsObjSenseMaximize);
    if (rc != kHighsStatusError)
        rc = Highs_addRows (highs, rows.n_rows, row_lower, rows.upper, 0, NULL, NULL, NULL);
    if (rc != kHighsStatusError)
        rc = Highs_addCols (highs, n_cols, cost, lower, upper, nnz, starts, index, value);
    if (rc != kHighsStatusError)
        rc = Highs_changeColsIntegralityByRange (highs, 0, n_cols - 1, integrality);
    free (row_lower);
    if (rc == kHighsStatusError)
    {
        snprintf (err, err_size, "failed to build the HiGHS model");
        goto cleanup;
    }

    debug_printf (DEBUG_INFO, "[ INFO    ] Solving ILP with %d variables and %d constraints ...\n",
                  (int) n_cols, (int) rows.n_rows);

    rc = Highs_run (highs);
    HighsInt model_status = Highs_getModelStatus (highs);
    HighsInt primal_status = kHighsSolutionStatusNone;
    Highs_getIntInfoValue (highs, "primal_solution_status", &primal_status);

    if (rc == kHighsStatusError || primal_status != kHighsSolutionStatusFeasible)
    {
        snprintf (err, err_size, "HiGHS failed to find a solution (model status %d)", (int) model_status);
        goto cleanup;
    }
    if (model_status != kHighsModelStatusOptimal)
    {
        result->status = PAIRUP_FEASIBLE;
        debug_printf (DEBUG_WARNING,
                      "[ WARNING ] Solver stopped early (model status %d); the result may not be optimal.\n",
                      (int) model_status);
    }

    debug_printf (DEBUG_INFO, "[ INFO    ] Solver finished in %.3f s, objective = %.0f.\n",
                  Highs_getRunTime (highs), Highs_getObjectiveValue (highs));

    col_value = malloc ((size_t) n_cols * sizeof(double));
    col_dual = malloc ((size_t) n_cols * sizeof(double));
    row_value = malloc ((size_t) rows.n_rows * sizeof(double));
    row_dual = malloc ((size_t) rows.n_rows * sizeof(double));
    if (!col_value || !col_dual || !row_value || !row_dual)
    {
        snprintf (err, err_size, "out of memory");
        goto cleanup;
    }
    Highs_getSolution (highs, col_value, col_dual, row_value, row_dual);

    for (HighsInt j = 0; j < n_x; j++)
    {
        if (col_value[j] > 0.5)
        {
            if (result->n_pairs >= PAIRUP_MAX_PAIRS)
            {
                snprintf (err, err_size, "too many pairs in solution");
                goto cleanup;
            }
            result->pairs[result->n_pairs++] = (pairup_pair) { cand[j].a, cand[j].b, cand[j].slot };
        }
    }
    qsort (result->pairs, (size_t) result->n_pairs, sizeof(pairup_pair), pair_cmp);

    if (validate (result, err, err_size) != 0)
        goto cleanup;

    fill_singles (result);
    status = 0;

cleanup:
    if (highs) Highs_destroy (highs);
    free (cand);
    free (row_slot);
    free (rows.upper);
    free (starts);
    free (index);
    free (value);
    free (cost);
    free (lower);
    free (upper);
    free (integrality);
    free (col_value);
    free (col_dual);
    free (row_value);
    free (row_dual);
    if (status != 0)
        result->n_pairs = result->n_singles = 0;
    return status;
}
