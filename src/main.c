#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "pairup/pairup-formatter.h"
#include "pairup/pairup.h"
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
Generate optimal matches based on member's available time with linear time complexity\n\
\n\
  -s, --show-csv              show the csv data only (no pair result)\n\
  -e, --ensure={MEMBER}       ensure specified member can have partner today\n\
  -d, --debug={LEVEL}         set the debug level (0: only error, 5: all info)\n\
  -p, --priority={FUNC}       specify the match priority algorithm\n\
  -v, --version               print the version information\n\
  -h, --help                  print this page\n\
Examples:\n\
  %s '英文讀書會時間 Ver.4.csv'              # display the optimal matches\n\
  %s -s '英文讀書會時間 Ver.4.csv'           # show the csv data only\n\
  %s -p LAST_ROW '英文讀書會時間 Ver.4.csv'  # match member from the last row\n\
  %s -e 'Bob' '英文讀書會時間 Ver.4.csv'     # match Bob first\n\
", program_name, program_name, program_name, program_name, program_name);
    }
    exit (status);
}

static char const short_options[] = "d:se:p:vh";

static struct option const long_options[] =
{
    {"show-csv", no_argument, NULL, 's'},
    {"priority", required_argument, NULL, 'p'},
    {"debug", required_argument, NULL, 'd'},
    {"version", no_argument, NULL, 'v'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}
};

int
main(int argc, char *argv[])
{
    // sheet_t worksheet = read_csv ("data.csv");
    // print_worksheet(&worksheet);
    //
    // pair_result_t *result = pairup(&worksheet);
    //
    // print_result (&worksheet, result);

    bool show_csv = false;
    bool ensure = false;
    bool priority = false;

    /* Parse the command line arguments using while loop */
    int c;
    while ((c = getopt_long (argc, argv, short_options, long_options, NULL)) != -1)
    {
        switch (c)
        {
            case 'd':
                debug_level = atoi(optarg);
                break;
            case 's':
                show_csv = true;
                break;
            case 'e':
                ensure = true;
                break;
            case 'p':
                priority = true;
                break;
            case 'v':
                printf ("Version 0.1\n");
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

    sheet_t worksheet = read_csv (path);

    if (show_csv)
    {
        print_worksheet(&worksheet);
        return 0;
    }
    
    pair_result_t *result = pairup(&worksheet);

    print_result (&worksheet, result);

    return 0;
}
