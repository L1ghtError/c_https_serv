#include "tls_adapter.h"

#include <stdlib.h>

#include "logger.h"
#include "mbedtls/error.h"
#include "utility.h"

void ServerTlsBaseInit(struct ServerTls *server_tls) {
  mbedtls_x509_crt_init(&server_tls->srvcert);
  mbedtls_ssl_config_init(&server_tls->conf);
  server_tls->session_tls.conf = &server_tls->conf;
  mbedtls_ctr_drbg_init(&server_tls->ctr_drbg);
  mbedtls_net_init(&server_tls->listen_fd);
  mbedtls_net_init(&server_tls->session_tls.client_fd);
  mbedtls_entropy_init(&server_tls->entropy);
  mbedtls_pk_init(&server_tls->pkey);
}

void ServerTlsFree(struct ServerTls *server_tls) {
  mbedtls_x509_crt_free(&server_tls->srvcert);
  mbedtls_ssl_config_free(&server_tls->conf);
  mbedtls_ctr_drbg_free(&server_tls->ctr_drbg);
  mbedtls_net_free(&server_tls->listen_fd);
  mbedtls_pk_free(&server_tls->pkey);
  if (server_tls->crt_file_path != NULL) {
    free(server_tls->crt_file_path);
  }
  if (server_tls->pkey_file_path != NULL) {
    free(server_tls->pkey_file_path);
  }
}

int ServerTlsSessionInit(struct ServerSessionTls *session_tls) {
  mbedtls_ssl_init(&session_tls->ssl);
  int ret = 0;
  if ((ret = mbedtls_ssl_setup(&session_tls->ssl, session_tls->conf)) != 0) {
    char *error_string = malloc(ERROR_STRING_SIZE);
    mbedtls_strerror(ret, error_string, ERROR_STRING_SIZE);
    log_printf("Server failed, ssl setup %s\n", error_string);
    free(error_string);
    return NETWORK_ERROR;
  }
  mbedtls_ssl_set_bio(&session_tls->ssl, &session_tls->client_fd,
                      mbedtls_net_send, mbedtls_net_recv, NULL);
  while ((ret = mbedtls_ssl_handshake(&session_tls->ssl)) != 0) {
    if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
      char *error_string = malloc(ERROR_STRING_SIZE);
      mbedtls_strerror(ret, error_string, ERROR_STRING_SIZE);
      log_printf("Server failed, ssl handshake %s\n", error_string);
      free(error_string);
      return NETWORK_ERROR;
    }
  }

  return 0;
}

void ServerTlsSessionFree(struct ServerSessionTls *session_tls) {
  mbedtls_net_free(&session_tls->client_fd);
  mbedtls_ssl_close_notify(&session_tls->ssl);
  mbedtls_ssl_free(&session_tls->ssl);
}

static void tls_debug_callback(void *ctx, int level, const char *file, int line,
                               const char *str) {
  (void)(level);
  fprintf((FILE *)ctx, "%s:%04d: %s", file, line, str);
  fflush((FILE *)ctx);
}

int LaunchTlsSocket(const char *server_host, const char *server_port,
                    struct ServerTls *server_tls) {
  if (mbedtls_ctr_drbg_seed(&server_tls->ctr_drbg, mbedtls_entropy_func,
                            &server_tls->entropy, NULL, 0) != 0) {
    printf("Server failed generating drbg seed\n");
    return NETWORK_ERROR;
  }
  int ret = 0;
  if ((ret = mbedtls_x509_crt_parse_file(&server_tls->srvcert,
                                         server_tls->crt_file_path)) != 0) {
    char *error_string = malloc(ERROR_STRING_SIZE);
    mbedtls_strerror(ret, error_string, ERROR_STRING_SIZE);
    printf("Server failed parse certificate %s\n", error_string);
    free(error_string);
    return NETWORK_ERROR;
  }

  mbedtls_pk_init(&server_tls->pkey);

  if (mbedtls_pk_parse_keyfile(&server_tls->pkey, server_tls->pkey_file_path,
                               "", mbedtls_ctr_drbg_random,
                               &server_tls->ctr_drbg) != 0) {
    printf("Server failed to parse private key\n");
    return NETWORK_ERROR;
  }

  if (mbedtls_ssl_conf_own_cert(&server_tls->conf, &server_tls->srvcert,
                                &server_tls->pkey) != 0) {
    printf("Server failed to set own certificate and private key\n");
    return NETWORK_ERROR;
  }
  mbedtls_ssl_conf_ca_chain(&server_tls->conf, &server_tls->srvcert, NULL);

  if (mbedtls_ssl_config_defaults(&server_tls->conf, MBEDTLS_SSL_IS_SERVER,
                                  MBEDTLS_SSL_TRANSPORT_STREAM,
                                  MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
    printf("Server failed mbedtls_ssl_config_defaults\n");
    return NETWORK_ERROR;
  }
  mbedtls_ssl_conf_rng(&server_tls->conf, mbedtls_ctr_drbg_random,
                       &server_tls->ctr_drbg);
  mbedtls_ssl_conf_dbg(&server_tls->conf, tls_debug_callback, stdout);
  if (mbedtls_net_bind(&server_tls->listen_fd, server_host, server_port,
                       MBEDTLS_NET_PROTO_TCP) != 0) {
    printf("Server failed mbedtls_net_bind returned\n");
    return NETWORK_ERROR;
  }
  return 0;
}