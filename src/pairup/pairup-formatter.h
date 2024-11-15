#ifndef PAIRUP_FORMATTER_H
#define PAIRUP_FORMATTER_H

#include "pairup-types.h"

#define GREETING "Enjoy the chat :D"

#define ALTERNATIVES_FOR_NOT_MATCHED " \n\
you can choose to \n\
1) take a day off (count out) \n\
2) requesting for partners \n\
3) leave a 4-minute up voice message and answer questions \n\
related to weekly topic. ONLY on Monday can talk about \n\
your last weekend or sharing something interesting."

void
print_result (pair_result_t *result);
		   
void
print_result_full (pair_result_t *result);

#endif  // PAIRUP_FORMATTER_H
