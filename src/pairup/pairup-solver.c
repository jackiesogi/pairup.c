/*
 * Solve the pairing model (see pairup-model.h) with HiGHS.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interfaces/highs_c_api.h"

#include "pairup-solver.h"
#include "pairup-problem.h"
#include "pairup-model.h"
#include "pairup-formatter.h"

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

    result->status = PAIRUP_OPTIMAL;
    result->n_pairs = 0;
    result->n_singles = 0;
    result->total_requests = 0;
    for (int m = 0; m < p->n_members; m++)
        result->total_requests += p->members[m].requests;

    pairup_model model;
    if (pairup_model_build (p, &model, err, err_size) != 0)
        return -1;

    if (model.n_cols == 0)
    {
        pairup_model_free (&model);
        fill_singles (result);
        debug_printf (DEBUG_INFO, "[ INFO    ] Nobody shares a time slot, nothing to solve.\n");
        return 0;
    }

    int status = -1;
    const HighsInt n_cols = model.n_cols, n_rows = model.n_rows, nnz = model.nnz;
    HighsInt *starts = malloc ((size_t) n_cols * sizeof(HighsInt));
    HighsInt *index = malloc ((size_t) nnz * sizeof(HighsInt));
    HighsInt *integrality = malloc ((size_t) n_cols * sizeof(HighsInt));
    double *cost = malloc ((size_t) n_cols * sizeof(double));
    double *lower = calloc ((size_t) n_cols, sizeof(double));
    double *upper = malloc ((size_t) n_cols * sizeof(double));
    double *row_lower = malloc ((size_t) n_rows * sizeof(double));
    double *row_upper = malloc ((size_t) n_rows * sizeof(double));
    double *col_value = malloc ((size_t) n_cols * sizeof(double));
    double *col_dual = malloc ((size_t) n_cols * sizeof(double));
    double *row_value = malloc ((size_t) n_rows * sizeof(double));
    double *row_dual = malloc ((size_t) n_rows * sizeof(double));
    void *highs = NULL;

    if (!starts || !index || !integrality || !cost || !lower || !upper || !row_lower ||
        !row_upper || !col_value || !col_dual || !row_value || !row_dual)
    {
        snprintf (err, err_size, "out of memory");
        goto cleanup;
    }

    /* Copy into HighsInt arrays: HighsInt may be 64-bit depending on the build */
    for (HighsInt j = 0; j < n_cols; j++)
    {
        starts[j] = model.col_start[j];
        cost[j] = model.cols[j].cost;
        upper[j] = 1;
        integrality[j] = kHighsVarTypeInteger;
    }
    for (HighsInt k = 0; k < nnz; k++)
        index[k] = model.row_index[k];

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
    for (HighsInt i = 0; i < n_rows; i++)
    {
        row_lower[i] = -inf;
        row_upper[i] = model.rows[i].upper;
    }

    HighsInt rc = Highs_changeObjectiveSense (highs, kHighsObjSenseMaximize);
    if (rc != kHighsStatusError)
        rc = Highs_addRows (highs, n_rows, row_lower, row_upper, 0, NULL, NULL, NULL);
    if (rc != kHighsStatusError)
        rc = Highs_addCols (highs, n_cols, cost, lower, upper, nnz, starts, index, model.value);
    if (rc != kHighsStatusError)
        rc = Highs_changeColsIntegralityByRange (highs, 0, n_cols - 1, integrality);
    if (rc == kHighsStatusError)
    {
        snprintf (err, err_size, "failed to build the HiGHS model");
        goto cleanup;
    }

    debug_printf (DEBUG_INFO, "[ INFO    ] Solving ILP with %d variables and %d constraints ...\n",
                  (int) n_cols, (int) n_rows);

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

    Highs_getSolution (highs, col_value, col_dual, row_value, row_dual);

    for (HighsInt j = 0; j < n_cols; j++)
    {
        const pairup_col *c = &model.cols[j];
        if (c->kind != PAIRUP_COL_PAIR || col_value[j] <= 0.5)
            continue;
        if (result->n_pairs >= PAIRUP_MAX_PAIRS)
        {
            snprintf (err, err_size, "too many pairs in solution");
            goto cleanup;
        }
        result->pairs[result->n_pairs++] = (pairup_pair) { c->a, c->b, c->slot };
    }
    qsort (result->pairs, (size_t) result->n_pairs, sizeof(pairup_pair), pair_cmp);

    if (validate (result, err, err_size) != 0)
        goto cleanup;

    fill_singles (result);
    status = 0;

cleanup:
    if (highs) Highs_destroy (highs);
    pairup_model_free (&model);
    free (starts);
    free (index);
    free (integrality);
    free (cost);
    free (lower);
    free (upper);
    free (row_lower);
    free (row_upper);
    free (col_value);
    free (col_dual);
    free (row_value);
    free (row_dual);
    if (status != 0)
        result->n_pairs = result->n_singles = 0;
    return status;
}
