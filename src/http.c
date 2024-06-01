#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "logger.h"

void LogHttpRequestHeader(const struct HttpRequestHeader *http_header) {
  char log_time[LOG_TIME_SIZE];
  time_t now = time(NULL);
  struct tm *t = malloc(sizeof(struct tm));
  localtime_r(&now, t);

  sprintf(log_time, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
  char *log_data = malloc(strlen(log_time) + strlen(http_header->method) +
                          strlen(http_header->path) + 4);
  free(t);
  strcpy(log_data, log_time);
  strcat(log_data, " ");
  strcat(log_data, http_header->method);
  strcat(log_data, " ");
  strcat(log_data, http_header->path);
  strcat(log_data, "\n");
  LogMessage(LOGGER_FILENAME, log_data);
  free(log_data);
}

void FreeThreadData(struct ThreadData *thread_data) {
  if (thread_data->static_file != NULL) {
    fclose(thread_data->static_file);
  }
  if (thread_data->root_folder != NULL) {
    free(thread_data->root_folder);
  }
  if (thread_data->default_document != NULL) {
    free(thread_data->default_document);
  }
  ServerTlsSessionFree(&thread_data->session_tls);
  FreeHttpRequestHeader(&thread_data->http_header);
  free(thread_data);
}

int SendHttpHeader(struct ServerSessionTls *session_tls,
                   struct HttpResponseHeader *resp, char *append_data,
                   int data_size) {
  char *payload = NULL;
  resp->total_byte_length += data_size;

  if (resp->total_byte_length > MAX_CHUNK_SIZE) {
    resp->total_byte_length -= data_size;
    return NETWORK_ERROR;
  }

  ResponseToBuff(resp, &payload);

  if (append_data && data_size) {
    memcpy(payload + ((resp->total_byte_length - data_size) - 1), append_data,
           data_size);
  }

  if (mbedtls_ssl_write(&session_tls->ssl, payload,
                        (resp->total_byte_length - 1)) < 0) {
    resp->total_byte_length -= data_size;
    free(payload);
    return NETWORK_ERROR;
  }
  resp->total_byte_length -= data_size;
  free(payload);
  return 0;
}

int SendStaticData(struct ThreadData *thread_data) {
  struct HttpResponseHeader resp = {0};

  int total_data_size = thread_data->static_file_size;
  int size_that_should_be_readed = MAX_CHUNK_SIZE;
  int size_of_chunked_delim = 0;
  SetResponseStatusLine(&resp, HTTP_VERSION, kErrorDescriptions->error_code,
                        kErrorDescriptions->error_status);
  SetResponseContentType(&resp, thread_data->http_header.path);

  SetResponseProperty(&resp, "Server: IhorWebServer_2004\r\n");

  int amount_of_data = 0;
  if (thread_data->isKeepAlive) {
    SetResponseProperty(&resp, "Connection: Keep-Alive\r\n");
  } else {
    SetResponseProperty(&resp, "Connection: close\r\n");
  }
  if (thread_data->isChunkEncoding) {
    SetResponseProperty(&resp, "Transfer-Encoding: chunked\r\n");
    ComputeChunkEncodingData(
        &size_of_chunked_delim, &size_that_should_be_readed, &total_data_size,
        thread_data->response_buffer, MAX_CHUNK_SIZE - resp.total_byte_length);
    amount_of_data =
        fread(thread_data->response_buffer +
                  (SaturateValue(size_of_chunked_delim - SIZEOF_CRLF,
                                 size_of_chunked_delim)),
              1, size_that_should_be_readed, thread_data->static_file);
  } else {
    SetResponseContentLength(&resp, thread_data->static_file_size);
    amount_of_data = fread(
        thread_data->response_buffer + strlen(thread_data->response_buffer), 1,
        MAX_CHUNK_SIZE - resp.total_byte_length, thread_data->static_file);
  }
  SendHttpHeader(&thread_data->session_tls, &resp, thread_data->response_buffer,
                 amount_of_data + size_of_chunked_delim);

  if (total_data_size > 0) {
    do {
      if (thread_data->isChunkEncoding) {
        ComputeChunkEncodingData(&size_of_chunked_delim,
                                 &size_that_should_be_readed, &total_data_size,
                                 thread_data->response_buffer, MAX_CHUNK_SIZE);
      }
      amount_of_data =
          fread(thread_data->response_buffer +
                    (SaturateValue(size_of_chunked_delim - SIZEOF_CRLF,
                                   size_of_chunked_delim)),
                1, size_that_should_be_readed, thread_data->static_file);

      if (mbedtls_ssl_write(&thread_data->session_tls.ssl,
                            thread_data->response_buffer,
                            amount_of_data + size_of_chunked_delim) < 0) {
        printf("Server failed, write error\n");
        FreeHttpResponsetHeader(&resp);
        return NETWORK_ERROR;
      }
      memset(thread_data->response_buffer, 0, MAX_CHUNK_SIZE);
    } while (amount_of_data);
  }
  if (thread_data->isChunkEncoding) {
    if (mbedtls_ssl_write(&thread_data->session_tls.ssl, "0\r\n\r\n", 5) < 0) {
      printf("Server failed, write chunk ending error");
    }
  } else if (!thread_data->isKeepAlive) {
    SetResponseProperty(&resp, "Connection: close\r\n");
    SendHttpHeader(&thread_data->session_tls, &resp, NULL, 0);
  }
  FreeHttpResponsetHeader(&resp);
  return 0;
}

void HandleError(enum HttpErrorCode error_code, const char *problem_name,
                 struct ThreadData *thread_data) {
  struct HttpResponseHeader resp = {0};
  char *error_payload = NULL;

  for (size_t i = 0; i < ARRLEN(kErrorDescriptions); i++) {
    if (abs(error_code) == i) {
      SetResponseStatusLine(&resp, HTTP_VERSION,
                            kErrorDescriptions[i].error_code,
                            kErrorDescriptions[i].error_status);
      error_payload = malloc(strlen(problem_name) +
                             strlen(kErrorDescriptions[i].error_status) + 2);
      strcpy(error_payload, problem_name);
      strcat(error_payload, " ");
      strcat(error_payload, kErrorDescriptions[i].error_status);
      break;
    }
  }

  SetResponseProperty(&resp, "Content-Type: text/plain\r\n");
  SetResponseProperty(&resp, "Connection: close\r\n");
  char *header_buff = NULL;
  ResponseToBuff(&resp, &header_buff);
  strcpy(thread_data->response_buffer, header_buff);
  strcat(thread_data->response_buffer, error_payload);
  mbedtls_ssl_write(&thread_data->session_tls.ssl, thread_data->request_buffer,
                    strlen(thread_data->response_buffer));
  free(header_buff);
  free(error_payload);
  FreeHttpResponsetHeader(&resp);
}

void *HandleConnection(void *thread_data_raw) {
  struct ThreadData *thread_data = thread_data_raw;
  if (ServerTlsSessionInit(&thread_data->session_tls) == NETWORK_ERROR) {
    log_printf("server session init error\n");
    FreeThreadData(thread_data);
    return 0;
  }
  int ret_code = 0;

  do {
    if ((ret_code = mbedtls_ssl_read(&thread_data->session_tls.ssl,
                                     thread_data->request_buffer,
                                     MAX_CHUNK_SIZE)) == OTHER_ERROR) {
      log_perror("server recv error");
      FreeThreadData(thread_data);
      return 0;
    }
    if (ret_code == 0) {
      if (thread_data->isKeepAlive) {
        CloseConnection(thread_data);
      }
      FreeThreadData(thread_data);
      return 0;
    }

    ret_code = FillHttpRequestHeader(&thread_data->http_header,
                                     thread_data->request_buffer);
    if (ret_code < 0) {
      printf("Header corrupted\n");
      HandleError(ret_code, "Your request", thread_data);
      FreeThreadData(thread_data);
      return 0;
    }
    SetDefaultDocument(&thread_data->http_header,
                       thread_data->default_document);
    LogHttpRequestHeader(&thread_data->http_header);

    thread_data->static_file_size =
        OpenStaticFile(thread_data->http_header.path, thread_data->root_folder,
                       &thread_data->static_file);

    if (thread_data->static_file_size < 0) {
      HandleError(thread_data->static_file_size, thread_data->http_header.path,
                  thread_data);
      FreeThreadData(thread_data);
      return 0;
    }
    thread_data->isKeepAlive =
        (stristr(thread_data->http_header.body, "Keep-Alive") != NULL);

    char *accept_encoding =
        stristr(thread_data->http_header.body, "Accept-Encoding: ");
    if (accept_encoding != NULL) {
      thread_data->isChunkEncoding =
          (stristr(accept_encoding, "chunked") != NULL);
    }

    if (SendStaticData(thread_data) == -1) {
      log_perror("server send error");
      FreeThreadData(thread_data);
      return 0;
    }
    if (thread_data->isKeepAlive) {
      FlushThreadData(thread_data);
    }
  } while (thread_data->isKeepAlive);
  FreeThreadData(thread_data);
  return 0;
}

void SetDefaultDocument(struct HttpRequestHeader *http_header,
                        const char *default_document) {
  if (!strcmp(http_header->path, "/")) {
    free(http_header->path);
    http_header->path = malloc(strlen(default_document) + 2);
    strcpy(http_header->path, "/");
    strcat(http_header->path, default_document);
  }
}

void FlushThreadData(struct ThreadData *thread_data) {
  memset(thread_data->request_buffer, 0, MAX_CHUNK_SIZE);
  memset(thread_data->response_buffer, 0, MAX_CHUNK_SIZE);
  fclose(thread_data->static_file);
  thread_data->static_file = NULL;
  thread_data->static_file_size = 0;
  thread_data->isChunkEncoding = 0;
  FreeHttpRequestHeader(&thread_data->http_header);
}

void CloseConnection(struct ThreadData *thread_data) {
  struct HttpResponseHeader resp = {0};
  SetResponseStatusLine(&resp, HTTP_VERSION, kErrorDescriptions->error_code,
                        kErrorDescriptions->error_status);
  SetResponseProperty(&resp, "Connection: close\r\n");
  SetResponseProperty(&resp, "Content-Length: 0\r\n");
  if (SendHttpHeader(&thread_data->session_tls, &resp,
                     thread_data->response_buffer,
                     thread_data->static_file_size) == NETWORK_ERROR) {
    printf("Server, send close header failed\n");
  }
  FreeHttpResponsetHeader(&resp);
}

void ComputeChunkEncodingData(int *size_of_chunked_delim,
                              int *size_that_should_be_readed,
                              int *total_data_size, char *buffer,
                              int max_chunk_size) {
  int saturated_size = SaturateValue(*total_data_size, max_chunk_size);
  *size_of_chunked_delim =
      SIZEOF_TWO_CRLF +
      PowerOfHex(saturated_size - SIZEOF_TWO_CRLF + PowerOfHex(saturated_size));
  *size_that_should_be_readed =
      SaturateValue(saturated_size, max_chunk_size - *size_of_chunked_delim);
  *total_data_size -= *size_that_should_be_readed;

  snprintf(buffer, ((*size_of_chunked_delim - SIZEOF_TWO_CRLF) + 1), "%x",
           *size_that_should_be_readed);
  char *crlf = (char *)"\r\n";
  strncpy(buffer + (*size_of_chunked_delim - SIZEOF_TWO_CRLF), crlf, 2);

  strcpy(buffer + ((*size_of_chunked_delim - SIZEOF_CRLF) +
                   *size_that_should_be_readed),
         "\r\n");
}