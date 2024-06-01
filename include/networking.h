#ifndef NETWORKING_H
#define NETWORKING_H

#include "tls_adapter.h"
#include "utility.h"

struct Server {
  char s_port[MAX_PORTLEN];
  int sockfd;
  struct sockaddr_storage s_storage;
  char s_host[INET6_ADDRSTRLEN];
  char *default_document;
  char *root_folder;
  int backlog;
  struct ServerTls server_tls;
};

// initialize required data for Server structure, return NETWORK ERROR on error
int SetupServer(struct Server *host_server);

// Starts the server, returns NETWORK_ERROR on error
int RunServer(struct Server *host_server);

// Loads and reads data from default.conf, returns OTHER_ERROR on error
int SetServerStartupParams(struct Server *host_server);

void FreeServerParams(struct Server *host_server);
#endif  // NETWORKING_H