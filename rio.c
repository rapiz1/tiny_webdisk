#include "rio.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

void rio_init(struct rio_t *rio, int fd) {
  rio->fd = fd;
  rio->cnt = 0;
  rio->buf_ptr = rio->buf;
}

int rio_read(struct rio_t *rio, char *usrbuf, int n) {
  while (rio->cnt <= 0) {
    rio->cnt = read(rio->fd, rio->buf_ptr, sizeof(rio->buf));
    if (rio->cnt < 0) {
      if (errno != EINTR)
        return -1;
    }
    else if (rio->cnt == 0) {
      return 0;
    }
    else {
      rio->buf_ptr = rio->buf; 
    }
  }
  int cnt = n < rio->cnt ? n : rio->cnt;
  memcpy(usrbuf, rio->buf_ptr, cnt);
  rio->buf_ptr += cnt;
  rio->cnt -= cnt;
  return cnt;
}

int rio_readline(struct rio_t *rio, char* dest, int maxlen) {
  int n, rc;
  char c;
  for (n = 0; n < maxlen; n++) {
    if ((rc = rio_read(rio, &c, 1)) == 1) {
      c = c == '\n' ? 0 : c;
      *dest++ = c;
      if (!c) break;
    }
    else if (rc == -1) {
      return -1;
    }
    else break;
  } 
  return n;
}
