#ifndef _TINY_WEB_DISK_RIO_H__
#define _TINY_WEB_DISK_RIO_H__
#define _GNU_SOURCE

#define RIO_BUF_LEN (1<<12)

struct rio_t {
  int fd;
  int cnt;
  char *buf_ptr;
  char buf[RIO_BUF_LEN];
};

void rio_init(struct rio_t *rio, int fd);
int rio_readline(struct rio_t *rio, char* dest, int maxlen);

#endif
