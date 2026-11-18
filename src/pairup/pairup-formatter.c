#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pairup-formatter.h"
#include "pairup-problem.h"
#include "pairup-model.h"
#include "portable_wcwidth.h"

#define FLEX_ONLY_SUGGESTION "????~????"

/********************************  Worksheet  ****************************************/

int
get_display_width (const char *str)
{
    int width = 0;
    while (*str)
    {
        unsigned char c = *str;
        if (c < 0x80)
        {
            width += 1;
            str++;
        }
        else
        {
            wchar_t wc;
            int len = mbtowc (&wc, str, MB_CUR_MAX);
            if (len > 0)
            {
                width += portable_wcwidth (wc); // Calculate the width of the character
                str += len;                     // Skip the character
            }
            else
            {
                str++;
            }
        }
    }
    return width;
}

/* Truncate the string to the maximum width */
void
print_truncated (const char *str,
                 int max_width)
{
    int current_width = 0;
    const char *start = str;

    while (*str && current_width < max_width)
    {
        unsigned char c = *str;
        if (c < 0x80)
        {
            current_width += 1;
            str++;
        }
        else
        {
            wchar_t wc;
            int len = mbtowc (&wc, str, MB_CUR_MAX);
            if (len > 0)
            {
                int wc_width = portable_wcwidth (wc);
                if (current_width + wc_width > max_width) break;
                current_width += wc_width;
                str += len;
            }
            else
            {
                str++; // Invalid character
            }
        }
    }

    /* Print the truncated string */
    fwrite (start, 1, str - start, stdout);

    /* Use ANSI escape to move cursor and fill remaining spaces */
    if (current_width < max_width)
    {
        printf ("\033[%dC", max_width - current_width); // Move cursor right
    }
}

void
print_worksheet (sheet *worksheet)
{
    setlocale (LC_CTYPE, ""); // Set locale for wide character support

    const int default_col_width = 10; // Default column width
    const int special_col_width = 14; // Special column width

    printf ("rows: %d\n", worksheet->rows);
    printf ("cols: %d\n", worksheet->cols);
    printf ("path: %s\n", worksheet->path);
    printf ("data:\n");

    for (int i = 0; i < worksheet->rows; i++)
    {
        for (int j = 0; j < worksheet->cols; j++)
        {
            char *cell = worksheet->data[i][j];
            int is_special_col = (j == FIELD_COL_NAME || j == FIELD_COL_END + 1);
            int is_field_col = (j >= FIELD_COL_START && j <= FIELD_COL_END && i >= FIELD_ROW_START);

            if (is_field_col)
            {
                printf ("    "); // Bold text
                print_truncated (cell ? cell : "", default_col_width - 4);
                continue;
            }

            int col_width = is_special_col ? special_col_width : default_col_width;
            print_truncated (cell ? cell : "", col_width);
        }
        printf ("\n");
    }
}

/*********************************  Result  ******************************************/

/* "1900~2000, 2130~2200" for the chat message */
static void
format_suggestion (const pairup_problem *problem,
                   int member,
                   char *out,
                   size_t size)
{
    char ranges[PAIRUP_MAX_SLOTS][PAIRUP_MAX_RANGE];
    int n = pairup_member_ranges (problem, member, ranges, PAIRUP_MAX_SLOTS);
    size_t used = 0;

    out[0] = '\0';
    for (int k = 0; k < n && used < size; k++)
    {
        int w = snprintf (out + used, size - used, "%s%s", k ? ", " : "", ranges[k]);
        if (w < 0)
            break;
        used += (size_t) w;
    }

    /* Members who only filled in the flex column have no fixed time to suggest */
    int flex = pairup_flex_slot (problem);
    if (n == 0 && flex >= 0 && (problem->members[member].avail >> flex) & 1u)
        snprintf (out, size, "%s", FLEX_ONLY_SUGGESTION);
}

void
print_result (const pairup_result *result)
{
    const pairup_problem *p = &result->problem;

    if (result->n_pairs == 0)
    {
        printf ("%s\n", NOPAIRS_SUGGESTION);
    }
    else
    {
        printf ("%s\n", GREETING);
        for (int i = 0; i < result->n_pairs; i++)
        {
            const pairup_pair *pair = &result->pairs[i];
            printf ("@%s -- @%s (%s)\n",
                    p->members[pair->a].name,
                    p->members[pair->b].name,
                    p->slots[pair->slot].label);
        }
    }

    printf ("\n");
    printf ("As for\n");

    for (int i = 0; i < result->n_singles; i++)
    {
        char suggestion[256];
        format_suggestion (p, result->singles[i], suggestion, sizeof(suggestion));
        printf ("@%s (%s)\n", p->members[result->singles[i]].name, suggestion);
    }

    printf ("%s\n", ALTERNATIVES_FOR_NOT_MATCHED);
}

cJSON *
init_result_json_object (const pairup_result *r)
{
    const pairup_problem *p = &r->problem;

    debug_printf (DEBUG_INFO, "Initializing a JSON object with pair result ...\n");
    cJSON *root = cJSON_CreateObject ();
    cJSON_AddNumberToObject (root, "exit_code", 0);
    cJSON_AddStringToObject (root, "exit_msg",
                             r->status == PAIRUP_OPTIMAL
                                 ? "Completed without error"
                                 : "Completed, but the solver hit its time limit so the result may not be optimal");
    cJSON_AddStringToObject (root, "algorithm", PAIRUP_ALGORITHM_NAME);
    cJSON_AddNumberToObject (root, "n_successful_req", r->n_pairs * 2);
    cJSON_AddNumberToObject (root, "n_failed_req", r->n_singles);

    debug_printf (DEBUG_INFO, "Creating JSON array and filling with paired members ...\n");
    cJSON *paired = cJSON_CreateArray ();
    for (int i = 0; i < r->n_pairs; ++i)
    {
        const pairup_pair *pair = &r->pairs[i];
        cJSON *obj = cJSON_CreateObject ();
        cJSON_AddStringToObject (obj, "matched_time", p->slots[pair->slot].label);
        cJSON_AddStringToObject (obj, "member_a", p->members[pair->a].name);
        cJSON_AddStringToObject (obj, "member_b", p->members[pair->b].name);
        cJSON_AddItemToArray (paired, obj);
    }

    debug_printf (DEBUG_INFO, "Creating JSON array and filling with single members ...\n");
    cJSON *singles = cJSON_CreateArray ();
    for (int i = 0; i < r->n_singles; ++i)
    {
        int m = r->singles[i];
        cJSON *obj = cJSON_CreateObject ();
        cJSON_AddStringToObject (obj, "member", p->members[m].name);

        char ranges[PAIRUP_MAX_SLOTS][PAIRUP_MAX_RANGE];
        int n = pairup_member_ranges (p, m, ranges, PAIRUP_MAX_SLOTS);
        cJSON *range_array = cJSON_CreateArray ();
        for (int k = 0; k < n; k++)
            cJSON_AddItemToArray (range_array, cJSON_CreateString (ranges[k]));

        cJSON_AddItemToObject (obj, "available_ranges", range_array);
        cJSON_AddItemToArray (singles, obj);
    }

    cJSON_AddItemToObject (root, "result_paired", paired);
    cJSON_AddItemToObject (root, "result_single", singles);

    return root;
}

void free_result_json_object (cJSON *root)
{
    if (root)
        cJSON_Delete (root);
}

void free_result_json_string (char *str)
{
    if (str)
        cJSON_free (str);
}

/******************************  Relation graph  *************************************/

/* Escape for a double-quoted DOT string */
static void
fput_dot_string (FILE *f,
                 const char *str)
{
    fputc ('"', f);
    for (; *str; str++)
    {
        if (*str == '"' || *str == '\\')
            fputc ('\\', f);
        fputc (*str, f);
    }
    fputc ('"', f);
}

/* Escape for text inside a DOT HTML-like label */
static void
fput_html (FILE *f,
           const char *str)
{
    for (; *str; str++)
    {
        switch (*str)
        {
            case '&': fputs ("&amp;", f); break;
            case '<': fputs ("&lt;", f); break;
            case '>': fputs ("&gt;", f); break;
            case '"': fputs ("&quot;", f); break;
            default:  fputc (*str, f);
        }
    }
}

static const char *const row_titles[] = {
    [PAIRUP_ROW_SLOT]     = "SLOT: each member uses each time slot at most once",
    [PAIRUP_ROW_REQUESTS] = "REQUESTS: no more practices than the member requested",
    [PAIRUP_ROW_PARTNER]  = "PARTNER: never the same partner twice",
    [PAIRUP_ROW_ENSURE]   = "ENSURE: y can only be 1 if the --ensure member is paired",
};

/* x columns come first, then y columns (see pairup_model_build) */
static void
fput_col_name (FILE *f,
               const pairup_model *m,
               int j)
{
    if (m->cols[j].kind == PAIRUP_COL_PAIR)
    {
        fprintf (f, "x%d", j + 1);
        return;
    }
    int n_pair = 0;
    while (n_pair < m->n_cols && m->cols[n_pair].kind == PAIRUP_COL_PAIR)
        n_pair++;
    fprintf (f, "y%d", j - n_pair + 1);
}

static void
fput_row_label (FILE *f,
                const pairup_problem *p,
                const pairup_row *r)
{
    fput_html (f, p->members[r->a].name);
    if (r->kind == PAIRUP_ROW_SLOT)
    {
        fputs (" @ ", f);
        fput_html (f, p->slots[r->slot].label);
    }
    else if (r->kind == PAIRUP_ROW_PARTNER)
    {
        fputs (" &amp; ", f);
        fput_html (f, p->members[r->b].name);
    }
}

#define TD_HEAD "<TD ALIGN=\"LEFT\" COLSPAN=\"3\" BGCOLOR=\"#e8e8e8\"><B>"
#define TD      "<TD ALIGN=\"LEFT\">"

/* The whole ILP as an HTML-like table, used as the graph label */
static void
fput_model_table (FILE *f,
                  const pairup_problem *p,
                  const pairup_model *m)
{
    int *row_start = NULL, *col_index = NULL;
    double *value = NULL;

    fprintf (f, "<<TABLE BORDER=\"1\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"3\">\n");
    fprintf (f, "<TR><TD COLSPAN=\"3\"><B>ILP sent to HiGHS: %d variables, %d constraints</B></TD></TR>\n",
             m->n_cols, m->n_rows);

    if (m->n_cols == 0)
    {
        fprintf (f, "<TR><TD COLSPAN=\"3\">Nobody shares a time slot, nothing to solve.</TD></TR>\n");
        fprintf (f, "</TABLE>>");
        return;
    }

    /* Variables and their objective coefficients */
    fprintf (f, "<TR>" TD_HEAD "Variables (binary) and objective: maximize the sum of weight * variable</B></TD></TR>\n");
    for (int j = 0; j < m->n_cols; j++)
    {
        const pairup_col *c = &m->cols[j];
        fprintf (f, "<TR>" TD);
        fput_col_name (f, m, j);
        fprintf (f, "</TD>" TD);
        if (c->kind == PAIRUP_COL_PAIR)
        {
            fput_html (f, p->members[c->a].name);
            fputs (" + ", f);
            fput_html (f, p->members[c->b].name);
            fputs (" @ ", f);
            fput_html (f, p->slots[c->slot].label);
        }
        else
        {
            fput_html (f, p->members[c->a].name);
            fputs (" has a partner (--ensure)", f);
        }
        fprintf (f, "</TD>" TD "weight %g</TD></TR>\n", c->cost);
    }

    /* Constraints, grouped by rule */
    if (pairup_model_rowwise (m, &row_start, &col_index, &value) != 0)
    {
        fprintf (f, "<TR><TD COLSPAN=\"3\">(out of memory while listing constraints)</TD></TR>\n</TABLE>>");
        return;
    }

    for (int kind = PAIRUP_ROW_SLOT; kind <= PAIRUP_ROW_ENSURE; kind++)
    {
        bool header = false;
        for (int i = 0; i < m->n_rows; i++)
        {
            const pairup_row *r = &m->rows[i];
            if ((int) r->kind != kind)
                continue;
            if (!header)
            {
                fprintf (f, "<TR>" TD_HEAD "%s</B></TD></TR>\n", row_titles[kind]);
                header = true;
            }

            fprintf (f, "<TR>" TD);
            fput_row_label (f, p, r);
            fprintf (f, "</TD><TD ALIGN=\"LEFT\" COLSPAN=\"2\">");
            for (int k = row_start[i]; k < row_start[i + 1]; k++)
            {
                bool negative = value[k] < 0;
                if (k > row_start[i])
                    fputs (negative ? " - " : " + ", f);
                else if (negative)
                    fputs ("-", f);
                fput_col_name (f, m, col_index[k]);
            }
            fprintf (f, " &lt;= %g</TD></TR>\n", r->upper);
        }
    }

    fprintf (f, "</TABLE>>");
    free (row_start);
    free (col_index);
    free (value);
}

void
generate_graph_output_image (const pairup_problem *problem,
                             const char *filename)
{
    char dotfile[1024];
    snprintf (dotfile, sizeof(dotfile), "%s.dot", filename);

    if (print_graph_to_file (problem, dotfile) != 0)
        return;

    char command[2200];
    snprintf (command, sizeof(command), "dot -Tpng -Gdpi=150 \"%s\" -o \"%s\"", dotfile, filename);

    if (system (command) != 0)
    {
        fprintf (stderr, "Error: failed to generate graph image\n");
        return;
    }
    printf ("Graph image has been generated to %s\n", filename);
}

/* Lowest slot index set in `mask` */
static int
first_slot (uint32_t mask)
{
    int t = 0;
    while (!((mask >> t) & 1u))
        t++;
    return t;
}

int
print_graph_to_file (const pairup_problem *problem,
                     const char *filename)
{
    char err[256];
    pairup_model model;

    if (pairup_model_build (problem, &model, err, sizeof(err)) != 0)
    {
        fprintf (stderr, "Error: %s\n", err);
        return -1;
    }

    printf ("Output: %s\n", filename);
    FILE *file = fopen (filename, "w");
    if (file == NULL)
    {
        fprintf (stderr, "Error: cannot open file %s\n", filename);
        pairup_model_free (&model);
        return -1;
    }

    fprintf (file, "graph G {\n");
    fprintf (file, "    /* File attributes */\n");
    fprintf (file, "    labelloc=b;\n    labeljust=l;\n");

    fprintf (file, "\n    /* Node attributes */\n");
    for (int i = 0; i < problem->n_members; i++)
    {
        const pairup_member *m = &problem->members[i];
        char label[PAIRUP_MAX_NAME + 16];
        snprintf (label, sizeof(label), "%s: %d", m->name, m->requests);
        fputs ("    ", file);
        fput_dot_string (file, m->name);
        fputs (" [label=", file);
        fput_dot_string (file, label);
        fputs ("];\n", file);
    }

    /* One edge per pair of members, labelled with their earliest shared slot */
    fprintf (file, "\n    /* Edge relations */\n");
    for (int a = 0; a < problem->n_members; a++)
    {
        for (int b = a + 1; b < problem->n_members; b++)
        {
            uint32_t common = problem->members[a].avail & problem->members[b].avail;
            if (!common)
                continue;
            fputs ("    ", file);
            fput_dot_string (file, problem->members[a].name);
            fputs (" -- ", file);
            fput_dot_string (file, problem->members[b].name);
            fputs (" [label=", file);
            fput_dot_string (file, problem->slots[first_slot (common)].label);
            fputs (" fontsize=7];\n", file);
        }
    }

    fprintf (file, "\n    /* The ILP handed to the solver */\n");
    fprintf (file, "    label=");
    fput_model_table (file, problem, &model);
    fprintf (file, ";\n}\n");

    fclose (file);
    pairup_model_free (&model);

    printf ("Graph has been written to %s\n", filename);
    return 0;
}

/**********************************  Debug  ******************************************/

void
display_graph (const pairup_problem *problem)
{
    for (int a = 0; a < problem->n_members; a++)
    {
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ]   Relation %d: [ %s ] --> ", a, problem->members[a].name);
        for (int t = 0; t < problem->n_slots; t++)
        {
            for (int b = 0; b < problem->n_members; b++)
            {
                uint32_t common = problem->members[a].avail & problem->members[b].avail;
                if (b != a && (common >> t) & 1u)
                    debug_printf (DEBUG_SUMMARY, "%s(at %s) --> ", problem->members[b].name, problem->slots[t].label);
            }
        }
        debug_printf (DEBUG_SUMMARY, "\n");
    }
}

void
display_summary (const pairup_result *result)
{
    const pairup_problem *p = &result->problem;
    int REQ = result->total_requests;
    int SREQ = result->n_pairs * 2;
    int NREQ = REQ - SREQ;

    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Members: %d\n", p->n_members);
    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Successful pairs: %d\n", result->n_pairs);
    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Total requests: %d\n", REQ);

    if (REQ == 0)
    {
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Successful requests: %d\n", SREQ);
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Failed requests: %d\n", NREQ);
    }
    else
    {
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Successful requests: %d (%d%%)\n", SREQ, SREQ * 100 / REQ);
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Failed requests: %d (%d%%)\n", NREQ, NREQ * 100 / REQ);
    }

    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Pair list:\n");
    for (int i = 0; i < result->n_pairs; i++)
    {
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ]   %s -- %s (%s)\n",
                      p->members[result->pairs[i].a].name,
                      p->members[result->pairs[i].b].name,
                      p->slots[result->pairs[i].slot].label);
    }
    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Single list:\n");
    for (int i = 0; i < result->n_singles; i++)
    {
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ]   %s\n", p->members[result->singles[i]].name);
    }
}

int debug_level = DEBUG_NONE;

void
do_if_debug_level_is_greater (int level,
                              callback fptr,
                              void *context,
                              const char *fmt, ...)
{
    if (level > debug_level)
    {
        return;
    }

    if (fmt != NULL)
    {
        va_list args;
        va_start (args, fmt);
        vprintf (fmt, args);
        va_end (args);
    }

    if (fptr != NULL)
    {
        fptr (context);
    }
}
