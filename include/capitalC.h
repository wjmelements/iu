#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
int Accept(int sockfd, struct sockaddr*, socklen_t*);
void* Calloc(size_t nmemb, size_t size);
void Close(int fd);
void Closedir(DIR* dp);
void Connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
void Fclose(FILE* fp);
DIR* Fdopendir(int fd);
FILE* Fopen(const char* path, const char* mode);
pid_t Fork(void);
#define Free free
void Kill(pid_t pid, int sig);
void Listen(int sockfd, int backlog);
void* Malloc(size_t size);
int Open(const char* path, int flags);
void Pthread_create(pthread_t* thread, pthread_attr_t* attr, void*(*func)(void*), void* arg);
void Pthread_detach(pthread_t thread);
void Pthread_join(pthread_t thread, void** retval);
void Pthread_mutex_destroy(pthread_mutex_t* mutex);
void Pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
void Pthread_mutex_lock(pthread_mutex_t* mutex);
void Pthread_mutex_unlock(pthread_mutex_t* mutex);
void Read(int fd, void* buf, size_t count);
void Send(int sockfd, const void* buf, size_t len, int flags);
void Snprintf(char* str, size_t size, const char* format, ...);
int Socket(int domain, int type, int protocol);
void Write(int fd, const void* buf, size_t count);
