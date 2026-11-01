#include "os.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

struct os_barrier {
    pthread_barrier_t barrier;
};

os_barrier* os_barrier_create(int count)
{
    os_barrier* barrier = malloc(sizeof(*barrier));
    if (barrier == NULL) {
        return NULL;
    }
    /* pthread returns the error code instead of setting errno */
    const int error = pthread_barrier_init(&barrier->barrier, NULL, (unsigned)count);
    if (error != 0) {
        free(barrier);
        errno = error;
        return NULL;
    }
    return barrier;
}

void os_barrier_wait(os_barrier* barrier)
{
    /* a broken barrier means the other threads would wait forever */
    const int result = pthread_barrier_wait(&barrier->barrier);
    if (result != 0 && result != PTHREAD_BARRIER_SERIAL_THREAD) {
        fprintf(stderr, "pthread_barrier_wait: %s\n", strerror(result));
        abort();
    }
}

void os_barrier_destroy(os_barrier* barrier)
{
    if (barrier != NULL) {
        (void)pthread_barrier_destroy(&barrier->barrier);
        free(barrier);
    }
}

/* new threads wait here until all of them are created
   otherwise a failed pthread_create would leave them stuck in the barrier */
typedef enum {
    GATE_CLOSED,
    GATE_OPEN,
    GATE_CANCELLED
} gate_state;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  changed;
    gate_state      state;
    os_thread_body  body;
    void*           context;
} team_gate;

typedef struct {
    team_gate* gate;
    int        index;
} team_member;

static gate_state gate_wait(team_gate* gate)
{
    pthread_mutex_lock(&gate->mutex);
    /* loop because of spurious wakeups */
    while (gate->state == GATE_CLOSED) {
        pthread_cond_wait(&gate->changed, &gate->mutex);
    }
    const gate_state state = gate->state;
    pthread_mutex_unlock(&gate->mutex);
    return state;
}

static void gate_set(team_gate* gate, gate_state state)
{
    pthread_mutex_lock(&gate->mutex);
    gate->state = state;
    pthread_cond_broadcast(&gate->changed);
    pthread_mutex_unlock(&gate->mutex);
}

static void* team_thread(void* argument)
{
    team_member* member = argument;
    if (gate_wait(member->gate) == GATE_OPEN) {
        member->gate->body(member->index, member->gate->context);
    }
    return NULL;
}

int os_team_run(int count, os_thread_body body, void* context)
{
    pthread_t*   threads = malloc(sizeof(*threads) * (size_t)count);
    team_member* members = malloc(sizeof(*members) * (size_t)count);
    if (threads == NULL || members == NULL) {
        free(threads);
        free(members);
        errno = ENOMEM;
        return -1;
    }

    team_gate gate;
    gate.state   = GATE_CLOSED;
    gate.body    = body;
    gate.context = context;
    int error = pthread_mutex_init(&gate.mutex, NULL);
    if (error == 0) {
        error = pthread_cond_init(&gate.changed, NULL);
        if (error != 0) {
            pthread_mutex_destroy(&gate.mutex);
        }
    }
    if (error != 0) {
        free(threads);
        free(members);
        errno = error;
        return -1;
    }

    /* index 0 is this thread so only count-1 new ones */
    int created = 0;
    for (int index = 1; index < count; ++index) {
        members[index].gate  = &gate;
        members[index].index = index;
        error = pthread_create(&threads[index], NULL, team_thread, &members[index]);
        if (error != 0) {
            break;
        }
        ++created;
    }

    gate_set(&gate, (error == 0) ? GATE_OPEN : GATE_CANCELLED);
    if (error == 0) {
        body(0, context);
    }

    for (int index = 1; index <= created; ++index) {
        const int joined = pthread_join(threads[index], NULL);
        if (joined != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(joined));
        }
    }

    pthread_cond_destroy(&gate.changed);
    pthread_mutex_destroy(&gate.mutex);
    free(threads);
    free(members);

    if (error != 0) {
        errno = error;
        return -1;
    }
    return 0;
}

double os_time_ms(void)
{
    /* monotonic clock does not jump when the system time changes */
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)now.tv_sec * 1000.0 + (double)now.tv_nsec / 1e6;
}

long long os_process_id(void)
{
    return (long long)getpid();
}

/* the pid is the same for all threads, the tid is per thread */
long long os_thread_id(void)
{
    return (long long)syscall(SYS_gettid);
}

int os_is_terminal(int fd)
{
    return isatty(fd) ? 1 : 0;
}
