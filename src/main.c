#include <errno.h>
#include <string.h>

#include "logger.h"
#include "networking.h"

int main() {
  struct Server http_server = {0};

  if (SetServerStartupParams(&http_server) == OTHER_ERROR) {
    printf("Cannot set server startup params\n");
    FreeServerParams(&http_server);
    return OTHER_ERROR;
  }

  LogInit(LOGGER_FILENAME);

  if (SetupServer(&http_server) == NETWORK_ERROR) {
    return errno;
  }
  if (RunServer(&http_server) == NETWORK_ERROR) {
    return errno;
  }
  FreeServerParams(&http_server);
  return 0;
}
