#define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "http.h"
#include "logger.h"
#include "sys.h"

#define REQBUF (1 << 10)
#define CGIBUFF (1 << 12)

int open_serverfd(const char *port) {
  struct addrinfo *res = NULL;
  struct addrinfo hints = {};
  int status = 0;

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(NULL, "8080", &hints, &res)) != 0) {
    error("getaddrinfo: %s", gai_strerror(status));
    exit(-1);
  }

  int server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  if (server_fd < 0) {
    error("socket: %s", strerror(errno));
    exit(-1);
  }
  int reuse = 1;
  status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse,
                   sizeof(int));
  if (status != 0) {
    log_perror("setsockopt");
  }

  if ((status = bind(server_fd, res->ai_addr, res->ai_addrlen)) != 0) {
    error("bind: %s", strerror(errno));
    exit(-1);
  }

  freeaddrinfo(res);

  info("bind on %s", port);

  return server_fd;
}

int main() {
  signal(SIGPIPE, SIG_IGN);

  int server_fd = open_serverfd("8080");

  if (listen(server_fd, SOMAXCONN) < 0) {
    error("listen: %s", strerror(errno));
    exit(-1);
  }

  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);

  struct epoll_event event = {};
  event.events = EPOLLIN;
  event.data.fd = server_fd;
  Epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

  struct sockaddr_storage client_addr;
  socklen_t client_len = sizeof(client_addr);

  struct epoll_event events[SOMAXCONN];
  memset(events, 0, sizeof(events));

  for (;;) {
    int n = epoll_wait(epoll_fd, events, SOMAXCONN, -1);
    if (n == -1) {
      log_perror("epoll");
    }
    for (int i = 0; i < n; i++) {
      if (events[i].events & EPOLLERR) {
        error("bad epoll event");
        exit(-1);
      } else if (events[i].data.fd == server_fd) {
        if (events[i].events & EPOLLIN) {
          // server fd
          int client_fd =
              accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
          if (client_fd == -1) {
            log_perror("accept");
          } else {
            info("client_fd: %d", client_fd);

            event.events = EPOLLIN;
            event.data.fd = client_fd;
            Epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
          }
        }
      } else if (events[i].events & EPOLLIN) {
        // client fd
        int client_fd = events[i].data.fd;

        info("EPOLLIN event on %d", client_fd);

        Epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);

        pthread_t p;
        if (pthread_create(&p, NULL, handle_conn, (void *)(long long)client_fd) < 0) {
          error("pthread");
          exit(-1);
        }
        if (pthread_detach(p) < 0) {
          error("pthread");
          exit(-1);
        }
      }
    }
  }

  close(server_fd);
}
