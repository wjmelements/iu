#ifndef CAPITAL_C
#define CAPITAL_C

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
/*
int Accept(int sockfd, struct sockaddr*, socklen_t*);
void* Calloc(size_t nmemb, size_t size);
void Close(int fd);
void Closedir(DIR* dp);
void Connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int Dup(int fd);
void Fclose(FILE* fp);
DIR* Fdopendir(int fd);
FILE* Fopen(const char* path, const char* mode);
pid_t Fork(void);
void Free(void*)
void Fstat(int fd, struct stat info);
void Gettimeofday(struct timeval* tv);
void Kill(pid_t pid, int sig);
void Linkat(int olddirfd, const char* oldpath, int newdirfd, const char* newpath, int flags);
void Listen(int sockfd, int backlog);
void Lseek(int fd, off_t offset, int whence);
void* Malloc(size_t size);
int Open(const char* path, int flags);
int Open(const char* path, int flags, mode_t mode);
void Pipe(int fd[2]);
int Poll(struct pollfd*, nfds_t nfds, int timeout);
void Pthread_create(pthread_t* thread, pthread_attr_t* attr, void*(*func)(void*), void* arg);
void Pthread_detach(pthread_t thread);
void Pthread_join(pthread_t thread, void** retval);
void Pthread_mutex_destroy(pthread_mutex_t* mutex);
void Pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
void Pthread_mutex_lock(pthread_mutex_t* mutex);
void Pthread_mutex_unlock(pthread_mutex_t* mutex);
void Read(int fd, void* buf, size_t count);
void Send(int sockfd, const void* buf, size_t len, int flags);
int Snprintf(char* str, size_t size, const char* format, ...);
int Socket(int domain, int type, int protocol);
void Unlink(const char* path);
void Wait(int* status);
void Write(int fd, const void* buf, size_t count);
*/
 
#define DIE()\
    do {\
        int e = errno;\
        perror(__func__);\
        exit(e);\
    } while (0)

#define DIEWITH(arg)\
    do {\
        int e = errno;\
        perror(arg);\
        exit(e);\
    } while (0)


static inline void Close(int fd) {
    int ret = close(fd);
    if (ret == -1) {
        DIE();
    }
}
static inline void Pipe(int fds[2]) {
    int ret = pipe(fds);
    if (ret == -1) {
        DIE();
    }
}
static inline int Dup(int fd) {
    int ret = dup(fd);
    if (ret == -1) {
        DIE();
    }
    return ret;
}
static inline void Lseek(int fd, off_t offset, int whence) {
    off_t ret = lseek(fd, offset, whence);
    if (ret == (off_t) -1) {
        DIE();
    }
}
static inline void Fclose(FILE* fp) {
    int ret = fclose(fp);
    if (ret == -1) {
        DIE();
    }
}
static inline DIR* Fdopendir(int fd) {
    DIR* ret = fdopendir(fd);
    if (ret == NULL) {
        DIE();
    }
    return ret;
}
static inline void Closedir(DIR* dp) {
    int ret = closedir(dp);
    if (ret == -1) {
        DIE();
    }
}
static inline pid_t Fork(void) {
    pid_t ret = fork();
    if (ret == -1) {
        DIE();
    }
    return ret;
}
static inline pid_t Wait(int* status) {
    pid_t ret = wait(status);
    if (ret == -1) {
        DIE();
    }
    return ret;
}
static inline int Snprintf(char* str, size_t size, const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    int ret = vsnprintf(str, size, format, arg);
    if (ret < 0) {
        DIE();
    }
    return ret;
}
static inline void Kill(pid_t pid, int sig) {
    int ret = kill(pid, sig);
    if (ret == -1) {
        int errnum = errno;
        char buf[16];
        switch (errnum) {
            case EINVAL:
                Snprintf(buf, sizeof(buf), "%i", sig);
                break;
            case EPERM:
            case ESRCH:
                Snprintf(buf, sizeof(buf), "%u", pid);
                break;
            default:
                DIE();
        }
        errno = errnum;
        DIEWITH(buf);
    }
}
static inline int Open(const char* path, int flags) {
    int ret = open(path, flags);
    if (ret == -1) {
        DIEWITH(path);
    }
    return ret;
}
static inline int Open(const char* path, int flags, mode_t mode) {
    int ret = open(path, flags, mode);
    if (ret == -1) {
        DIEWITH(path);
    }
    return ret;
}
static inline FILE* Fopen(const char* path, const char* mode) {
    FILE* ret = fopen(path, mode);
    if (ret == NULL) {
        DIEWITH(path);
    }
    return ret;
}
static inline void Fstat(int fd, struct stat* info) {
    int success = fstat(fd, info);
    if (success == -1) {
        DIE();
    }
}
static inline void Linkat(int olddirfd, const char* oldpath, int newdirfd, const char* newpath, int flags) {
    int status = linkat(olddirfd, oldpath, newdirfd, newpath, flags);
    if (status) {
        DIE();
    }
}
static inline void Unlink(const char* path) {
    int ret = unlink(path);
    if (ret == -1) {
        DIEWITH(path);
    }
}
#define Free free
static inline void* Malloc(size_t size) {
    void* ret = malloc(size);
    if (ret == NULL) {
        DIE();
    }
    return ret;
}
static inline void* Calloc(size_t nmemb, size_t size) {
    void* ret = calloc(nmemb, size);
    if (ret == NULL) {
        DIE();
    }
    return ret;
}
static inline void Read(int fd, void* buf, size_t count) {
    while (1) {
        ssize_t ret = read(fd, buf, count);
        if (ret == -1) {
            switch (errno) {
                case EINTR:
                    // try again
                    break;
                default:
                    DIE();
            }
        } else if (ret < count) {
            count -= ret;
            buf += ret;
        } else {
            return;
        }
    }
}
static inline void Write(int fd, const void* buf, size_t count) {
    while (1) {
        ssize_t ret = write(fd, buf, count);
        if (ret == -1) {
            switch (errno) {
                case EINTR:
                    // try again
                    break;
                default:
                    DIE();
            }
        } else if (ret < count) {
            count -= ret;
            buf += ret;
        } else {
            return;
        }
    }
}
static inline int Socket(int domain, int type, int protocol) {
    int ret = socket(domain, type, protocol);
    if (ret == -1) {
        DIE();
    }
    return ret;
}
static inline void Connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    int ret = connect(sockfd, addr, addrlen);
    if (ret == -1) {
        DIE();
    }
}
static inline void Listen(int sockfd, int backlog) {
    int ret = listen(sockfd, backlog);
    if (ret == -1) {
        DIE();
    }
}
static inline int Accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    int ret = accept(sockfd, addr, addrlen);
    if (ret == -1) {
        DIE();
    }
    return ret;
}
static inline int Poll(struct pollfd* pollfds, nfds_t nfds, int timeout) {
    int ret = poll(pollfds, nfds, timeout);
    if (ret == -1) {
        DIE();
    }
    return ret;
}
static inline void Send(int sockfd, const void* buf, size_t len, int flags) {
    while (1) {
        ssize_t ret = send(sockfd, buf, len, flags);
        if (ret == -1) {
            switch (errno) {
                case EINTR:
                    // try again
                    break;
                default:
                    DIE();
            }
        } else if (ret < len) {
            len -= ret;
            buf += ret;
        } else {
            return;
        }
    }
}
static inline void Gettimeofday(struct timeval* tv) {
    int status = gettimeofday(tv, NULL);
    if (status == -1) {
        DIE();
    }
}
/*
 * begin pthread stuff
 */
static inline void Pthread_create(pthread_t* thread, pthread_attr_t* attr, void*(*func)(void*), void* arg) {
    errno = pthread_create(thread, attr, func, arg);
    if (errno) {
        DIE();
    }
}
static inline void Pthread_detach(pthread_t thread) {
    errno = pthread_detach(thread);
    if (errno) {
        DIE();
    }
}
static inline void Pthread_join(pthread_t thread, void** retval) {
    errno = pthread_join(thread, retval);
    if (errno) {
        DIE();
    }
}
static inline void Pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
    errno = pthread_mutex_init(mutex, attr);
    if (errno) {
        DIE();
    }
}
static inline void Pthread_mutex_lock(pthread_mutex_t* mutex) {
    errno = pthread_mutex_lock(mutex);
    if (errno) {
        DIE();
    }
}
static inline void Pthread_mutex_unlock(pthread_mutex_t* mutex) {
    errno = pthread_mutex_unlock(mutex);
    if (errno) {
        DIE();
    }
}
static inline void Pthread_mutex_destroy(pthread_mutex_t* mutex) {
    errno = pthread_mutex_destroy(mutex);
    if (errno) {
        DIE();
    }
}
/* 
 *  end pthread stuff
 */

#undef DIE
#undef DIEWITH

#endif // CAPITAL_C
