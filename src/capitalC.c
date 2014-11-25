#include "capitalC.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>

 
#define DIE()\
    perror(__func__);\
    exit(errno)

#define DIEWITH(arg)\
    perror(arg);\
    exit(errno)


void Close(int fd) {
    int ret = close(fd);
    if (ret == -1) {
        DIE();
    }
}
void FClose(FILE* fp) {
    int ret = fclose(fp);
    if (ret == -1) {
        DIE();
    }
}
DIR* Fdopendir(int fd) {
    DIR* ret = fdopendir(fd);
    if (ret == NULL) {
        DIE();
    }
    return ret;
}
void Closedir(DIR* dp) {
    int ret = closedir(dp);
    if (ret == -1) {
        DIE();
    }
}
pid_t Fork(void) {
    pid_t ret = fork();
    if (ret == -1) {
        DIE();
    }
    return ret;
}
void Kill(pid_t pid, int sig) {
    int ret = kill(pid, sig);
    if (ret == -1) {
        int e = errno;
        char buf[16];
        switch (e) {
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
        errno = e;
        DIEWITH(buf);
    }
}
void Snprintf(char* str, size_t size, const char* format, ...) {
    va_list arg;
    va_start(arg, format);
    int ret = vsnprintf(str, size, format, arg);
    if (ret < 0) {
        DIE();
    }
}
int Open(const char* path, int flags) {
    int ret = open(path, flags);
    if (ret == -1) {
        DIEWITH(path);
    }
    return ret;
}
FILE* Fopen(const char* path, const char* mode) {
    FILE* ret = fopen(path, mode);
    if (ret == NULL) {
        DIEWITH(path);
    }
    return ret;
}
void* Malloc(size_t size) {
    void* ret = malloc(size);
    if (ret == NULL) {
        DIE();
    }
    return ret;
}
void* Calloc(size_t nmemb, size_t size) {
    void* ret = calloc(nmemb, size);
    if (ret == NULL) {
        DIE();
    }
    return ret;
}
void Read(int fd, void* buf, size_t count) {
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
void Write(int fd, const void* buf, size_t count) {
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
int Socket(int domain, int type, int protocol) {
    int ret = socket(domain, type, protocol);
    if (ret == -1) {
        DIE();
    }
    return ret;
}
void Connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    int ret = connect(sockfd, addr, addrlen);
    if (ret == -1) {
        DIE();
    }
}
void Listen(int sockfd, int backlog) {
    int ret = listen(sockfd, backlog);
    if (ret == -1) {
        DIE();
    }
}
int Accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    int ret = accept(sockfd, addr, addrlen);
    if (ret == -1) {
        DIE();
    }
    return ret;
}
void Send(int sockfd, const void* buf, size_t len, int flags) {
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
/*
 * begin pthread stuff
 */
void Pthread_create(pthread_t* thread, pthread_attr_t* attr, void*(*func)(void*), void* arg) {
    errno = pthread_create(thread, attr, func, arg);
    if (errno) {
        DIE();
    }
}
void Pthread_detach(pthread_t thread) {
    errno = pthread_detach(thread);
    if (errno) {
        DIE();
    }
}
void Pthread_join(pthread_t thread, void** retval) {
    errno = pthread_join(thread, retval);
    if (errno) {
        DIE();
    }
}
void Pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
    errno = pthread_mutex_init(mutex, attr);
    if (errno) {
        DIE();
    }
}
void Pthread_mutex_lock(pthread_mutex_t* mutex) {
    errno = pthread_mutex_lock(mutex);
    if (errno) {
        DIE();
    }
}
void Pthread_mutex_unlock(pthread_mutex_t* mutex) {
    errno = pthread_mutex_unlock(mutex);
    if (errno) {
        DIE();
    }
}
void Pthread_mutex_destroy(pthread_mutex_t* mutex) {
    errno = pthread_mutex_destroy(mutex);
    if (errno) {
        DIE();
    }
}
/* 
 *  end pthread stuff
 */
