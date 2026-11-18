/*
 * Solver tests:
 *   1. Random small problems: the ILP result must match an exhaustive search
 *      on the same lexicographic objective (ensured members, pairs, fixed slots).
 *   2. Loading a sheet: header detection, flex column, requests, summary row.
 *   3. Merging consecutive slots into time ranges.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pairup/pairup.h"

static int failures = 0;

#define CHECK(cond, ...)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            fprintf (stderr, "FAIL %s:%d: ", __FILE__, __LINE__); \
            fprintf (stderr, __VA_ARGS__);                \
            fprintf (stderr, "\n");                       \
            failures++;                                   \
        }                                                 \
    } while (0)

/******************************  Exhaustive search  **********************************/

static uint32_t rng_state = 12345;

static uint32_t
rnd (void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

typedef struct
{
    const pairup_problem *p;
    int flex;
    int cap[PAIRUP_MAX_MEMBERS];
    uint32_t free_slots[PAIRUP_MAX_MEMBERS];
    int paired[PAIRUP_MAX_MEMBERS];
    bool met[PAIRUP_MAX_MEMBERS][PAIRUP_MAX_MEMBERS];
    int pairs, fixed;
    long best;
} search_t;

static long
score (const pairup_problem *p, const int *paired, int pairs, int fixed)
{
    long ensured = 0;
    for (int m = 0; m < p->n_members; m++)
        ensured += (p->members[m].ensured && paired[m] > 0);
    return ensured * 1000000L + pairs * 1000L + fixed;
}

static void
dfs (search_t *s, int i)
{
    const pairup_problem *p = s->p;

    while (i < p->n_members && s->cap[i] == 0)
        i++;
    if (i == p->n_members)
    {
        long v = score (p, s->paired, s->pairs, s->fixed);
        if (v > s->best)
            s->best = v;
        return;
    }

    /* Option 1: member i takes no more partners */
    int saved = s->cap[i];
    s->cap[i] = 0;
    dfs (s, i + 1);
    s->cap[i] = saved;

    /* Option 2: pair i with a later member j in a shared free slot */
    for (int j = i + 1; j < p->n_members; j++)
    {
        if (s->cap[j] == 0 || s->met[i][j])
            continue;
        uint32_t common = s->free_slots[i] & s->free_slots[j];
        for (int t = 0; t < p->n_slots; t++)
        {
            uint32_t bit = (uint32_t) 1 << t;
            if (!(common & bit))
                continue;

            s->cap[i]--; s->cap[j]--;
            s->free_slots[i] &= ~bit; s->free_slots[j] &= ~bit;
            s->paired[i]++; s->paired[j]++;
            s->met[i][j] = true;
            s->pairs++; s->fixed += (t != s->flex);

            dfs (s, i);

            s->pairs--; s->fixed -= (t != s->flex);
            s->met[i][j] = false;
            s->paired[i]--; s->paired[j]--;
            s->free_slots[i] |= bit; s->free_slots[j] |= bit;
            s->cap[i]++; s->cap[j]++;
        }
    }
}

static long
brute_force (const pairup_problem *p)
{
    static search_t s;
    memset (&s, 0, sizeof(s));
    s.p = p;
    s.flex = pairup_flex_slot (p);
    s.best = -1;
    for (int m = 0; m < p->n_members; m++)
    {
        s.cap[m] = p->members[m].requests;
        s.free_slots[m] = p->members[m].avail;
    }
    dfs (&s, 0);
    return s.best;
}

static void
random_problem (pairup_problem *p)
{
    memset (p, 0, sizeof(*p));
    p->n_slots = 1 + rnd () % 6;
    for (int t = 0; t < p->n_slots; t++)
    {
        snprintf (p->slots[t].label, PAIRUP_MAX_LABEL, "S%d", t);
        p->slots[t].col = 2 + t;
    }
    if (rnd () % 2)
        p->slots[p->n_slots - 1].is_flex = true;

    p->n_members = 2 + rnd () % 6;
    for (int m = 0; m < p->n_members; m++)
    {
        pairup_member *x = &p->members[m];
        snprintf (x->name, PAIRUP_MAX_NAME, "m%d", m);
        x->requests = 1 + rnd () % 2;
        do {
            x->avail = rnd () & (((uint32_t) 1 << p->n_slots) - 1);
        } while (x->avail == 0);
        x->ensured = (rnd () % 7 == 0);
    }
}

static void
test_random_problems (void)
{
    static pairup_problem p;
    static pairup_result r;
    char err[256];

    for (int k = 0; k < 2000; k++)
    {
        random_problem (&p);
        if (pairup_solve (&p, 0, &r, err, sizeof(err)) != 0)
        {
            CHECK (0, "case %d: solver failed: %s", k, err);
            continue;
        }

        int paired[PAIRUP_MAX_MEMBERS] = { 0 };
        int fixed = 0;
        int flex = pairup_flex_slot (&p);
        for (int i = 0; i < r.n_pairs; i++)
        {
            paired[r.pairs[i].a]++;
            paired[r.pairs[i].b]++;
            fixed += (r.pairs[i].slot != flex);
        }

        long got = score (&p, paired, r.n_pairs, fixed);
        long want = brute_force (&p);
        CHECK (got == want, "case %d: solver score %ld, exhaustive search %ld", k, got, want);
        CHECK (r.status == PAIRUP_OPTIMAL, "case %d: not proven optimal", k);
    }
}

/*******************************  Sheet loading  *************************************/

static void
test_load_sheet (void)
{
    const char *path = "test_sheet.csv";
    FILE *f = fopen (path, "w");
    CHECK (f != NULL, "cannot write %s", path);
    if (!f) return;

    fputs (",,1700~1730,1730~1800,1800~1830,FLEX,\r\n", f);
    fputs ("Alice,,1,1,,,Alice\r\n", f);
    fputs ("Bob,,,2,2,,Bob\r\n", f);
    fputs ("Carol,,,,,1,Carol\r\n", f);
    fputs ("Dave,,,,,,Dave\r\n", f);
    fputs ("填答數,,1,2,1,1,填答數\r\n", f);
    /* Stray marks below the table: no name, so they must be ignored */
    fputs (",,,2,2,,\r\n", f);
    fputs (",,,,v,,", f);
    fclose (f);

    sheet_t ws = read_csv (path);
    static struct pairup_options opt;
    static pairup_problem p;
    char err[256];

    pairup_options_init (&opt);
    CHECK (pairup_problem_load (&ws, &opt, &p, err, sizeof(err)) == 0, "load failed: %s", err);
    CHECK (p.n_slots == 3, "expected 3 slots without flex, got %d", p.n_slots);
    CHECK (p.n_members == 2, "expected Alice and Bob, got %d members", p.n_members);
    CHECK (strcmp (p.members[0].name, "Alice") == 0 && p.members[0].requests == 1 &&
           p.members[0].avail == 0x3, "Alice parsed wrong");
    CHECK (strcmp (p.members[1].name, "Bob") == 0 && p.members[1].requests == 2 &&
           p.members[1].avail == 0x6, "Bob parsed wrong");

    opt.enable_flex = true;
    CHECK (pairup_problem_load (&ws, &opt, &p, err, sizeof(err)) == 0, "load failed: %s", err);
    CHECK (p.n_slots == 4 && p.slots[3].is_flex, "flex slot missing");
    CHECK (p.n_members == 3 && strcmp (p.members[2].name, "Carol") == 0 &&
           p.members[2].avail == 0x8, "Carol (flex only) parsed wrong");

    remove (path);
}

static void
test_ranges (void)
{
    static pairup_problem p;
    memset (&p, 0, sizeof(p));
    const char *labels[] = { "1700~1730", "1730~1800", "1800~1830", "1900~1930" };
    const int cols[] = { 2, 3, 4, 6 };
    p.n_slots = 4;
    for (int t = 0; t < 4; t++)
    {
        snprintf (p.slots[t].label, PAIRUP_MAX_LABEL, "%s", labels[t]);
        p.slots[t].col = cols[t];
    }
    p.n_members = 1;
    p.members[0].avail = 0xB;  /* slots 0, 1, 3 */

    char ranges[PAIRUP_MAX_SLOTS][PAIRUP_MAX_RANGE];
    int n = pairup_member_ranges (&p, 0, ranges, PAIRUP_MAX_SLOTS);
    CHECK (n == 2, "expected 2 ranges, got %d", n);
    CHECK (n >= 1 && strcmp (ranges[0], "1700~1800") == 0, "range 0 is '%s'", ranges[0]);
    CHECK (n >= 2 && strcmp (ranges[1], "1900~1930") == 0, "range 1 is '%s'", ranges[1]);
}

int
main (void)
{
    test_random_problems ();
    test_load_sheet ();
    test_ranges ();

    if (failures)
    {
        fprintf (stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf ("all tests passed\n");
    return 0;
}
