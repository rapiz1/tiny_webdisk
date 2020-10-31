#ifndef _TINY_WEB_DISK_SYS_H__
#define _TINY_WEB_DISK_SYS_H__
#define _GNU_SOURCE

#include <unistd.h>
#include <sys/epoll.h>

size_t Write (int __fd, const void *__buf, size_t __n);
int Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

#endif
