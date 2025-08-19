#ifndef PORTABLE_WCWIDTH_H
#define PORTABLE_WCWIDTH_H

#include <wchar.h>

#ifdef _WIN32
#include <windows.h>

static inline int portable_wcwidth(wchar_t wc) {
    WORD type;
    if (GetStringTypeW(CT_CTYPE3, &wc, 1, &type)) {
        if (type & C3_FULLWIDTH) return 2;
        else return 1;
    }
    return 1;
}

#else
static inline int portable_wcwidth(wchar_t wc) {
    return wcwidth(wc);
}
#endif

#endif // PORTABLE_WCWIDTH_H
