#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
        emit_try_help ();
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
  -d, --debug={LEVEL}         set the debug level (0: only error, 5: all info)\n\
  -p, --priority={FUNC}       specify the match priority algorithm\n\
  -v, --version               print the version information\n\
  -h, --help                  print this page\n\n\
Examples:\n\
  %s '英文讀書會時間 Ver.4.csv'              # display the optimal matches\n\
  %s -s '英文讀書會時間 Ver.4.csv'           # show the csv data only\n\
  %s -g '英文讀書會時間 Ver.4.csv'           # generate 'relations.png' pairing graph\n\
  %s -p LAST_ROW '英文讀書會時間 Ver.4.csv'  # match member from the last row\n\
  %s -e 'Bob' '英文讀書會時間 Ver.4.csv'     # match Bob first\n\n\
For more information, see <https://github.com/jackiesogi/pairup.c>.\n\
", program_name, program_name, program_name, program_name, program_name, program_name);
    }
    exit (status);
}

bool
has_installed_graphviz()
{
    if (system("which dot > /dev/null") == 0)
        return true;
    return false;
}

static char const short_options[] = "d:sg::e:p:vh";

static struct option const long_options[] =
{
    {"show-csv", no_argument, NULL, 's'},
    {"graph", optional_argument, NULL, 'g'},
    {"priority", required_argument, NULL, 'p'},
    {"ensure", required_argument, NULL, 'e'},
    {"debug", required_argument, NULL, 'd'},
    {"version", no_argument, NULL, 'v'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

int
main(int argc, char *argv[])
{
    /* Flags for command line arguments */
    struct pairup_options x;
    pairup_options_init(&x);
    char graph_output[1024];

    /* Parse the command line arguments using while loop */
    int c;
    while ((c = getopt_long (argc, argv, short_options, long_options, NULL)) != -1)
    {
        switch (c)
        {
            case 'd':
                x.debug_level = atoi(optarg);
                debug_level = atoi(optarg);
                break;
            case 'g':
                if (!has_installed_graphviz())
                {
                    fprintf(stderr, "Please install package `graphviz` first\n");
                    exit(EXIT_FAILURE);
                }
                x.generate_graph = true;
                if (optarg)
                    strcpy(graph_output, (const char *)optarg);
                else
                    strcpy(graph_output, "relations.png");
                break;
            case 's':
                x.show_csv = true;
                break;
            case 'e':
                x.ensure = true;
                printf("Not implemented yet\n");
                exit(EXIT_FAILURE);
                break;
            case 'p':
                x.priority = true;
                printf("Not implemented yet\n");
                exit(EXIT_FAILURE);
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

    /* Randomize the worksheet rows to avoid bias */
    shuffle_worksheet (&worksheet, time(NULL));

    /* Trigger the top-level pairup function */
    pair_result_t *result = pairup (&worksheet);

    /* Print the result */
    print_result (&worksheet, result);

    free_pair_result (result);

    return 0;
}
