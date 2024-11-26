#include <stdarg.h>
#include <stdio.h>
#include "pairup-formatter.h"
#include "pairup-types.h"
#include "pairup-algorithm.h"
#include "rw-csv.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>
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
            int len = mbtowc(&wc, str, MB_CUR_MAX);
            if (len > 0)
            {
                width += wcwidth(wc); // Calculate the width of the character
                str += len;           // Skip the character
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

    while (*str && current_width < max_width - 1)
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
            int len = mbtowc(&wc, str, MB_CUR_MAX);
            if (len > 0)
            {
                int wc_width = wcwidth(wc);
                if (current_width + wc_width > max_width - 1) break;
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
    fwrite(start, 1, str - start, stdout);

    /* Fill the remaining space with spaces */
    for (int i = current_width; i < max_width; i++)
    {
        putchar(' ');
    }
}

void
print_worksheet (sheet_t *worksheet)
{
    setlocale(LC_CTYPE, "");

    printf("rows: %d\n", worksheet->rows);
    printf("cols: %d\n", worksheet->cols);
    printf("path: %s\n", worksheet->path);
    printf("data:\n");
    for (int i = 0; i < worksheet->rows; i++)
    {
        for (int j = 0; j < worksheet->cols; j++)
        {
            char *cell = worksheet->data[i][j];
            if (cell == NULL)
            {
                print_truncated("", 10);
            }
            else
            {
                print_truncated(cell, 10);
            }
        }
        printf("\n");
    }
}

void
print_graph_to_file(sheet_t *worksheet, const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        fprintf(stderr, "Error: cannot open file %s\n", filename);
        return;
    }

    fprintf(file, "graph G { \n  size=\"6,4\";\n  ratio=fill;\n");

    relation_graph_t *graph = pairup_graph(worksheet);
    if (graph == NULL)
    {
        fprintf(stderr, "Error: failed to generate graph\n");
        fclose(file);
        return;
    }

    fprintf(file, "  // Node attributes\n");
    for (size_t i = 0; i < graph->count; i++)
    {
        relation_t *relation = graph->relations[i];
        member *member = relation->candidates[0];

        fprintf(file, "  \"%s\" [label=\"%s: %zu\"];\n",
                member->name, member->name, member->requests);
    }

    fprintf(file, "  // Edges\n");
    typedef struct {
        char *node1;
        char *node2;
        slot time;
    } edge_t;
    edge_t *printed_edges = (edge_t *)malloc(graph->count * sizeof(edge_t));
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
                char *timestr = _get_time_slot(worksheet, time);
                fprintf(file, "  \"%s\" -- \"%s\" [label=\"%s\"];\n",
                        source->name, target->name, timestr);

                printed_edges[printed_count].node1 = source->name;
                printed_edges[printed_count].node2 = target->name;
                printed_edges[printed_count].time = time;
                printed_count++;
            }
        }
    }

    fprintf(file, "}\n");

    fclose(file);
    free(printed_edges);

    printf("Graph has been written to %s\n", filename);

    _free_relation_graph(graph);
}

void
print_digraph_to_file(sheet_t *worksheet, const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        fprintf(stderr, "Error: cannot open file %s\n", filename);
        return;
    }

    fprintf(file, "digraph G { \n  size=\"6,4\";\n  ratio=fill;\n");

    relation_graph_t *graph = pairup_graph(worksheet);
    if (graph == NULL)
    {
        fprintf(stderr, "Error: failed to generate graph\n");
        fclose(file);
        return;
    }

    int i, j;
    for (i = 0; i < graph->count; i++)
    {
        relation_t *relation = graph->relations[i];

        char *source = relation->candidates[0]->name;

        if (relation->count == 1)
        {
            fprintf(file, "  \"%s\";\n", source);
            continue;
        }

        for (j = 1; j < relation->count; j++)
        {
            char *target = relation->candidates[j]->name;
            fprintf(file, "  \"%s\" -> \"%s\";\n", source, target);
        }
    }

    fprintf(file, "}\n");

    fclose(file);

    printf("Graph has been written to %s\n", filename);

    _free_relation_graph(graph);
}

void
print_result (sheet_t *worksheet,
              pair_result_t *result)
{
    printf("%s\n", GREETING);

    if (result->pairs == 0)
    {
        printf("No pairs available to display.\n");
    }
    else 
    {
        for (int i = 0; i < result->pairs; i++)
        {
            pair_t *pair = result->pair_list[i];
            char *time = _get_time_slot(worksheet, pair->time);
            printf("@%s -- @%s (%s)\n", pair->a->name, pair->b->name, time);
        }
    }
    
    printf("\n");
    printf("As for\n");

    if (result->singles != 0)
    {
        for (int i = 0; i < result->singles; i++)
        {
            member_t *member = result->single_list[i];
            printf("@%s\n", member->name);
        }
    }

    printf("%s\n", ALTERNATIVES_FOR_NOT_MATCHED);
}

void
print_result_statistics (pair_result_t *result)
{
    size_t M = result->member;
    size_t P = result->pairs;
    size_t NP = result->singles;
    size_t REQ = result->total_requests;
    size_t RATIO_P = (P * 100) / M;
    size_t RATIO_NP = (NP * 100) / M;

    printf("=======================  STATISTICS  =======================\n");
    printf("Number of members today: %zu\n", result->member);
    printf("Number of successful pairs: %zu (%zu%%)\n", result->pairs, RATIO_P);
    printf("Number of members not fully paired: %zu (%zu%%)\n", result->singles, RATIO_NP);
    printf("Total requests of today %zu\n", REQ);
    printf("Number of fully paired requests %zu (%zu%%)\n", P * 2, (P * 2 * 100) / REQ);
    printf("Number of not fully paired requests %zu (%zu%%)\n", NP, (NP * 100) / REQ);
    printf("Pair list:\n");
    for (int i = 0; i < result->pairs; i++)
    {
        printf("%s -- %s\n", result->pair_list[i]->a->name, result->pair_list[i]->b->name);
    }
    printf("Single list:\n");
    for (int i = 0; i < result->singles; i++)
    {
        printf("%s\n", result->single_list[i]->name);
    }
    printf("=============================================================\n");
}

int debug_level = DEBUG_NONE;

// void
// log_message (int level,
//              const char *message)
// {
//     if (level <= debug_level)
//     {
//         switch (level)
//         {
//             case DEBUG_ERROR:
//                 fprintf(stderr, "[ERROR] %s\n", message);
//                 break;
//             case DEBUG_WARNING:
//                 fprintf(stderr, "[WARNING] %s\n", message);
//                 break;
//             case DEBUG_INFO:
//                 fprintf(stdout, "[INFO] %s\n", message);
//                 break;
//         }
//     }
// }

/* Debug function */
void
log_message (int level,
             debug_fn fptr,
             void *context)
{
    if (level > debug_level)
    {
        return;
    }
    if (fptr != NULL)
    {
        fptr (context);
    }
}
