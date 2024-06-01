#include "networking.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"
#include "logger.h"

static int server_should_run = 1;

void TerminateHandler() { server_should_run = 0; }

int RunServer(struct Server *host_server) {
  printf("waiting for connectins on: %s:%s...\n", host_server->s_host,
         host_server->s_port);

  struct sigaction terminate_action = {0};
  terminate_action.sa_handler = TerminateHandler;
  sigaction(SIGINT, &terminate_action, NULL);

  struct sockaddr_storage their_addr = {0};
  socklen_t sin_size = sizeof(their_addr);
  size_t internal_sin_size = 0;
  pthread_t server_thread;

  while (server_should_run) {
    int retcode =
        mbedtls_net_accept(&host_server->server_tls.listen_fd,
                           &host_server->server_tls.session_tls.client_fd,
                           &their_addr, sin_size, &internal_sin_size);
    if (retcode != 0) {
      perror("server accept error");
      continue;
    }
    char client_ip[INET6_ADDRSTRLEN];
    their_addr.ss_family = internal_sin_size == 4 ? AF_INET : AF_INET6;
    GetSockaddrIp(&their_addr, client_ip, sizeof(client_ip));
    printf("Server: got connection from: %s\n", client_ip);

    struct ThreadData *thread_data = malloc(sizeof(struct ThreadData));
    memset(thread_data, 0, sizeof(struct ThreadData));
    thread_data->default_document =
        malloc(strlen(host_server->default_document) + 1);
    strcpy(thread_data->default_document, host_server->default_document);
    thread_data->root_folder = malloc(strlen(host_server->root_folder) + 1);
    strcpy(thread_data->root_folder, host_server->root_folder);
    thread_data->session_tls.client_fd =
        host_server->server_tls.session_tls.client_fd;
    thread_data->session_tls.conf = &host_server->server_tls.conf;
    pthread_create(&server_thread, NULL, HandleConnection, (void *)thread_data);
    pthread_detach(server_thread);
  }
  return 0;
}

int SetupServer(struct Server *host_server) {
  ServerTlsBaseInit(&host_server->server_tls);
  if (LaunchTlsSocket(host_server->s_host, host_server->s_port,
                      &host_server->server_tls) == NETWORK_ERROR) {
    return NETWORK_ERROR;
  }
  return 0;
}

int SetServerStartupParams(struct Server *host_server) {
  FILE *conf_file = NULL;
  long ret = OpenStaticFile("/default.conf", "..", &conf_file);
  if (ret < 0) {
    if (conf_file != NULL) {
      fclose(conf_file);
    }
    return OTHER_ERROR;
  }
  size_t size = ret;
  host_server->backlog = 10;
  char *conf = malloc(size + 1);
  char *seg_start = conf;
  int integrity_test = 0;
  while (getline(&conf, &size, conf_file) != -1) {
    char *token = strtok_r(conf, " ", &conf);
    if (!strcmp(token, "port:")) {
      char *token = strtok_r(conf, "\n", &conf);
      strcpy(host_server->s_port, token);
      integrity_test |= 1;
    }
    if (!strcmp(token, "host:")) {
      char *token = strtok_r(conf, "\n", &conf);
      strcpy(host_server->s_host, token);
      integrity_test |= 2;
    }
    if (!strcmp(token, "backlog:")) {
      char *token = strtok_r(conf, "\n", &conf);
      host_server->backlog = atoi(token);
      integrity_test |= 4;
    }
    if (!strcmp(token, "default_document:")) {
      char *token = strtok_r(conf, "\n", &conf);
      host_server->default_document = malloc(strlen(token) + 1);
      strcpy(host_server->default_document, token);
      integrity_test |= 8;
    }
    if (!strcmp(token, "root_folder:")) {
      char *token = strtok_r(conf, "\n", &conf);
      host_server->root_folder = malloc(strlen(token) + 1);
      strcpy(host_server->root_folder, token);
      integrity_test |= 16;
    }
    if (!strcmp(token, "crt_file_path:")) {
      char *token = strtok_r(conf, "\n", &conf);
      host_server->server_tls.crt_file_path = malloc(strlen(token) + 1);
      strcpy(host_server->server_tls.crt_file_path, token);
      integrity_test |= 32;
    }
    if (!strcmp(token, "pkey_file_path:")) {
      char *token = strtok_r(conf, "\n", &conf);
      host_server->server_tls.pkey_file_path = malloc(strlen(token) + 1);
      strcpy(host_server->server_tls.pkey_file_path, token);
      integrity_test |= 64;
    }
  }

  fclose(conf_file);
  free(seg_start);
  if (integrity_test != 127) {
    return OTHER_ERROR;
  }
  return 0;
}

void FreeServerParams(struct Server *host_server) {
  if (host_server->root_folder != NULL) {
    free(host_server->root_folder);
  }
  if (host_server->default_document != NULL) {
    free(host_server->default_document);
  }
  ServerTlsFree(&host_server->server_tls);
}