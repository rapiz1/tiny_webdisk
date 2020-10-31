#include "http.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#include "logger.h"

/*
  Handle a HTTP request
*/
void handle_conn(int fd) {
  info("Conenction incoming");
  const char msg[] = "Hello";
  Write(fd, msg, sizeof(msg));
  close(fd);
  info("Conenction closed");
}
