#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pairup-model.h"
#include "pairup-problem.h"

static int
add_row (pairup_model *m,
         pairup_row_kind kind,
         int a,
         int b,
         int slot,
         double upper)
{
    m->rows[m->n_rows] = (pairup_row) { kind, a, b, slot, upper };
    return m->n_rows++;
}

static void
add_entry (pairup_model *m,
           int row,
           double value)
{
    m->row_index[m->nnz] = row;
    m->value[m->nnz] = value;
    m->nnz++;
}

int
pairup_model_build (const pairup_problem *p,
                    pairup_model *m,
                    char *err,
                    size_t err_size)
{
    const int n = p->n_members;
    const int S = p->n_slots;
    const int flex = pairup_flex_slot (p);

    memset (m, 0, sizeof(*m));

    size_t n_pair_cols = 0;
    int total_requests = 0;
    for (int a = 0; a < n; a++)
    {
        total_requests += p->members[a].requests;
        for (int b = a + 1; b < n; b++)
            n_pair_cols += (size_t) pairup_popcount (p->members[a].avail & p->members[b].avail);
    }

    /* Objective weights, see the comment in pairup-model.h */
    const double max_pairs = total_requests / 2;
    m->weight_pair = max_pairs + 1;
    m->weight_ensure = max_pairs * (m->weight_pair + 1) + 1;

    /* Upper bounds on sizes: x has <= 7 non-zeros, y has 1 */
    size_t max_cols = n_pair_cols + (size_t) n;
    size_t max_rows = (size_t) n * S + 2 * (size_t) n + n_pair_cols;
    size_t max_nnz = 7 * n_pair_cols + (size_t) n;

    int *row_slot = malloc (((size_t) n * S + 1) * sizeof(int));
    int row_cap[PAIRUP_MAX_MEMBERS], row_ens[PAIRUP_MAX_MEMBERS];

    m->cols = malloc ((max_cols + 1) * sizeof(*m->cols));
    m->rows = malloc ((max_rows + 1) * sizeof(*m->rows));
    m->col_start = malloc ((max_cols + 1) * sizeof(int));
    m->row_index = malloc ((max_nnz + 1) * sizeof(int));
    m->value = malloc ((max_nnz + 1) * sizeof(double));

    if (!row_slot || !m->cols || !m->rows || !m->col_start || !m->row_index || !m->value)
    {
        free (row_slot);
        pairup_model_free (m);
        snprintf (err, err_size, "out of memory");
        return -1;
    }

    for (int i = 0; i < n * S; i++) row_slot[i] = -1;
    for (int i = 0; i < n; i++) row_cap[i] = row_ens[i] = -1;

    /* x[a,b,t] columns, created in (a, b, t) order */
    for (int a = 0; a < n; a++)
    {
        for (int b = a + 1; b < n; b++)
        {
            uint32_t common = p->members[a].avail & p->members[b].avail;
            if (!common)
                continue;

            /* Meeting the same partner twice needs 2+ shared slots */
            int row_partner = pairup_popcount (common) >= 2
                ? add_row (m, PAIRUP_ROW_PARTNER, a, b, -1, 1)
                : -1;

            for (int t = 0; t < S; t++)
            {
                if (!((common >> t) & 1u))
                    continue;

                int j = m->n_cols++;
                m->cols[j] = (pairup_col) {
                    PAIRUP_COL_PAIR, a, b, t,
                    m->weight_pair + (t == flex ? 0 : 1)
                };
                m->col_start[j] = m->nnz;

                const int ends[2] = { a, b };
                for (int e = 0; e < 2; e++)
                {
                    int x = ends[e];
                    int *rs = &row_slot[x * S + t];
                    if (*rs < 0)
                        *rs = add_row (m, PAIRUP_ROW_SLOT, x, -1, t, 1);
                    if (row_cap[x] < 0)
                        row_cap[x] = add_row (m, PAIRUP_ROW_REQUESTS, x, -1, -1, p->members[x].requests);
                    add_entry (m, *rs, 1);
                    add_entry (m, row_cap[x], 1);

                    if (p->members[x].ensured)
                    {
                        if (row_ens[x] < 0)
                            row_ens[x] = add_row (m, PAIRUP_ROW_ENSURE, x, -1, -1, 0);
                        add_entry (m, row_ens[x], -1);
                    }
                }

                if (row_partner >= 0)
                    add_entry (m, row_partner, 1);
            }
        }
    }

    /* y[m] columns for ensured members who can be paired at all */
    for (int x = 0; x < n; x++)
    {
        if (row_ens[x] < 0)
            continue;
        int j = m->n_cols++;
        m->cols[j] = (pairup_col) { PAIRUP_COL_ENSURE, x, -1, -1, m->weight_ensure };
        m->col_start[j] = m->nnz;
        add_entry (m, row_ens[x], 1);
    }
    m->col_start[m->n_cols] = m->nnz;

    free (row_slot);
    return 0;
}

void
pairup_model_free (pairup_model *m)
{
    free (m->cols);
    free (m->rows);
    free (m->col_start);
    free (m->row_index);
    free (m->value);
    memset (m, 0, sizeof(*m));
}

int
pairup_model_rowwise (const pairup_model *m,
                      int **row_start,
                      int **col_index,
                      double **value)
{
    int *start = calloc ((size_t) m->n_rows + 1, sizeof(int));
    int *index = malloc (((size_t) m->nnz + 1) * sizeof(int));
    double *val = malloc (((size_t) m->nnz + 1) * sizeof(double));
    int *fill = malloc (((size_t) m->n_rows + 1) * sizeof(int));

    if (!start || !index || !val || !fill)
    {
        free (start); free (index); free (val); free (fill);
        return -1;
    }

    for (int k = 0; k < m->nnz; k++)
        start[m->row_index[k] + 1]++;
    for (int i = 0; i < m->n_rows; i++)
        start[i + 1] += start[i];
    memcpy (fill, start, (size_t) m->n_rows * sizeof(int));

    /* Columns are visited in order, so each row's terms come out sorted */
    for (int j = 0; j < m->n_cols; j++)
    {
        for (int k = m->col_start[j]; k < m->col_start[j + 1]; k++)
        {
            int pos = fill[m->row_index[k]]++;
            index[pos] = j;
            val[pos] = m->value[k];
        }
    }

    free (fill);
    *row_start = start;
    *col_index = index;
    *value = val;
    return 0;
}
