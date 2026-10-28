/* gauss solves Ax = b in -t threads
   the system comes from stdin or is random with -g for the timings */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common/common.h"
#include "gauss.h"
#include "os/os.h"
#include "system.h"

typedef struct {
    long threads;
    long generate;
} options;

static void print_usage(void)
{
    fprintf(stderr,
            "usage: gauss -t <threads> [-g <n>]\n"
            "  -t  number of threads working at once, 1..%d (required)\n"
            "  -g  solve a random system of size n instead of reading one\n"
            "the system is read from stdin: n, then n rows \"a1 ... an b\"\n",
            MAX_THREADS);
}

static int parse_options(int argc, char** argv, options* keys)
{
    keys->threads  = 0;
    keys->generate = 0;

    int key = 0;
    while ((key = getopt(argc, argv, "t:g:")) != -1) {
        switch (key) {
        case 't':
            if (parse_long(optarg, 1, MAX_THREADS, &keys->threads) < 0) {
                fprintf(stderr, "-t: an integer from 1 to %d is expected\n", MAX_THREADS);
                return -1;
            }
            break;
        case 'g':
            if (parse_long(optarg, 1, MAX_SIZE, &keys->generate) < 0) {
                fprintf(stderr, "-g: an integer from 1 to %d is expected\n", MAX_SIZE);
                return -1;
            }
            break;
        default:
            return -1;
        }
    }

    if (optind != argc) {
        fprintf(stderr, "unexpected argument: %s\n", argv[optind]);
        return -1;
    }
    if (keys->threads == 0) {
        fprintf(stderr, "the number of threads (-t) is required\n");
        return -1;
    }
    return 0;
}

int main(int argc, char** argv)
{
    options keys;
    if (parse_options(argc, argv, &keys) < 0) {
        print_usage();
        return EXIT_USAGE;
    }

    fprintf(stderr, "gauss: pid=%lld tid=%lld threads=%ld\n",
            os_process_id(), os_thread_id(), keys.threads);

    linear_system system   = { 0, NULL, NULL };
    linear_system original = { 0, NULL, NULL };
    double*       x        = NULL;

    int code = (keys.generate != 0)
             ? system_generate(&system, (int)keys.generate)
             : system_read(stdin, &system, os_is_terminal(STDIN_FILENO));
    /* the solver destroys the matrix, keep a copy to check the answer */
    if (code == EXIT_OK) {
        code = system_copy(&original, &system);
    }
    if (code == EXIT_OK) {
        x = malloc(sizeof(double) * (size_t)system.size);
        code = (x == NULL) ? EXIT_MEMORY : EXIT_OK;
    }
    if (code == EXIT_MEMORY) {
        fprintf(stderr, "not enough memory for the system\n");
    }

    if (code == EXIT_OK) {
        /* only the solving is timed, input does not depend on threads */
        const double start = os_time_ms();
        code = gauss_solve(&system, (int)keys.threads, x);
        const double time = os_time_ms() - start;

        if (code == EXIT_SINGULAR) {
            fprintf(stderr, "the matrix is singular: the system has no unique solution\n");
        }
        if (code == EXIT_OK) {
            fprintf(stderr, "solved: n=%d threads=%ld time=%.3f ms residual=%.3e\n",
                    system.size, keys.threads, time, system_residual(&original, x));
            /* nobody reads thousands of lines of a random answer */
            if (keys.generate == 0) {
                code = system_write_solution(stdout, x, system.size);
                if (code != EXIT_OK) {
                    print_error("stdout");
                }
            }
        }
    }

    free(x);
    system_destroy(&original);
    system_destroy(&system);
    return code;
}
