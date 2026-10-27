/* everything that depends on the os lives here, posix version in os_unix.c */
#ifndef LAB3_OS_H
#define LAB3_OS_H

typedef struct os_barrier os_barrier;

os_barrier* os_barrier_create(int count);
void        os_barrier_wait(os_barrier* barrier);
void        os_barrier_destroy(os_barrier* barrier);

typedef void (*os_thread_body)(int index, void* context);

/* runs body in count threads at once, index 0 is the calling thread
   returns -1 if a thread could not be created */
int os_team_run(int count, os_thread_body body, void* context);

double    os_time_ms(void);
long long os_process_id(void);
long long os_thread_id(void);
int       os_is_terminal(int fd);

#endif
