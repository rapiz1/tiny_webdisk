#include "logger.h"

#include <errno.h>

#include <stdlib.h>
#include <string.h>

void log_perror(const char* msg) {
  error("%s: %s", msg, strerror(errno));
  exit(-1);
}
