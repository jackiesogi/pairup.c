#ifndef RW_CSV_H
#define RW_CSV_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct sheet_struct
{
    char *path;
    int rows;
    int cols;
    char ***data;
} sheet_t;

sheet_t
read_csv (const char *path);

int
write_csv (sheet_t *sheet,
           const char *path);

char *
get_token (FILE *stream,
           char *buffer,
           size_t buffer_size);

int
_get_rcount (FILE *file);

int
_get_ccount (FILE *file);

void
free_sheet (sheet_t **sheet);

void
set_cell (sheet_t *sheet,
          int row,
          int col,
          char *buf,
          int buf_size);

void
get_cell (sheet_t *sheet,
          int row,
          int col,
          char *buf,
          int buf_size);
void
get_cell_from_one (sheet_t *sheet,
                   int row,
                   int col,
                   char *buf,
                   int buf_size);

#endif  // RW_CSV_H
