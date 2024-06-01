#ifndef TLS_ADAPTER_H
#define TLS_ADAPTER_H

#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/net_sockets.h"

#define ERROR_STRING_SIZE 1024

struct ServerSessionTls {
  mbedtls_net_context client_fd;
  mbedtls_ssl_context ssl;
  const mbedtls_ssl_config *conf;
};

struct ServerTls {
  struct ServerSessionTls session_tls;
  mbedtls_net_context listen_fd;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_ssl_config conf;
  mbedtls_x509_crt srvcert;
  mbedtls_pk_context pkey;
  char *crt_file_path;
  char *pkey_file_path;
};

void ServerTlsBaseInit(struct ServerTls *server_tls);

void ServerTlsFree(struct ServerTls *server_tls);

int ServerTlsSessionInit(struct ServerSessionTls *session_tls);

void ServerTlsSessionFree(struct ServerSessionTls *session_tls);

int LaunchTlsSocket(const char *server_host, const char *server_port,
                    struct ServerTls *server_tls);
#endif  // TLS_ADAPTER_H