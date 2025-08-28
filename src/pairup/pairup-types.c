#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pairup-types.h"
#include "cJSON.h"

/********************************  Number of practices  *********************************/

const char *zero_sign[] = {
    "0",
    "\xEF\xBC\x90", // "０" (U+FF10)
    " "
};

const char *once_sign[] = {
    "1",
    "\xEF\xBC\x91", // "１"
    "V", "v", "X", "x",
    "\xEF\xBC\xB6", // "Ｖ"
    "\xEF\xBD\x96", // "ｖ"
    "\xEF\xBC\xB8", // "Ｘ"
    "\xEF\xBD\x98", // "ｘ"
    "once"
};

const char *twice_sign[] = {
    "2",
    "\xEF\xBC\x92", // "２"
    "twice"
};

/**********************************  Helper functions  **********************************/

/* Remove spaces from the string */
void remove_spaces (const char *str, char *result)
{
    while (*str != '\0')
    {
        if (*str != ' ')
        {
            *result++ = *str;
        }
        str++;
    }
    *result = '\0';
}
/* ---------------- History JSON helpers ---------------- */
typedef struct history_entry {
    char host[MAX_NAME_LEN];
    size_t partner_count;
    char partners[MAX_MEMBERS_LEN][MAX_NAME_LEN];
} history_entry;

typedef struct history_week_bucket {
    char week[128];
    size_t entry_count;
    history_entry entries[MAX_MEMBERS_LEN];
} history_week_bucket;

typedef struct history_ctx_impl {
    size_t week_count;
    history_week_bucket weeks[64];
} history_ctx_impl;

static history_ctx_impl *history_new_ctx () { return (history_ctx_impl*)calloc(1, sizeof(history_ctx_impl)); }

static history_week_bucket *history_get_or_create_week (history_ctx_impl *ctx, const char *week)
{
    for (size_t i = 0; i < ctx->week_count; ++i)
        if (strncmp(ctx->weeks[i].week, week, sizeof(ctx->weeks[i].week)) == 0)
            return &ctx->weeks[i];
    if (ctx->week_count >= 64) return NULL;
    history_week_bucket *b = &ctx->weeks[ctx->week_count++];
    strncpy(b->week, week, sizeof(b->week)-1);
    b->week[sizeof(b->week)-1] = '\0';
    b->entry_count = 0;
    return b;
}

static history_entry *history_get_or_create_entry (history_week_bucket *b, const char *host)
{
    for (size_t i = 0; i < b->entry_count; ++i)
        if (strcmp(b->entries[i].host, host) == 0)
            return &b->entries[i];
    if (b->entry_count >= MAX_MEMBERS_LEN) return NULL;
    history_entry *e = &b->entries[b->entry_count++];
    strncpy(e->host, host, sizeof(e->host)-1);
    e->host[sizeof(e->host)-1] = '\0';
    e->partner_count = 0;
    return e;
}

void *history_load (const char *path)
{
    FILE *f = fopen(path, "r");
    history_ctx_impl *ctx = history_new_ctx();
    if (!f)
        return ctx; /* start empty */
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = (char*)malloc(len + 1);
    if (!buf) { fclose(f); return ctx; }
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return ctx;

    cJSON *weeks = cJSON_GetObjectItem(root, "weeks");
    if (!cJSON_IsArray(weeks)) { cJSON_Delete(root); return ctx; }
    cJSON *w;
    cJSON_ArrayForEach(w, weeks)
    {
        cJSON *week = cJSON_GetObjectItem(w, "week");
        cJSON *items = cJSON_GetObjectItem(w, "items");
        if (!cJSON_IsString(week) || !cJSON_IsArray(items)) continue;
        history_week_bucket *b = history_get_or_create_week(ctx, week->valuestring);
        if (!b) continue;
        cJSON *it;
        cJSON_ArrayForEach(it, items)
        {
            cJSON *host = cJSON_GetObjectItem(it, "host");
            cJSON *partners = cJSON_GetObjectItem(it, "already_paired");
            if (!cJSON_IsString(host) || !cJSON_IsArray(partners)) continue;
            history_entry *e = history_get_or_create_entry(b, host->valuestring);
            if (!e) continue;
            cJSON *p;
            cJSON_ArrayForEach(p, partners)
            {
                if (cJSON_IsString(p) && e->partner_count < MAX_MEMBERS_LEN)
                {
                    strncpy(e->partners[e->partner_count], p->valuestring, MAX_NAME_LEN-1);
                    e->partners[e->partner_count][MAX_NAME_LEN-1] = '\0';
                    e->partner_count++;
                }
            }
        }
    }
    cJSON_Delete(root);
    return ctx;
}

static bool history_entry_contains (const history_entry *e, const char *name)
{
    for (size_t i = 0; i < e->partner_count; ++i)
        if (strcmp(e->partners[i], name) == 0)
            return true;
    return false;
}

bool history_was_paired (void *vctx, const char *week, const char *a, const char *b)
{
    if (!vctx || !week || !a || !b) return false;
    history_ctx_impl *ctx = (history_ctx_impl*)vctx;
    for (size_t i = 0; i < ctx->week_count; ++i)
    {
        if (strcmp(ctx->weeks[i].week, week) != 0) continue;
        history_week_bucket *wb = &ctx->weeks[i];
        for (size_t j = 0; j < wb->entry_count; ++j)
        {
            if (strcmp(wb->entries[j].host, a) == 0)
                return history_entry_contains(&wb->entries[j], b);
            if (strcmp(wb->entries[j].host, b) == 0)
                return history_entry_contains(&wb->entries[j], a);
        }
    }
    return false;
}

static cJSON *history_to_json (history_ctx_impl *ctx)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *weeks = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "weeks", weeks);
    for (size_t i = 0; i < ctx->week_count; ++i)
    {
        history_week_bucket *wb = &ctx->weeks[i];
        cJSON *w = cJSON_CreateObject();
        cJSON_AddStringToObject(w, "week", wb->week);
        cJSON *items = cJSON_CreateArray();
        cJSON_AddItemToObject(w, "items", items);
        for (size_t j = 0; j < wb->entry_count; ++j)
        {
            history_entry *e = &wb->entries[j];
            cJSON *obj = cJSON_CreateObject();
            cJSON_AddStringToObject(obj, "host", e->host);
            cJSON *arr = cJSON_CreateArray();
            for (size_t k = 0; k < e->partner_count; ++k)
                cJSON_AddItemToArray(arr, cJSON_CreateString(e->partners[k]));
            cJSON_AddItemToObject(obj, "already_paired", arr);
            cJSON_AddItemToArray(items, obj);
        }
        cJSON_AddItemToArray(weeks, w);
    }
    return root;
}

int history_save_update (void *vctx, const char *path, const char *week, pair_result *r)
{
    if (!vctx || !path || !week || !r) return -1;
    history_ctx_impl *ctx = (history_ctx_impl*)vctx;
    history_week_bucket *wb = history_get_or_create_week(ctx, week);
    if (!wb) return -1;
    for (int i = 0; i < r->pairs; ++i)
    {
        const char *a = r->pair_list[i]->a->name;
        const char *b = r->pair_list[i]->b->name;
        history_entry *ea = history_get_or_create_entry(wb, a);
        history_entry *eb = history_get_or_create_entry(wb, b);
        if (ea && !history_entry_contains(ea, b) && ea->partner_count < MAX_MEMBERS_LEN)
        {
            strncpy(ea->partners[ea->partner_count], b, MAX_NAME_LEN-1);
            ea->partners[ea->partner_count][MAX_NAME_LEN-1] = '\0';
            ea->partner_count++;
        }
        if (eb && !history_entry_contains(eb, a) && eb->partner_count < MAX_MEMBERS_LEN)
        {
            strncpy(eb->partners[eb->partner_count], a, MAX_NAME_LEN-1);
            eb->partners[eb->partner_count][MAX_NAME_LEN-1] = '\0';
            eb->partner_count++;
        }
    }
    cJSON *root = history_to_json(ctx);
    char *json = cJSON_Print(root);
    FILE *f = fopen(path, "w");
    if (f)
    {
        fputs(json, f);
        fclose(f);
    }
    cJSON_free(json);
    cJSON_Delete(root);
    return 0;
}

/* Convert the string to lower case */
void to_upper (const char *str, char *result)
{
    while (*str != '\0')
    {
        *result++ = toupper (*str);
        str++;
    }
    *result = '\0';
}

/* Normalize the string */
void normalize (const char *str, char *result)
{
    remove_spaces (str, result);
    to_upper (result, result);
}

/* Check if the sign represents 'practice zero time' */
bool is_zero (const char *sign)
{
    char src[10], dst[10];
    for (int i = 0; i < sizeof(zero_sign) / sizeof(zero_sign[0]); i++)
    {
        normalize (sign, src);
        normalize (zero_sign[i], dst);
        if (strcmp(src, dst) == 0)
        {
            return true;
        }
    }
    return false;
}

/* Check if the sign represents 'practice one time' */
bool is_once (const char *sign)
{
    char src[10], dst[10];
    for (int i = 0; i < sizeof(once_sign) / sizeof(once_sign[0]); i++)
    {
        normalize (sign, src);
        normalize (once_sign[i], dst);
        if (strcmp(src, dst) == 0)
        {
            return true;
        }
    }
    return false;
}

/* Check if the sign represents 'practice two times' */
bool is_twice (const char *sign)
{
    char src[10], dst[10];
    for (int i = 0; i < sizeof(twice_sign) / sizeof(twice_sign[0]); i++)
    {
        normalize (sign, src);
        normalize (twice_sign[i], dst);
        if (strcmp(src, dst) == 0)
        {
            return true;
        }
    }
    return false;
}

/* Check if the sign represents 'available' */
bool is_available (const char *sign)
{
    // return (!is_zero(sign) && (is_once(sign) || is_twice(sign)));
    return (is_once(sign) || is_twice(sign));
}

/****************************  Allocator and Deallocator  ********************************/

void
pairup_options_init (struct pairup_options *x)
{
    x->show_csv = false;
    x->generate_graph = false;
    x->ensure = false;
    x->priority = false;
    x->debug_level = 2;
    x->json_output = false;
    x->avoid_same_match = false;
    x->avoid_week[0] = '\0';
    x->history_path[0] = '\0';
    x->history_ctx = NULL;
}

member_t *
new_member (void)
{
    return (member_t *) malloc (sizeof(member_t));
}

void
free_member (member_t *m)
{
    if (!m)
    {
        return;
    }

    free (m);
}

pair *
new_pair (void)
{
    return (pair *) malloc (sizeof(pair));
}

void
free_pair (pair *p)
{
    if (!p)
    {
        return;
    }

    free (p);
}

relation *
new_relation (void)
{
    return (relation *) malloc (sizeof(relation));
}

void
free_relation (relation *r)
{
    if (!r)
    {
        return;
    }
    // printf("relation address: %p\n", r);
    free (r);
}

relation_graph *
new_relation_graph (void)
{
    return (relation_graph *) malloc (sizeof(relation_graph));
}

void
free_relation_graph (relation_graph *graph)
{
    // printf("graph address: %p\n", graph);
    if (!graph)
    {
        return;
    }

    for (int i = 0; i < graph->count; i++)
    {
        free_relation (graph->relations[i]);
        graph->relations[i] = NULL;
    }

    free (graph);
}

pair_result *
new_pair_result (int pairs,
                 int singles,
                 int members)
{
    pair_result *result = (pair_result *) malloc (sizeof(pair_result));
    if (result == NULL)
    {
        fprintf (stderr, "Memory allocation failed for pair_result\n");
        return NULL;
    }

    result->member = members;
    result->singles = singles;
    result->pairs = pairs;
    result->algorithm_applied = NULL;

    /* Currently the member graph, single_list, pair_list are not dynamically allocated */
    return result;
}

void
free_pair_result (pair_result *result)
{
    if (!result)
    {
        return;
    }

    free (result);
}
