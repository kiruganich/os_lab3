#ifndef LAB3_COMMON_H
#define LAB3_COMMON_H

enum {
    EXIT_OK       = 0,
    EXIT_USAGE    = 2,
    EXIT_READ     = 3,
    EXIT_WRITE    = 4,
    EXIT_SINGULAR = 5,
    EXIT_THREAD   = 6,
    EXIT_MEMORY   = 7
};

#define MAX_THREADS 256
#define MAX_SIZE    10000

/* prints "context: error text" like perror */
void print_error(const char* context);

/* 0 if text is a whole number in [minimum, maximum], else -1 */
int parse_long(const char* text, long minimum, long maximum, long* value);

#endif
