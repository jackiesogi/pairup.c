#include <ctype.h>
#include <string.h>

#include "pairup-types.h"

/********************************  Number of practices  *********************************/

const char *zero_sign[] = {
    "0",
    "\xEF\xBC\x90", // "０" (U+FF10)
    " ",
    NULL
};

const char *once_sign[] = {
    "1",
    "\xEF\xBC\x91", // "１"
    "V", "v", "X", "x",
    "\xEF\xBC\xB6", // "Ｖ"
    "\xEF\xBD\x96", // "ｖ"
    "\xEF\xBC\xB8", // "Ｘ"
    "\xEF\xBD\x98", // "ｘ"
    "once",
    NULL
};

const char *twice_sign[] = {
    "2",
    "\xEF\xBC\x92", // "２"
    "twice",
    NULL
};

/**********************************  Helper functions  **********************************/

/* Drop spaces and upper-case ASCII letters, writing at most `size - 1` bytes */
static void
normalize (const char *str,
           char *out,
           size_t size)
{
    size_t n = 0;
    for (; *str != '\0' && n + 1 < size; str++)
    {
        unsigned char c = (unsigned char) *str;
        if (c == ' ' || c == '\r' || c == '\n' || c == '\t')
            continue;
        out[n++] = (char) toupper (c);
    }
    out[n] = '\0';
}

static bool
matches_any (const char *sign,
             const char *const list[])
{
    char src[32], dst[32];

    if (!sign)
        return false;

    normalize (sign, src, sizeof(src));
    for (int i = 0; list[i] != NULL; i++)
    {
        normalize (list[i], dst, sizeof(dst));
        if (strcmp (src, dst) == 0)
            return true;
    }
    return false;
}

bool is_zero (const char *sign)  { return matches_any (sign, zero_sign); }
bool is_once (const char *sign)  { return matches_any (sign, once_sign); }
bool is_twice (const char *sign) { return matches_any (sign, twice_sign); }

bool is_available (const char *sign)
{
    return is_once (sign) || is_twice (sign);
}

/*************************************  Options  ****************************************/

void
pairup_options_init (struct pairup_options *x)
{
    memset (x, 0, sizeof(*x));
    x->time_limit = 60.0;
}

bool
pairup_options_add_ensure (struct pairup_options *x,
                           const char *name)
{
    if (x->n_ensure >= PAIRUP_MAX_ENSURE)
        return false;

    strncpy (x->ensure[x->n_ensure], name, PAIRUP_MAX_NAME - 1);
    x->ensure[x->n_ensure][PAIRUP_MAX_NAME - 1] = '\0';
    x->n_ensure++;
    return true;
}
