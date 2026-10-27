#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_error(const char* context)
{
    fprintf(stderr, "%s: %s\n", context, strerror(errno));
}

int parse_long(const char* text, long minimum, long maximum, long* value)
{
    errno = 0;
    char*      end    = NULL;
    const long parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0' || errno == ERANGE ||
        parsed < minimum || parsed > maximum) {
        return -1;
    }
    *value = parsed;
    return 0;
}
