#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define HTTP_HEAD_COUNT 3
#define HTTP_BODY_FIELD_LIMIT 128
#define MAX_URL_SIZE 2048
// this macro represents 2 spaces,2 \r, 2 \n and \0.
#define SIZEOF_HEADER_DELIMINATIONS 7
#define SIZEOF_CRLF 2
#define SIZEOF_TWO_CRLF (SIZEOF_CRLF * 2)

struct HttpResponseHeader {
  char *version;
  char *status_code;
  char *status;
  char **body;
  int number_of_body_fields;
  int total_byte_length;
};

struct HttpRequestHeader {
  char *method;
  char *path;
  char *version;
  char *body;
};

struct MIMEtype {
  char *extension;
  char *type;
};

void SetResponseContentType(struct HttpResponseHeader *http_resp,
                            char *filename);

void SetResponseContentLength(struct HttpResponseHeader *http_resp, int size);

void FreeHttpRequestHeader(struct HttpRequestHeader *http_header);

void FreeHttpResponsetHeader(struct HttpResponseHeader *http_header);

// Fills http_header structure with data from buffer
// If buffer is corrupted, return kBadRequest.
// If method differences by GET, returns kNotImplemented
// returns 0 on success
int FillHttpRequestHeader(struct HttpRequestHeader *http_header, char *buffer);

// Assigns version/status_code/status to the corresponding http_resp fields
void SetResponseStatusLine(struct HttpResponseHeader *http_resp, char *version,
                           char *status_code, char *status);

// Assigns a string to an empty body[n], if the key (which is separated by ' '
// character) already exists in one of the buffers, then it will overwrite whole
// body[n]
void SetResponseProperty(struct HttpResponseHeader *http_resp, char *property);

// Reallocates http_resp.body and doubles its size
void ExpandBodySize(struct HttpResponseHeader *http_resp);

// Allocates buffer regarding on total_byte_length of http_resp, and passes the
// http_resp fields to it
int ResponseToBuff(struct HttpResponseHeader *http_resp, char **buff);

#endif  // HTTP_PARSER_H