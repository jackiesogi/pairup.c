#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
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
Generate the maximum number of matches based on member's available time\n\n\
Options:\n\
  -s, --show-csv              show the csv data only (no pair result)\n\
  -g, --graph={OUTPUT}        generate the relation graph\n\
  -e, --ensure={MEMBER}       ensure specified member can have partner today\n\
  -j, --json-output           print structural output(JSON)\n\
  -F, --enable-flex           pair up members who filled the flex time column\n\
  -d, --debug={LEVEL}         set the debug level (0: only error, 5: all info)\n\
  -p, --priority={FUNC}       (deprecated, ignored) the solver always finds the optimum\n\
  -v, --version               print the version information\n\
  -h, --help                  print this page\n\n\
Examples:\n\
  %s '英文讀書會時間 Ver.4.csv'              # display the optimal matches\n\
  %s -s '英文讀書會時間 Ver.4.csv'           # show the csv data only\n\
  %s -g '英文讀書會時間 Ver.4.csv'           # generate 'relations.png' pairing graph\n\
  %s -e 'Bob' '英文讀書會時間 Ver.4.csv'     # match Bob first\n\n\
For more information, see <https://github.com/jackiesogi/pairup.c>.\n\
", program_name, program_name, program_name, program_name, program_name);
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

static char const short_options[] = "d:sg::e:jp:vhF";

static struct option const long_options[] =
{
    {"show-csv", no_argument, NULL, 's'},
    {"graph", optional_argument, NULL, 'g'},
    {"priority", required_argument, NULL, 'p'},
    {"ensure", required_argument, NULL, 'e'},
    {"json-output", no_argument, NULL, 'j'},
    {"enable-flex", no_argument, NULL, 'F'},
    {"debug", required_argument, NULL, 'd'},
    {"version", no_argument, NULL, 'v'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

void
sigsegv_handler (int sig) {
    (void) sig;
    fprintf (stdout, "The worksheet format is incorrect. Please check the content on Google Sheets with the following format rules, or contact the developer:\n\
(1) No extra characters should appear outside the main table.\n\
(2) There should be no empty rows between members.\n\
(3) There should be no empty columns between the time slots.\n\
(4) The number of members should not exceed %d. (If so, contact the developer to scale it up.)\n", PAIRUP_MAX_MEMBERS);
    exit(EXIT_FAILURE);
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
    static struct pairup_options x;  /* static: the ensure list is large */
    pairup_options_init (&x);

    /* Parse the command line arguments using while loop */
    int c;
    while ((c = getopt_long (argc, argv, short_options, long_options, NULL)) != -1)
    {
        switch (c)
        {
            case 'd':
                debug_level = parse_debug_level (optarg);
                break;
            case 'g':
                if (!has_installed_graphviz())
                {
                    debug_printf (DEBUG_ERROR, "[ ERROR   ] Please install package `graphviz` first\n");
                    exit (EXIT_FAILURE);
                }
                x.generate_graph = true;
                snprintf (x.graph_output, sizeof(x.graph_output), "%s",
                          optarg ? optarg : "relations.png");
                break;
            case 's':
                x.show_csv = true;
                break;
            case 'e':
                if (!pairup_options_add_ensure (&x, optarg))
                    fprintf (stderr, "%s: too many --ensure members, ignoring '%s'\n",
                             program_name, optarg);
                break;
            case 'j':
                x.json_output = true;
                break;
            case 'F':
                x.enable_flex = true;
                break;
            case 'p':
                fprintf (stderr, "%s: --priority is deprecated and ignored, "
                         "the solver always finds the maximum matching\n", program_name);
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
    char err[256] = "";

    if (x.generate_graph)
    {
        pairup_problem *problem = malloc (sizeof(*problem));
        if (!problem || pairup_problem_load (&worksheet, &x, problem, err, sizeof(err)) != 0)
        {
            fprintf (stderr, "%s: %s\n", program_name, problem ? err : "out of memory");
            free (problem);
            exit (EXIT_FAILURE);
        }
        generate_graph_output_image (problem, x.graph_output);
        free (problem);
        return 0;
    }

    if (x.show_csv)
    {
        print_worksheet (&worksheet);
        return 0;
    }

    /* Randomize the worksheet rows so ties between optimal results are broken fairly */
    debug_printf(DEBUG_INFO, "\
[ INFO    ] Shuffling each row inside the input worksheet to avoid bias result ...\n");
    shuffle_worksheet (&worksheet, time(NULL));
    debug_printf(DEBUG_INFO, "[ INFO    ] Finished shuffling.\n");

    /* Trigger the top-level pairup function */
    debug_printf(DEBUG_INFO, "[ INFO    ] Starting the pairing up process ...\n");
    pairup_result *result = pairup_run (&worksheet, &x, err, sizeof(err));
    if (!result)
    {
        fprintf (stderr, "%s: %s\n", program_name, err);
        exit (EXIT_FAILURE);
    }

    /* Print the result */
    if (x.json_output == true)
    {
        cJSON *root = init_result_json_object (result);
        char *json_output = get_result_json_string (root);
        fprintf (stdout, "%s\n", json_output);
        free_result_json_object (root);
        free_result_json_string (json_output);
    }
    else
    {
        print_result (result);
    }

    pairup_result_free (result);
    debug_printf(DEBUG_INFO, "[ INFO    ] Done!\n");

    return 0;
}
