#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "pairup-problem.h"
#include "pairup-formatter.h"

#define FLEX_LABEL "flex"

/* Copy `src` into `dst` without splitting a UTF-8 sequence, trimming spaces if asked */
static void
copy_utf8 (char *dst,
           size_t size,
           const char *src,
           bool trim)
{
    if (trim)
    {
        while (*src && isspace ((unsigned char) *src))
            src++;
    }

    size_t n = strlen (src);
    if (trim)
    {
        while (n > 0 && isspace ((unsigned char) src[n - 1]))
            n--;
    }
    if (n >= size)
    {
        n = size - 1;
        /* Back off continuation bytes (10xxxxxx) so we cut on a character boundary */
        while (n > 0 && ((unsigned char) src[n] & 0xC0) == 0x80)
            n--;
    }
    memcpy (dst, src, n);
    dst[n] = '\0';
}

static const char *
cell_at (const sheet_t *worksheet,
         int row,
         int col)
{
    if (row < 0 || row >= worksheet->rows || col < 0 || col >= worksheet->cols)
        return NULL;
    return worksheet->data[row][col];
}

/* Name cell is blank (`read_csv` stores empty cells as " -- ") */
static bool
is_blank_name (const char *name)
{
    char buf[PAIRUP_MAX_NAME];
    if (!name)
        return true;
    copy_utf8 (buf, sizeof(buf), name, true);
    return buf[0] == '\0' || strcmp (buf, "--") == 0;
}

static bool
is_summary_name (const char *name)
{
    char buf[PAIRUP_MAX_NAME];
    if (!name)
        return false;
    copy_utf8 (buf, sizeof(buf), name, true);
    return strcmp (buf, PAIRUP_SUMMARY_NAME) == 0;
}

static bool
is_flex_header (const char *label)
{
    char buf[PAIRUP_MAX_LABEL];
    copy_utf8 (buf, sizeof(buf), label, true);
    for (char *p = buf; *p; p++)
        *p = (char) toupper ((unsigned char) *p);
    return strcmp (buf, "FLEX") == 0;
}

/* Any non-empty header is a time slot. `read_csv` stores empty cells as " -- ". */
static bool
is_time_header (const char *label)
{
    char buf[PAIRUP_MAX_LABEL];
    copy_utf8 (buf, sizeof(buf), label, true);
    return buf[0] != '\0' && strcmp (buf, "--") != 0;
}

/* Read the header row and register the regular slots (and the flex slot, if enabled) */
static int
load_slots (const sheet_t *worksheet,
            const struct pairup_options *opt,
            pairup_problem *problem,
            char *err,
            size_t err_size)
{
    int flex_col = -1;

    problem->n_slots = 0;
    for (int j = FIELD_COL_START; j < worksheet->cols; j++)
    {
        const char *label = cell_at (worksheet, 0, j);
        if (!label)
            continue;

        if (is_flex_header (label))
        {
            if (flex_col < 0)
                flex_col = j;
            continue;
        }
        if (!is_time_header (label))
            continue;

        if (problem->n_slots >= PAIRUP_MAX_SLOTS - 1)
        {
            snprintf (err, err_size, "too many time slot columns (max %d)", PAIRUP_MAX_SLOTS - 1);
            return -1;
        }
        pairup_slot *slot = &problem->slots[problem->n_slots++];
        copy_utf8 (slot->label, sizeof(slot->label), label, true);
        slot->col = j;
        slot->is_flex = false;
    }

    if (problem->n_slots == 0)
    {
        snprintf (err, err_size, "no time slot columns found in the header row");
        return -1;
    }

    if (opt && opt->enable_flex)
    {
        if (flex_col < 0)
        {
            debug_printf (DEBUG_WARNING, "[ WARNING ] Flex is enabled but no FLEX column was found.\n");
        }
        else
        {
            pairup_slot *slot = &problem->slots[problem->n_slots++];
            snprintf (slot->label, sizeof(slot->label), "%s", FLEX_LABEL);
            slot->col = flex_col;
            slot->is_flex = true;
        }
    }

    return 0;
}

static void
apply_ensure_list (const sheet_t *worksheet,
                   const struct pairup_options *opt,
                   pairup_problem *problem)
{
    if (!opt)
        return;

    for (size_t k = 0; k < opt->n_ensure; k++)
    {
        const char *target = opt->ensure[k];
        bool on_sheet = false, has_slot = false;

        for (int i = FIELD_ROW_START; i < worksheet->rows - 1; i++)
        {
            const char *name = cell_at (worksheet, i, FIELD_COL_NAME);
            if (name && strcmp (name, target) == 0)
            {
                on_sheet = true;
                break;
            }
        }

        for (int m = 0; m < problem->n_members; m++)
        {
            if (strcmp (problem->members[m].name, target) == 0)
            {
                problem->members[m].ensured = true;
                has_slot = true;
            }
        }

        if (!on_sheet)
            debug_printf (DEBUG_WARNING, "[ WARNING ] Warning: '%s' not found in worksheet\n", target);
        else if (!has_slot)
            debug_printf (DEBUG_INFO, "[ INFO    ] '%s' did not fill in any time slot today.\n", target);
        else
            debug_printf (DEBUG_INFO, "[ INFO    ] %s will be prioritized.\n", target);
    }
}

int
pairup_problem_load (const sheet_t *worksheet,
                     const struct pairup_options *opt,
                     pairup_problem *problem,
                     char *err,
                     size_t err_size)
{
    memset (problem, 0, sizeof(*problem));

    if (!worksheet || !worksheet->data || worksheet->rows < 1)
    {
        snprintf (err, err_size, "the worksheet is empty");
        return -1;
    }

    if (load_slots (worksheet, opt, problem, err, err_size) != 0)
        return -1;

    debug_printf (DEBUG_INFO, "[ INFO    ] Generating member list ...\n");

    /*
     * Member rows are every row with a name, except the summary row. Older
     * sheets without a summary label keep the summary in the last row.
     */
    bool has_summary_row = false;
    for (int i = FIELD_ROW_START; i < worksheet->rows; i++)
        has_summary_row |= is_summary_name (cell_at (worksheet, i, FIELD_COL_NAME));
    const int last_row = has_summary_row ? worksheet->rows : worksheet->rows - 1;

    for (int i = FIELD_ROW_START; i < last_row; i++)
    {
        uint32_t avail = 0;
        int requests = 0;
        const char *name = cell_at (worksheet, i, FIELD_COL_NAME);

        if (is_summary_name (name))
            continue;

        for (int t = 0; t < problem->n_slots; t++)
        {
            const char *cell = cell_at (worksheet, i, problem->slots[t].col);
            if (!cell || !is_available (cell))
                continue;

            avail |= (uint32_t) 1 << t;
            /* The first filled cell decides how many practices the member wants */
            if (requests == 0)
                requests = is_twice (cell) ? 2 : 1;
        }

        if (avail == 0)
            continue;

        if (is_blank_name (name))
        {
            /* Rows are shuffled, so describe the row by what was filled in */
            debug_printf (DEBUG_WARNING, "[ WARNING ] Ignoring a row with no name, filled in at:");
            for (int t = 0; t < problem->n_slots; t++)
            {
                if ((avail >> t) & 1u)
                    debug_printf (DEBUG_WARNING, " %s", problem->slots[t].label);
            }
            debug_printf (DEBUG_WARNING, "\n");
            continue;
        }

        if (problem->n_members >= PAIRUP_MAX_MEMBERS)
        {
            snprintf (err, err_size, "too many members (max %d)", PAIRUP_MAX_MEMBERS);
            return -1;
        }

        pairup_member *m = &problem->members[problem->n_members++];
        copy_utf8 (m->name, sizeof(m->name), name, false);
        m->row = i;
        m->requests = requests;
        m->avail = avail;
        m->ensured = false;

        debug_printf (DEBUG_ALL,
                      "[ ALL     ] On row %2d, found member '%s' with %d slot(s) and request=%d.\n",
                      m->row, m->name, pairup_popcount (m->avail), m->requests);
    }

    apply_ensure_list (worksheet, opt, problem);

    debug_printf (DEBUG_INFO, "[ INFO    ] Found %d member(s) with available time.\n", problem->n_members);
    return 0;
}

int
pairup_flex_slot (const pairup_problem *problem)
{
    for (int t = 0; t < problem->n_slots; t++)
    {
        if (problem->slots[t].is_flex)
            return t;
    }
    return -1;
}

/* "1900~1930" + "2000~2030" -> "1900~2030" */
static void
merge_range_label (const char *start_label,
                   const char *end_label,
                   char *out,
                   size_t size)
{
    const char *tilde_start = strchr (start_label, '~');
    const char *tilde_end = strchr (end_label, '~');

    int left_len = tilde_start ? (int) (tilde_start - start_label) : (int) strlen (start_label);
    const char *right = (tilde_end && tilde_end[1]) ? tilde_end + 1 : end_label;

    snprintf (out, size, "%.*s~%s", left_len, start_label, right);
}

int
pairup_member_ranges (const pairup_problem *problem,
                      int member,
                      char ranges[][PAIRUP_MAX_RANGE],
                      int max_ranges)
{
    uint32_t avail = problem->members[member].avail;
    int count = 0;
    int start = -1, last = -1;

    for (int t = 0; t <= problem->n_slots && count < max_ranges; t++)
    {
        bool on = t < problem->n_slots &&
                  !problem->slots[t].is_flex &&
                  (avail >> t) & 1u;
        bool adjacent = on && last >= 0 && problem->slots[t].col == problem->slots[last].col + 1;

        if (on && start >= 0 && adjacent)
        {
            last = t;
            continue;
        }

        if (start >= 0)
        {
            merge_range_label (problem->slots[start].label, problem->slots[last].label,
                               ranges[count], PAIRUP_MAX_RANGE);
            count++;
            start = -1;
        }

        if (on)
        {
            start = t;
            last = t;
        }
    }

    return count;
}
