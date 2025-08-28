#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#if defined(_WIN32) || defined(_WIN64)
#include "getopt.h"
#else
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#endif

#include "pairup/pairup.h"
#include "version.h"
#include "rw-csv.h"

const char *program_name = "pairup";

#define emit_try_help() \
  do \
    { \
      fprintf (stderr, "Try '%s --help' for more information.\n", \
               program_name); \
    } \
  while (0)

void
usage (int status)
{
    if (status != EXIT_SUCCESS)
    {
        emit_try_help();
    }
    else
    {
        printf ("\
Usage: %s [OPTION]... SOURCE_CSV\n\
Generate optimal matches based on member's available time with linear time complexity\n\n\
Options:\n\
  -s, --show-csv              show the csv data only (no pair result)\n\
  -g, --graph={OUTPUT}        generate the relation graph\n\
  -e, --ensure={MEMBER}       ensure specified member can have partner today\n\
  -j, --json-output           print structural output(JSON)\n\
  -d, --debug={LEVEL}         set the debug level (0: only error, 5: all info)\n\
  -p, --priority={FUNC}       specify the match priority algorithm\n\
  -A, --avoid-same-match=WEEK avoid pairing same partners within the same week; write history JSON\n\
  -v, --version               print the version information\n\
  -h, --help                  print this page\n\n\
Examples:\n\
  %s '英文讀書會時間 Ver.4.csv'              # display the optimal matches\n\
  %s -s '英文讀書會時間 Ver.4.csv'           # show the csv data only\n\
  %s -g '英文讀書會時間 Ver.4.csv'           # generate 'relations.png' pairing graph\n\
  %s -p LAST_ROW '英文讀書會時間 Ver.4.csv'  # match member from the last row\n\
  %s -e 'Bob' '英文讀書會時間 Ver.4.csv'     # match Bob first\n\
  %s -A 2025-W35 data.csv                    # avoid re-pairing happened in week 2025-W35\n\n\
For more information, see <https://github.com/jackiesogi/pairup.c>.\n\
", program_name, program_name, program_name, program_name, program_name, program_name, program_name);
    }
    exit (status);
}

bool
has_installed_graphviz ()
{
    if (system("which dot > /dev/null") == 0)
        return true;
    return false;
}

static char const short_options[] = "d:sg::e:jp:A:vh";

static struct option const long_options[] =
{
    {"show-csv", no_argument, NULL, 's'},
    {"graph", optional_argument, NULL, 'g'},
    {"priority", required_argument, NULL, 'p'},
    {"ensure", required_argument, NULL, 'e'},
    {"json-output", no_argument, NULL, 'j'},
    {"avoid-same-match", required_argument, NULL, 'A'},
    {"debug", required_argument, NULL, 'd'},
    {"version", no_argument, NULL, 'v'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

void
sigsegv_handler (int sig) {
    fprintf (stdout, "The worksheet format is incorrect. Please check the content on Google Sheets with the following format rules, or contact the developer:\n\
(1) No extra characters should appear outside the main table.\n\
(2) There should be no empty rows between members.\n\
(3) There should be no empty columns between the time slots.\n\
(4) The number of members should not exceed 64. (If so, contact the developer to scale it up.)\n");
    exit(EXIT_FAILURE);
}

void
append_user_defined_ensure_list (struct user_defined_ensure_list *list,
                                 const char *src)
{
    if (list->ensure_list_size < MAX_MEMBERS_LEN) {
        strncpy(list->ensure_list_content[list->ensure_list_size],
                src,
                MAX_NAME_LEN - 1);
        list->ensure_list_content[list->ensure_list_size][MAX_NAME_LEN - 1] = '\0';
        list->ensure_list_size++;
    }
}

int
parse_debug_level (const char *str)
{
    if (!str)
    {
        return -1;
    }

    if (isdigit (str[0]))
    {
        return atoi(&str[0]);
    }
    else if (strncmp(str, "NONE", 8) == 0)
    {
        return 0;
    }
    else if (strncmp(str, "ERROR", 8) == 0)
    {
        return 1;
    }
    else if (strncmp(str, "WARNING", 8) == 0)
    {
        return 2;
    }
    else if (strncmp(str, "SUMMARY", 8) == 0)
    {
        return 3;
    }
    else if (strncmp(str, "INFO", 8) == 0)
    {
        return 4;
    }
    else if (strncmp(str, "ALL", 8) == 0)
    {
        return 5;
    }
    else
    {
        return -1;
    }
}

int
main (int argc, char *argv[])
{

#ifndef _WIN32
    signal (SIGSEGV, sigsegv_handler);
#endif

    /* Flags for command line arguments */
    struct pairup_options x;
    pairup_options_init (&x);
    char graph_output[1024];
    struct user_defined_ensure_list elist;

    /* Parse the command line arguments using while loop */
    int c;
    while ((c = getopt_long (argc, argv, short_options, long_options, NULL)) != -1)
    {
        switch (c)
        {
            case 'd':
                debug_level = parse_debug_level (optarg);
                x.debug_level = debug_level;
                break;
            case 'g':
                if (!has_installed_graphviz())
                {
                    debug_printf (DEBUG_ERROR, "[ ERROR   ] Please install package `graphviz` first\n");
                    exit (EXIT_FAILURE);
                }
                x.generate_graph = true;
                if (optarg)
                    strcpy (graph_output, (const char *)optarg);
                else
                    strcpy (graph_output, "relations.png");
                break;
            case 's':
                x.show_csv = true;
                break;
            case 'e':
                x.ensure = true;
                append_user_defined_ensure_list (&elist, optarg);
                break;
            case 'j':
                x.json_output = true;
                break;
            case 'p':
                x.priority = true;
                strncpy(x.priority_func, optarg, 1024);
                break;
            case 'A':
                x.avoid_same_match = true;
                if (optarg)
                {
                    strncpy(x.avoid_week, optarg, sizeof(x.avoid_week)-1);
                    x.avoid_week[sizeof(x.avoid_week)-1] = '\0';
                }
                else
                {
                    strcpy(x.avoid_week, "UNKNOWN-WEEK");
                }
                /* default history file next to SOURCE_CSV with name already-pair.json */
                strcpy(x.history_path, "already-pair.json");
                break;
            case 'v':
                printf ("%s\n", PROGRAM_VERSION);
                return 0;
            case 'h':
                usage (EXIT_SUCCESS);
                break;
            case '?':
                usage (EXIT_FAILURE);
                break;
            default:
                abort ();
        }
    }

    if (optind >= argc)
    {
        fprintf (stderr, "%s: missing operand\n", program_name);
        exit (EXIT_FAILURE);
    }

    /* Additional non-option arguments is the input file */
    char *path = argv[optind];

    /* Read the csv file */
    sheet_t worksheet = read_csv (path);

    /* Load history if needed */
    if (x.avoid_same_match)
    {
        x.history_ctx = history_load(x.history_path);
    }

    if (x.generate_graph)
    {
        generate_graph_output_image (&worksheet, graph_output);
        return 0;
    }
    if (x.show_csv)
    {
        print_worksheet (&worksheet);
        return 0;
    }
    if (x.ensure == true)
    {
        x.ensure_member_list = &elist;
    }

    /* Randomize the worksheet rows to avoid bias */
    debug_printf(DEBUG_INFO, "\
[ INFO    ] Shuffling each row inside the input worksheet to avoid bias result ...\n");
    shuffle_worksheet (&worksheet, time(NULL));
    debug_printf(DEBUG_INFO, "[ INFO    ] Finished shuffling.\n");

    /* Trigger the top-level pairup function */
    debug_printf(DEBUG_INFO, "[ INFO    ] Starting the pairing up process ...\n");
    pair_result_t *result = pairup (&worksheet, &x);

    /* Print the result */
    if (x.json_output == true)
    {
        cJSON *root = init_result_json_object (&worksheet, result);
        char *json_output = get_result_json_string (root);
        fprintf (stdout, "%s\n", json_output);
        free_result_json_object (root);
        free_result_json_string (json_output);
    }
    else
    {
        print_result (&worksheet, result);
    }

    if (x.avoid_same_match && x.history_ctx)
    {
        history_save_update(x.history_ctx, x.history_path, x.avoid_week, result);
    }
    free_pair_result (result);

    debug_printf(DEBUG_INFO, "[ INFO    ] Done!\n");
    return 0;
}
