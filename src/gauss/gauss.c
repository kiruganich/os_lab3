#include "gauss.h"

#include "common/common.h"
#include "os/os.h"

#include <float.h>
#include <math.h>
#include <stdio.h>

/* shared by all threads, no locks needed
   thread 0 writes before a barrier and the others read after it */
typedef struct {
    linear_system* system;
    int            threads;
    os_barrier*    barrier;
    double         tolerance;
    int            singular;
} gauss_task;

/* thread 0 puts the row with the largest |a_ik| in place k
   returns -1 if the matrix is singular */
static int choose_pivot(gauss_task* task, int k)
{
    double** rows = task->system->rows;
    int      best = k;

    for (int i = k + 1; i < task->system->size; ++i) {
        if (fabs(rows[i][k]) > fabs(rows[best][k])) {
            best = i;
        }
    }
    if (fabs(rows[best][k]) <= task->tolerance) {
        return -1;
    }

    double* swap = rows[k];
    rows[k]      = rows[best];
    rows[best]   = swap;
    return 0;
}

/* row i belongs to thread i % threads so the work stays even until the end */
static void eliminate_rows(gauss_task* task, int k, int index)
{
    const int     n     = task->system->size;
    double**      rows  = task->system->rows;
    const double* pivot = rows[k];

    for (int i = k + 1; i < n; ++i) {
        if (i % task->threads != index) {
            continue;
        }
        double*      row    = rows[i];
        const double factor = row[k] / pivot[k];
        for (int j = k + 1; j <= n; ++j) {
            row[j] -= factor * pivot[j];
        }
        row[k] = 0.0;
    }
}

static void gauss_thread(int index, void* context)
{
    gauss_task* task = context;

    fprintf(stderr, "gauss: thread %d/%d tid=%lld\n", index, task->threads, os_thread_id());

    for (int k = 0; k < task->system->size; ++k) {
        if (index == 0 && choose_pivot(task, k) < 0) {
            task->singular = 1;
        }
        os_barrier_wait(task->barrier);       /* pivot row is ready */
        if (task->singular) {
            return;
        }
        eliminate_rows(task, k, index);
        os_barrier_wait(task->barrier);       /* column k is done */
    }
}

/* each x[i] needs the next ones so this part stays in one thread */
static void back_substitution(const linear_system* system, double* x)
{
    const int n = system->size;
    for (int i = n - 1; i >= 0; --i) {
        const double* row = system->rows[i];
        double        sum = row[n];
        for (int j = i + 1; j < n; ++j) {
            sum -= row[j] * x[j];
        }
        x[i] = sum / row[i];
    }
}

int gauss_solve(linear_system* system, int threads, double* x)
{
    /* zero check is relative to the size of the values */
    double scale = 0.0;
    for (int i = 0; i < system->size; ++i) {
        for (int j = 0; j < system->size; ++j) {
            scale = fmax(scale, fabs(system->rows[i][j]));
        }
    }
    if (scale == 0.0) {
        return EXIT_SINGULAR;
    }

    gauss_task task;
    task.system    = system;
    task.threads   = threads;
    task.tolerance = scale * system->size * DBL_EPSILON;
    task.singular  = 0;
    task.barrier   = os_barrier_create(threads);
    if (task.barrier == NULL) {
        print_error("pthread_barrier_init");
        return EXIT_THREAD;
    }

    const int started = os_team_run(threads, gauss_thread, &task);
    if (started < 0) {
        print_error("pthread_create");
    }
    os_barrier_destroy(task.barrier);

    if (started < 0) {
        return EXIT_THREAD;
    }
    if (task.singular) {
        return EXIT_SINGULAR;
    }
    back_substitution(system, x);
    return EXIT_OK;
}
