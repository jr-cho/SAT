#include "../../include/common/utils.h"
#include <ctype.h>

char *str_dup(const char *s)
{
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy) memcpy(copy, s, len);
    return copy;
}

char *str_trim(char *s)
{
    if (!s) return NULL;

    while (isspace((unsigned char)*s))
        s++;

    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end))
        end--;
    *(end + 1) = '\0';

    return s;
}

int str_starts_with(const char *s, const char *prefix)
{
    if (!s || !prefix) return 0;
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

Finding *finding_new(void)
{
    Finding *f = calloc(1, sizeof(Finding));
    if (f) {
        f->line   = -1;
        f->column = -1;
    }
    return f;
}

void finding_free(Finding *f)
{
    if (!f) return;
    free(f->file);
    free(f->category);
    free(f->message);
    free(f);
}
