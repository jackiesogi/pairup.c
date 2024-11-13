#ifndef PAIRUP_FORMATTER_H
#define PAIRUP_FORMATTER_H

#define GREETING "Enjoy the chat :D"

#define ALTERNATIVES_FOR_NOT_MATCHED " \n\
As for \n\
\n\
you can choose to \n\
1) take a day off (count out) \n\
2) requesting for partners \n\
3) leave a 4-minute up voice message and answer questions \n\
related to weekly topic. ONLY on Monday can talk about \n\
your last weekend or sharing something interesting."

/* TODO: Fix the parameter */
void print_result (const char *pair_list,
		   const char *single_list);
		   
#endif  // PAIRUP_FORMATTER_H
