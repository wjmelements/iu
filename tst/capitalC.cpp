#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "capitalC.h"
#include <assert.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <string.h>


static volatile size_t count = 0;
void* func(void* _arg) {
    pthread_mutex_t* arg = (pthread_mutex_t*)_arg;
    Pthread_mutex_lock(arg);
    count++;
    Pthread_mutex_unlock(arg);
}
#define test_negative(FUNC, ERRNO)\
    {\
        pid_t pid = Fork();\
        if (!pid) {\
            Close(2);\
            assert(2 == Open("/dev/null", O_WRONLY));\
            FUNC;\
            exit(EXIT_SUCCESS);\
        }\
        int status;\
        assert(pid == Wait(&status));\
        assert(WIFEXITED(status));\
        if (WEXITSTATUS(status) != ERRNO) {\
            printf("Wrong errno for "#FUNC": %i (%s)\n", WEXITSTATUS(status), strerror(WEXITSTATUS(status)));\
        }\
        assert(WEXITSTATUS(status) == ERRNO);\
    }

void test_negatives() {
    pthread_t anything;
    int bleh;
    test_negative(Close(-1), EBADF);
    test_negative(Wait(&bleh), ECHILD);
    test_negative(Lseek(-1,0,0), EBADF);
}

void test_pthreads() {
    pthread_t thread;
    pthread_mutex_t mutex;
    Pthread_mutex_init(&mutex, NULL);
    const size_t expected = 2;
    for (size_t i = 0; i < expected; i++) {
        Pthread_create(&thread, NULL, func, &mutex);
        Pthread_detach(thread);
    }
    while (count != expected) {
        sched_yield();
    }
    Pthread_mutex_destroy(&mutex);
}

int main() {
    pid_t pid = Fork();
    if (!pid) {
        while (1);
    }
    Kill(pid, SIGKILL);
    int status;
    assert(pid == Wait(&status));
    assert(WIFSIGNALED(status));
    assert(WTERMSIG(status) == SIGKILL);
    char buf[16];
    char origin[16] = "abcdefg";
    Snprintf(buf, 16, origin);
    assert(strncmp(buf, origin, 16) == 0);
    int fd = Open("include/capitalC.h", O_NOATIME);
    int dfd = Open("include", O_DIRECTORY | O_NOATIME);
    DIR* dp = Fdopendir(dfd);
    Closedir(dp);
    Close(fd);
    const size_t buffer_size = 400000;
    char* buffer = (char*) Malloc(buffer_size);
    int rfd = Open("/dev/urandom", O_RDONLY);
    Read(rfd, buffer, buffer_size);
    Close(rfd);
    int dnfd = Open("/dev/null", O_WRONLY);
    Write(dnfd, buffer, buffer_size);
    Close(dnfd);
    Free(buffer);
    //test_pthreads();
    test_negatives();
    return 0;
}
