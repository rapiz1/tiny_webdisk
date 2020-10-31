#include "http.h"

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "logger.h"
#include "rio.h"
#include "sys.h"

#define LINELEN (1 << 8)

static void strip_crlf(char* s, int *l) {
  if (*l != 0 && (s[*l-1] == '\n' || s[*l-1] == '\r')) {
    s[--(*l)] = 0;
  }
}

/*
  Get status code in string form
*/
static const char* strstatus(int status_code) {
  if (status_code == OK)
    return "OK";
  else if (status_code == NOT_FOUND)
    return "Not found";
  else if (status_code == BAD_REQUEST)
    return "Bad request";
  else
    return "Forbidden";
}

/*
  Get MIME from pathname
  Default to binary stream
*/
static const char* get_filetype(const char* pathname) {
  char* suffix = strrchr(pathname, '.');
  if (suffix == NULL)
    return "application/octet-stream";
  else if (strcmp(suffix, ".html") == 0)
    return "text/html";
  else if (strcmp(suffix, ".txt") == 0)
    return "text/plain";
  return "application/octet-stream";
}

/*
  Write resposne to fd
  Note that no `body` will be sent
*/
static void response(int fd, int status_code, int content_length,
                     const char* filetype) {
  char buf[1 << 10];
  int buflen = 0;

  buflen += sprintf(buf + buflen, "HTTP/1.0 %d %s\n", status_code,
                    strstatus(status_code));
  buflen += sprintf(buf + buflen, "Server: tiny_webdisk\n");
  buflen += sprintf(buf + buflen, "Content-Length: %d\n", content_length);

  if (status_code == OK && filetype) {
    buflen += sprintf(buf + buflen, "Content-Type: %s\n", filetype);
  }

  buf[buflen++] = '\n';

  Write(fd, buf, buflen);
}

/*
  Remove heading /
  Fallback empty pathname to disk/index.html
*/
static void standardize_pathname(char pathname[LINELEN]) {
  int len = strlen(pathname);
  if (pathname[0] == '/') {
    for (int i = 0; i < len; i++) {
      pathname[i] = pathname[i + 1];
    }
  }
  if (!pathname[0]) {
    strcpy(pathname, "disk/index.html");
  }
}

/*
  Handle a HTTP request
*/
void* handle_conn(void* vfd) {
  int fd = (int)((long long)vfd);

  struct rio_t rio;
  rio_init(&rio, fd);

  char req[LINELEN];
  int reqlen = rio_readline(&rio, req, LINELEN);
  info("%s", req);

  char pathname[LINELEN], version[LINELEN], method[LINELEN];
  if (sscanf(req, "%s %s %s", method, pathname, version) != 3) {
    response(fd, BAD_REQUEST, 0, NULL);
    warn("Bad request");
    goto END;
  }

  standardize_pathname(pathname);

  if (strcasecmp("GET", method) == 0) {
    int file_fd = open(pathname, O_RDONLY);
    if (file_fd == -1) {
      // if (errno == EACCES) {
      response(fd, NOT_FOUND, 0, NULL);
      warn("Failed to open the file %s", pathname);
      //} else {
    } else {
      struct stat file_stat;
      if (stat(pathname, &file_stat) < 0) {
        log_perror("stat");
      }

      int filesize = file_stat.st_size;

      response(fd, OK, filesize, get_filetype(pathname));

      sendfile(fd, file_fd, NULL, filesize);
    }
  } else if (strcasecmp("POST", method) == 0) {
    int file_fd = open(pathname, O_RDWR | O_CREAT,
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    
    if (file_fd == -1) {
      // if (errno == EACCES) {
      response(fd, FORBIDDEN, 0, NULL);
      warn("Failed to open the file %s", pathname);
      //} else {
    } else {
      char opt[LINELEN];
      char optVal[LINELEN];
      int filesize = 0;

      for (;;) {
        // drain the header
        reqlen = rio_readline(&rio, req, LINELEN);
        strip_crlf(req, &reqlen);
        info("%s (%d)", req, reqlen);
        sscanf(req, "%s %s", opt, optVal);

        if (strcasecmp(opt, "Content-Length:") == 0) {
          sscanf(optVal, "%d", &filesize);
        }

        if (!reqlen) break;
      }

      info("filesize: %d", filesize);

      if (filesize != 0) {
        if (fallocate(file_fd, 0, 0, filesize) < 0) {
          log_perror("fallocate");
        }
        else {
          void* file_charrep =
              mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd, 0);

          if (file_charrep == MAP_FAILED) {
            log_perror("mmap");
          }

          rio_readn(&rio, file_charrep, filesize);
        }
      }

      close(file_fd);

      response(fd, OK, 0, NULL);
    }
  } else {
    response(fd, BAD_REQUEST, 0, NULL);
    warn("Unsupported method %s", method);
    goto END;
  }

END:
  close(fd);
  return NULL;
}
