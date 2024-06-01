#ifndef HTTP_H
#define HTTP_H

#define MAX_CHUNK_SIZE 8192
#define LOG_TIME_SIZE 9
#define HTTP_VERSION "HTTP/1.1"

#include "http_parser.h"
#include "tls_adapter.h"
#include "utility.h"

struct ThreadData {
  struct ServerSessionTls session_tls;
  char request_buffer[MAX_CHUNK_SIZE];
  char response_buffer[MAX_CHUNK_SIZE];
  struct HttpRequestHeader http_header;
  FILE *static_file;
  long static_file_size;
  char *default_document;
  char *root_folder;
  int isKeepAlive;
  int isChunkEncoding;
};
// Frees all ThreadData, after calling this function it should not be used to
// handle the connection
void FreeThreadData(struct ThreadData *thread_data);

// Based on the error code, sends the corresponding HTTP message to the user
void HandleError(enum HttpErrorCode error_code, const char *problem_name,
                 struct ThreadData *thread_data);

void *HandleConnection(void *thread_data_raw);

void LogHttpRequestHeader(const struct HttpRequestHeader *http_header);

// based on thread_data sends the file to the appropriate socket, return
// NETWORK_ERROR on failure
int SendStaticData(struct ThreadData *thread_data);

// Sends an http header to the appropriate user and may append data to the
// header if the total size is less than MAX_CHUNK_SIZE
// Returns NETWORK_ERROR if something goes wrong
int SendHttpHeader(struct ServerSessionTls *session_tls,
                   struct HttpResponseHeader *resp, char *append_data,
                   int data_size);

void SetDefaultDocument(struct HttpRequestHeader *http_header,
                        const char *default_document);

// Frees part of ThreadData so it can handle a new receive cycle
void FlushThreadData(struct ThreadData *thread_data);

// Sends "connection: close" to the appropriate user
void CloseConnection(struct ThreadData *thread_data);

// Calculates the size of the data to be read and the associated variables,
//  size_of_chunked_delim - size of "\r\n\r\n" + size of file data in hex
//  size_that_should_be_readed - maximum available file size
//  total_data_size - the size that needs to be divided into chunks
//  buffer - form for chunked encoding, will contain size and newline chars
//   max_chunk_size - max possible size_that_should_be_readed
void ComputeChunkEncodingData(int *size_of_chunked_delim,
                              int *size_that_should_be_readed,
                              int *total_data_size, char *buffer,
                              int max_chunk_size);
#endif  // HTTP_H
