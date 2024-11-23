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

char *get_token(FILE *stream, char *buffer, size_t buffer_size);
int _get_rcount(FILE *file);
int _get_ccount(FILE *file);

sheet_t
read_csv (const char *path)
{
    FILE *file = fopen(path, "r");
    if (!file)
    {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    sheet_t sheet;
    sheet.path = strdup(path);
    sheet.rows = _get_rcount(file);
    sheet.cols = _get_ccount(file);

    sheet.data = (char ***) malloc (sheet.rows * sizeof(char **));
    if (!sheet.data)
    {
        perror("Failed to allocate data array");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_SET);
    char buffer[1024];
    for (int i = 0; i < sheet.rows; ++i)
    {
        sheet.data[i] = (char **) malloc(sheet.cols * sizeof(char *));
        if (!sheet.data[i])
        {
            perror("Failed to allocate row");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < sheet.cols; ++j)
        {
            sheet.data[i][j] = get_token(file, buffer, sizeof(buffer));
        }
    }

    fclose(file);
    return sheet;
}

int
write_csv (sheet_t *sheet,
           const char *path)
{
    FILE *file = fopen(path, "w");
    if (!file)
    {
        perror("Failed to open file");
        return -1;
    }

    for (int i = 0; i < sheet->rows; ++i)
    {
        for (int j = 0; j < sheet->cols; ++j)
        {
            fprintf(file, "%s", sheet->data[i][j]);

            if (j < sheet->cols - 1) {
                fprintf(file, ",");
            }
        }
        fprintf(file, "\n");
    }

    fclose(file);
    return 0;
}

char *
get_token (FILE *stream,
           char *buffer,
           size_t buffer_size)
{
    int i = 0;
    char ch;

    while ((ch = fgetc(stream)) != ',' && ch != '\n' && ch != EOF)
    {
        if (i < buffer_size - 1)
        {
            buffer[i++] = ch;
        }
    }
    buffer[i] = '\0';

    if (i == 0)
    {
        return NULL;
    }

    char *token = strdup(buffer);
    if (!token)
    {
        perror("Failed to allocate token");
        exit(EXIT_FAILURE);
    }

    return token;
}

/* Get the number of rows in the file */
int
_get_rcount (FILE *file)
{
    int count = 0;
    char ch;
    int has_content = 0;

    while ((ch = fgetc(file)) != EOF) {
        has_content = 1;
        if (ch == '\n') {
            count++;
        }
    }

    if (has_content && ch != '\n') {
        count++;
    }

    fseek(file, 0, SEEK_SET);
    return count;
}

int
_get_ccount (FILE *file)
{
    int count = 0;
    char ch;
    
    while ((ch = fgetc(file)) != EOF && ch != '\n')
    {
        if (ch == ',')
        {
            count++;
        }
    }
    
    fseek(file, 0, SEEK_SET);
    return count + 1;
}

void free_sheet(sheet_t **sheet)
{
    if (!sheet || !*sheet)
    {
        return;
    }

    for (int i = 0; i < (*sheet)->rows; ++i)
    {
        for (int j = 0; j < (*sheet)->cols; ++j)
        {
            free((*sheet)->data[i][j]);
            (*sheet)->data[i][j] = NULL;
        }
        free((*sheet)->data[i]);
        (*sheet)->data[i] = NULL;
    }
    free((*sheet)->data);
    (*sheet)->data = NULL;

    free((*sheet)->path);
    (*sheet)->path = NULL;

    free(*sheet);
    *sheet = NULL;
}

void
set_cell (sheet_t *sheet,
          int row,
          int col,
          char *buf,
          int buf_size)
{
    if (row <= 0 || row > sheet->rows || col <= 0 || col > sheet->cols)
    {
        fprintf(stderr, "Invalid cell position\n");
        return;
    }

    char *cell = sheet->data[row - 1][col - 1];
    strncpy(cell, buf, buf_size - 1);
    cell[buf_size - 1] = '\0';
}

void
get_cell_from_one (sheet_t *sheet,
                   int row,
                   int col,
                   char *buf,
                   int buf_size)
{
    if (row <= 0 || row > sheet->rows || col <= 0 || col > sheet->cols)
    {
        fprintf(stderr, "Invalid cell position\n");
        return;
    }

    char *cell = sheet->data[row - 1][col - 1];

    if (cell == NULL)
    {
        snprintf(buf, buf_size, "");
    }
    else
    {
        strncpy(buf, cell, buf_size - 1);
        buf[buf_size - 1] = '\0';
    }
}

void
get_cell (sheet_t *sheet,
          int row,
          int col,
          char *buf,
          int buf_size)
{
    if (row <= 0 || row > sheet->rows || col <= 0 || col > sheet->cols)
    {
        fprintf(stderr, "Invalid cell position\n");
        return;
    }

    char *cell = sheet->data[row][col];

    if (cell == NULL)
    {
        snprintf(buf, buf_size, "");
    }
    else
    {
        strncpy(buf, cell, buf_size - 1);
        buf[buf_size - 1] = '\0';
    }
}
