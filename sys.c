#include "sys.h"

#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "logger.h"

size_t Write (int __fd, const void *__buf, size_t __n) {
  int status = write (__fd, __buf, __n);
  if (status < 0) {
    log_perror("write");
  }
  return status;
}

int Epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
  int status = 0;
  if ((status = epoll_ctl(epfd, op, fd, event)) < 0) {
    log_perror("epoll_ctl");
  }
  return status;
}
