#include "main.h"

#include <errno.h>
#include <netdb.h>
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

#define REQBUF (1<<10)
#define CGIBUFF (1<<12)

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

  if ((status = bind(server_fd, res->ai_addr, res->ai_addrlen)) != 0) {
    error("bind: %s", strerror(errno));
    exit(-1);
  }

  freeaddrinfo(res);

  info("bind on %s", port);

  return server_fd;
}

struct conn_stat {
  char req_buf[REQBUF];
  int req_buf_len;
  char cgi_buf[CGIBUFF];
  int cgi_buf_len;
  int client_fd;
  int cgi_fd;
};

struct conn_stat *conn_stat_create(int client_fd, int cgi_fd) {
  struct conn_stat* stat = malloc(sizeof(struct conn_stat));
  stat->req_buf_len = 0;
  stat->cgi_buf_len = 0;
  stat->client_fd = client_fd;
  stat->cgi_fd = cgi_fd;
  return stat;
}

int main() {
  signal(SIGPIPE, SIG_IGN);

  int server_fd = open_serverfd("8080");
  set_nonblocking(server_fd);

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

  struct conn_stat *conn_stats_by_client_fd[SOMAXCONN];
  struct conn_stat *conn_stats_by_cgi_fd[SOMAXCONN];
  memset(conn_stats_by_client_fd, 0, sizeof(conn_stats_by_client_fd));
  memset(conn_stats_by_cgi_fd, 0, sizeof(conn_stats_by_cgi_fd));

  for (;;) {
    int n = epoll_wait(epoll_fd, events, SOMAXCONN, -1);
    if (n == -1) {
      log_perror("epoll");
    }
    for (int i = 0; i < n; i++) {
      if (events[i].events & EPOLLERR) {
        error("bad epoll event");
        exit(-1);
      } else if (events[i].data.fd == server_fd || (events[i].events | EPOLLIN)) {
        // server fd
        for (;;) {
          int client_fd =
              accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
          if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
              break;
            } else {
              log_perror("accept");
            }
          } else {
            info("client_fd: %d", client_fd);

            set_nonblocking(client_fd);
            event.events = EPOLLIN;
            event.data.fd = client_fd;
            Epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event);
            conn_stats_by_client_fd[client_fd] = conn_stat_create(client_fd, -1);
          }
        }
        info("accept break");
      } else if (events[i].events & EPOLLIN) {
        // client fd
        info("EPOLLIN event");
        int client_fd = events[i].data.fd;
        struct conn_stat* stat = conn_stats_by_client_fd[client_fd];
        int pos = 0;

        for (;;) {
          int cnt = read(client_fd, stat->req_buf + stat->req_buf_len, REQBUF - stat->req_buf_len);
          if (cnt == -1) {
            if (errno == EAGAIN) break;
            else {
              log_perror("client_fd: read");
            }
          }
          else {
            stat->req_buf_len += cnt;
            stat->req_buf[pos] = 0;
          }
        }

        info("raw:");
        printf("\n%s\n", stat->req_buf);

        close(client_fd);

        free(stat);
        conn_stats_by_client_fd[client_fd] = NULL;
      }
    }
  }

  close(server_fd);
}
