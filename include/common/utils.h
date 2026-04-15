#ifndef UTILS_H
#define UTILS_H

#include "types.h"

char *str_dup(const char *s);
char *str_trim(char *s);
int   str_starts_with(const char *s, const char *prefix);

Finding *finding_new(void);
void     finding_free(Finding *f);

#endif
