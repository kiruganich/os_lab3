#include "system.h"

#include "common/common.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

int system_create(linear_system* system, int size)
{
    const size_t width = (size_t)size + 1;

    system->size = size;
    system->data = malloc(sizeof(double) * (size_t)size * width);
    system->rows = malloc(sizeof(double*) * (size_t)size);
    if (system->data == NULL || system->rows == NULL) {
        system_destroy(system);
        return EXIT_MEMORY;
    }
    for (int i = 0; i < size; ++i) {
        system->rows[i] = system->data + (size_t)i * width;
    }
    return EXIT_OK;
}

int system_copy(linear_system* copy, const linear_system* source)
{
    const int code = system_create(copy, source->size);
    if (code != EXIT_OK) {
        return code;
    }
    memcpy(copy->data, source->data,
           sizeof(double) * (size_t)source->size * ((size_t)source->size + 1));
    return EXIT_OK;
}

void system_destroy(linear_system* system)
{
    free(system->data);
    free(system->rows);
    system->data = NULL;
    system->rows = NULL;
    system->size = 0;
}

int system_read(FILE* input, linear_system* system, int prompt)
{
    /* prompts go to stderr so stdout has only the answer */
    if (prompt) {
        fprintf(stderr, "size of the system n: ");
    }
    long size = 0;
    if (fscanf(input, "%ld", &size) != 1 || size < 1 || size > MAX_SIZE) {
        fprintf(stderr, "the first number must be the size n from 1 to %d\n", MAX_SIZE);
        return EXIT_READ;
    }

    const int code = system_create(system, (int)size);
    if (code != EXIT_OK) {
        return code;
    }

    for (int i = 0; i < system->size; ++i) {
        if (prompt) {
            fprintf(stderr, "row %d of %d (a1 ... a%d b): ", i + 1, system->size, system->size);
        }
        for (int j = 0; j <= system->size; ++j) {
            /* fscanf accepts inf and nan, we do not */
            if (fscanf(input, "%lf", &system->rows[i][j]) != 1 ||
                !isfinite(system->rows[i][j])) {
                fprintf(stderr, "row %d, column %d: a finite number is expected\n",
                        i + 1, j + 1);
                system_destroy(system);
                return EXIT_READ;
            }
        }
    }

    /* a person does not press ctrl+d so do not wait for the end of input */
    if (prompt) {
        return EXIT_OK;
    }
    char extra = 0;
    if (fscanf(input, " %c", &extra) == 1) {
        fprintf(stderr, "extra data after %ld rows of the system\n", size);
        system_destroy(system);
        return EXIT_READ;
    }
    return EXIT_OK;
}

int system_generate(linear_system* system, int size)
{
    const int code = system_create(system, size);
    if (code != EXIT_OK) {
        return code;
    }

    /* same seed every run so the timings are comparable */
    srand(1);
    for (int i = 0; i < size; ++i) {
        double* row = system->rows[i];
        double  b   = 0.0;
        for (int j = 0; j < size; ++j) {
            row[j] = 2.0 * rand() / RAND_MAX - 1.0;
            b += row[j];
        }
        row[size] = b;
    }
    return EXIT_OK;
}

int system_write_solution(FILE* output, const double* x, int size)
{
    for (int i = 0; i < size; ++i) {
        if (fprintf(output, "%.10g\n", x[i]) < 0) {
            return EXIT_WRITE;
        }
    }
    return (fflush(output) == 0) ? EXIT_OK : EXIT_WRITE;
}

double system_residual(const linear_system* system, const double* x)
{
    double worst = 0.0;
    for (int i = 0; i < system->size; ++i) {
        const double* row = system->rows[i];
        double        sum = 0.0;
        for (int j = 0; j < system->size; ++j) {
            sum += row[j] * x[j];
        }
        const double error = fabs(sum - row[system->size]);
        if (error > worst) {
            worst = error;
        }
    }
    return worst;
}
