#include <stdarg.h>
#include <stdio.h>
#include "pairup-formatter.h"
#include "pairup-types.h"
#include "pairup-algorithm.h"
#include "rw-csv.h"
#include "portable_wcwidth.h"

#include <stdio.h>
#include <string.h>
#include <locale.h>

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
            int is_special_col = (j == FILED_COL_NAME || j == FILED_COL_END + 1);
            int is_field_col = (j >= FILED_COL_START && j <= FILED_COL_END && i >= FILED_ROW_START);

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

int
calculate_dpi (int line_count)
{
    int base_dpi = 72;
    int max_dpi = 600;
    int threshold = 200;

    if (line_count >= threshold)
        return max_dpi;

    return base_dpi + (max_dpi - base_dpi) * line_count / threshold;
}

int
count_lines (const char *filename)
{
    FILE *file = fopen (filename, "r");
    if (!file)
    {
        perror ("Error opening file");
        return 0;
    }

    int lines = 0;
    char ch;
    while ((ch = fgetc(file)) != EOF)
    {
        if (ch == '\n')
            lines++;
    }

    fclose (file);
    return lines;
}

void
generate_graph_output_image (sheet *worksheet,
                             const char *filename)
{
    char dotfile[1024];
    strcpy (dotfile, filename);
    strcat (dotfile, ".dot");

    print_graph_to_file (worksheet, dotfile);

    int line_count = count_lines (dotfile);
    int dpi = calculate_dpi (line_count);

    char command[2048];
    snprintf (command, sizeof(command), "dot -Tpng -Gdpi=%d %s -o %s", dpi, dotfile, filename);

    int ret = system (command);

    if (ret != 0)
    {
        fprintf (stderr, "Error: failed to generate graph image\n");
        return;
    }
    printf ("Graph image has been generated to %s\n", filename);
}

void
print_graph_to_file (sheet *worksheet,
                     const char *filename)
{
    printf("Output: %s\n", filename);
    FILE *file = fopen (filename, "w");
    if (file == NULL)
    {
        fprintf (stderr, "Error: cannot open file %s\n", filename);
        return;
    }

    fprintf (file, "graph G {\n");
    fprintf (file,"    /* File attributes */\n");
    fprintf (file,"    size=\"6,4\";\n    ratio=fill;\n");

    relation_graph_t *graph = pairup_graph (worksheet);
    if (graph == NULL)
    {
        fprintf (stderr, "Error: failed to generate graph\n");
        fclose (file);
        return;
    }

    fprintf (file, "\n    /* Node attributes */\n");
    for (size_t i = 0; i < graph->count; i++)
    {
        relation_t *relation = graph->relations[i];
        member *member = relation->candidates[0];

        fprintf (file, "    \"%s\" [label=\"%s: %zu\"];\n",
                 member->name, member->name, member->requests);
    }

    fprintf (file, "\n    /* Edge relations */\n");

    size_t edge_count = 0;
    for (size_t i = 0; i < graph->count; i++)
    {
        relation_t *relation = graph->relations[i];
        edge_count += (relation->count - 1);
    }

    typedef struct {
        char *node1;
        char *node2;
        slot time;
    } edge_t;

    edge_t *printed_edges = (edge_t *) malloc (edge_count * sizeof(edge_t));
    if (!printed_edges)
    {
        fprintf (stderr, "Error: failed to allocate memory for edges\n");
        fclose (file);
        return;
    }

    int printed_count = 0;

    for (size_t i = 0; i < graph->count; i++)
    {
        relation_t *relation = graph->relations[i];
        member *source = relation->candidates[0];

        for (size_t j = 1; j < relation->count; j++)
        {
            member *target = relation->candidates[j];
            slot time = relation->matched_slot[j];

            int is_duplicate = 0;
            for (int k = 0; k < printed_count; k++)
            {
                if ((strcmp(printed_edges[k].node1, source->name) == 0 &&
                     strcmp(printed_edges[k].node2, target->name) == 0) ||
                    (strcmp(printed_edges[k].node1, target->name) == 0 &&
                     strcmp(printed_edges[k].node2, source->name) == 0))
                {
                    is_duplicate = 1;
                    break;
                }
            }

            if (!is_duplicate)
            {
                char *timestr = get_time_slot (worksheet, time);
                fprintf (file, "    \"%s\" -- \"%s\" [label=\"%s\" fontsize=7];\n",
                         source->name, target->name, timestr);

                printed_edges[printed_count].node1 = source->name;
                printed_edges[printed_count].node2 = target->name;
                printed_edges[printed_count].time = time;
                printed_count++;
            }
        }
    }

    fprintf (file, "}\n");

    fclose (file);

    free (printed_edges);

    printf ("Graph has been written to %s\n", filename);

    free_relation_graph (graph);
}

void
print_digraph_to_file (sheet *worksheet,
                       const char *filename)
{
    FILE *file = fopen (filename, "w");
    if (file == NULL)
    {
        fprintf (stderr, "Error: cannot open file %s\n", filename);
        return;
    }

    fprintf (file, "digraph G { \n  size=\"6,4\";\n  ratio=fill;\n");

    relation_graph_t *graph = pairup_graph (worksheet);
    if (graph == NULL)
    {
        fprintf (stderr, "Error: failed to generate graph\n");
        fclose (file);
        return;
    }

    int i, j;
    for (i = 0; i < graph->count; i++)
    {
        relation_t *relation = graph->relations[i];

        char *source = relation->candidates[0]->name;

        if (relation->count == 1)
        {
            fprintf (file, "  \"%s\";\n", source);
            continue;
        }

        for (j = 1; j < relation->count; j++)
        {
            char *target = relation->candidates[j]->name;
            fprintf (file, "  \"%s\" -> \"%s\";\n", source, target);
        }
    }

    fprintf (file, "}\n");

    fclose (file);

    printf ("Graph has been written to %s\n", filename);

    free_relation_graph (graph);
}

void
print_result (sheet *worksheet,
              pair_result *result)
{
    if (result->pairs == 0)
    {
        printf ("%s\n", NOPAIRS_SUGGESTION);
    }
    else
    {
        printf ("%s\n", GREETING);
        for (int i = 0; i < result->pairs; i++)
        {
            pair_t *pair = result->pair_list[i];
            char *time = get_time_slot (worksheet, pair->time);
            printf ("@%s -- @%s (%s)\n", pair->a->name, pair->b->name, time);
        }
    }

    printf ("\n");
    printf ("As for\n");

    if (result->singles != 0)
    {
        for (int i = 0; i < result->singles; i++)
        {
            member_t *member = result->single_list[i];
            printf ("@%s\n", member->name);
        }
    }

    printf ("%s\n", ALTERNATIVES_FOR_NOT_MATCHED);
}

void
display_graph (void *context)
{
    relation_graph_t *graph = (relation_graph_t *)context;

    for (int i = 0; i < graph->count; i++)
    {
        relation_t *row = graph->relations[i];
        /*printf ("Relation %d: [ %s ] --> ", i, row->candidates[0]->name);*/
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ]   Relation %d: [ %s ] --> ", i, row->candidates[0]->name);
        for (int j = 1; j < row->count; j++)
        {
            int time = row->matched_slot[j];
            /*printf ("%s(at %d) --> ", row->candidates[j]->name, time);*/
            debug_printf (DEBUG_SUMMARY, "%s(at %d) --> ", row->candidates[j]->name, time);
        }
        printf ("\n");
    }
}

void
display_summary (pair_result *result)
{
    size_t M = result->member;
    size_t P = result->pairs;
    size_t NP = result->singles;
    size_t REQ = result->total_requests;
    size_t SREQ = P << 1;
    size_t NREQ = NP;

    int RATIO_SREQ, RATIO_NREQ;
    if (REQ == 0)
    {
        RATIO_SREQ = -1;
        RATIO_NREQ = -1;
    }
    else
    {
        RATIO_SREQ = (SREQ * 100) / REQ;
        RATIO_NREQ = (NREQ * 100) / REQ;
    }

    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Members: %zu\n", M);
    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Successful pairs: %zu\n", P);
    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Total requests: %zu\n", REQ);

    if (RATIO_SREQ == -1 || RATIO_NREQ == -1)
    {
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Successful requests: %zu\n", SREQ);
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Failed requests: %zu\n", NREQ);
    }
    else
    {
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Successful requests: %zu (%d%%)\n", SREQ, RATIO_SREQ);
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Failed requests: %zu (%d%%)\n", NREQ, RATIO_NREQ);
    }

    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Pair list:\n");
    for (int i = 0; i < result->pairs; i++)
    {
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ]   %s -- %s\n", result->pair_list[i]->a->name, result->pair_list[i]->b->name);
    }
    debug_printf (DEBUG_SUMMARY, "[ SUMMARY ] Single list:\n");
    for (int i = 0; i < result->singles; i++)
    {
        debug_printf (DEBUG_SUMMARY, "[ SUMMARY ]   %s\n", result->single_list[i]->name);
    }
}

int debug_level = DEBUG_NONE;

/* Debug function */
// void
// do_if_debug_level_is_greater (int level,
//                               callback fptr,
//                               void *context,
//                               const char *fmt, ...)
// {
//     if (level > debug_level)
//     {
//         return;
//     }

//     va_list args;
//     va_start (args, fmt);
//     va_end (args);

//     if (fmt != NULL)
//     {
//         vprintf (fmt, args);
//     }

//     if (fptr != NULL)
//     {
//         fptr (context);
//     }
// }

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
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }

    if (fptr != NULL)
    {
        fptr(context);
    }
}
