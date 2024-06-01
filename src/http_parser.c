#include "http_parser.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "utility.h"

static const struct MIMEtype kMIMETypes[] = {
    {".aac", "audio/aac"},
    {".abw", "application/x-abiword"},
    {".arc", "application/x-freearc"},
    {".avi", "video/x-msvideo"},
    {".azw", "application/vnd.amazon.ebook"},
    {".bin", "application/octet-stream"},
    {".bmp", "image/bmp"},
    {".bz", "application/x-bzip"},
    {".bz2", "application/x-bzip2"},
    {".cda", "application/x-cdf"},
    {".csh", "application/x-csh"},
    {".css", "text/css"},
    {".csv", "text/csv"},
    {".doc", "application/msword"},
    {".docx",
     "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".eot", "application/vnd.ms-fontobject"},
    {".epub", "application/epub+zip"},
    {".gz", "application/gzip"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".ico", "image/vnd.microsoft.icon"},
    {".ics", "text/calendar"},
    {".jar", "application/java-archive"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".jsonld", "application/ld+json"},
    {".mid", "audio/midi audio/x-midi"},
    {".midi", "audio/midi audio/x-midi"},
    {".mjs", "text/javascript"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"},
    {".mpeg", "video/mpeg"},
    {".mpkg", "application/vnd.apple.installer+xml"},
    {".odp", "application/vnd.oasis.opendocument.presentation"},
    {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {".odt", "application/vnd.oasis.opendocument.text"},
    {".oga", "audio/ogg"},
    {".ogv", "video/ogg"},
    {".ogx", "application/ogg"},
    {".opus", "audio/opus"},
    {".otf", "font/otf"},
    {".png", "image/png"},
    {".pdf", "application/pdf"},
    {".php", "application/x-httpd-php"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx",
     "application/"
     "vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".rar", "application/vnd.rar"},
    {".rtf", "application/rtf"},
    {".sh", "application/x-sh"},
    {".svg", "image/svg+xml"},
    {".swf", "application/x-shockwave-flash"},
    {".tar", "application/x-tar"},
    {".tif", "image/tiff"},
    {".tiff", "image/tiff"},
    {".ts", "video/mp2t"},
    {".ttf", "font/ttf"},
    {".txt", "text/plain"},
    {".vsd", "application/vnd.visio"},
    {".wav", "audio/wav"},
    {".weba", "audio/webm"},
    {".webm", "video/webm"},
    {".webp", "image/webp"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".xhtml", "application/xhtml+xml"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx",
     "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".xml", "application/xml"},
    {".xul", "application/vnd.mozilla.xul+xml"},
    {".zip", "application/zip"},
    {".3gp", "video/3gpp"},
    {".3g2", "video/3gpp2"},
    {".7z", "application/x-7z-compressed"}};

void ExpandBodySize(struct HttpResponseHeader *http_resp) {
  char **new_body =
      malloc(sizeof(new_body) * (http_resp->number_of_body_fields * 2));
  memcpy(new_body, http_resp->body,
         (sizeof(new_body) * http_resp->number_of_body_fields));
  free(http_resp->body);
  http_resp->body = new_body;
}

void SetResponseContentType(struct HttpResponseHeader *http_resp,
                            char *filename) {
  int is_extension_match = 0;
  for (size_t i = 0; i < ARRLEN(kMIMETypes); i++) {
    if (stristr(filename, kMIMETypes[i].extension)) {
      char *property =
          malloc(strlen("Content-Type: \r\n") + strlen(kMIMETypes[i].type) + 1);
      strcpy(property, "Content-Type: ");
      strcat(property, kMIMETypes[i].type);
      strcat(property, "\r\n");
      SetResponseProperty(http_resp, property);
      free(property);
      is_extension_match = 1;
    }
  }
  if (is_extension_match == 0) {
    SetResponseProperty(http_resp, "Content-Type: text/plain\r\n");
  }
}

void SetResponseContentLength(struct HttpResponseHeader *http_resp, int size) {
  int number_of_digits = 0;
  int size_copy = size;
  do {
    ++number_of_digits;
    size_copy /= 10;
  } while (size_copy);
  char *buff = malloc(strlen("Content-Length: \r\n") + number_of_digits + 1);

  strcpy(buff, "Content-Length: ");
  sprintf(buff + strlen(buff), "%d", size);
  strcat(buff, "\r\n");
  SetResponseProperty(http_resp, buff);
  free(buff);
}

void SetResponseStatusLine(struct HttpResponseHeader *http_resp, char *version,
                           char *status_code, char *status) {
  http_resp->version = malloc(strlen(version) + 1);
  http_resp->status_code = malloc(strlen(status_code) + 1);
  http_resp->status = malloc(strlen(status) + 1);
  strcpy(http_resp->version, version);
  strcpy(http_resp->status_code, status_code);
  strcpy(http_resp->status, status);
  http_resp->total_byte_length +=
      (strlen(version) + strlen(status_code) + strlen(status) +
       SIZEOF_HEADER_DELIMINATIONS);
}

void SetResponseProperty(struct HttpResponseHeader *http_resp, char *property) {
  if (http_resp->number_of_body_fields == 0) {
    http_resp->body = malloc(sizeof(http_resp->body) * 2);
  } else if (http_resp->number_of_body_fields == HTTP_BODY_FIELD_LIMIT) {
    return;
  } else if (http_resp->number_of_body_fields ==
             NextHighestPowerOfTwo(http_resp->number_of_body_fields)) {
    ExpandBodySize(http_resp);
  }

  int property_key_length = GetKeywordIndex(property, " ");
  if (property_key_length == OTHER_ERROR) {
    return;
  }
  char *property_key = malloc(property_key_length + 1);
  strncpy(property_key, property, property_key_length);
  property_key[property_key_length] = '\0';

  for (int i = 0; i < http_resp->number_of_body_fields; i++) {
    if (strstr(http_resp->body[i], property_key) != NULL) {
      free(property_key);
      http_resp->total_byte_length +=
          (strlen(property) - strlen(http_resp->body[i]));
      http_resp->body[i] = realloc(http_resp->body[i], strlen(property) + 1);
      strcpy(http_resp->body[i], property);
      return;
    }
  }
  free(property_key);
  http_resp->total_byte_length += (strlen(property));
  http_resp->body[http_resp->number_of_body_fields] =
      malloc(strlen(property) + 1);
  strcpy(http_resp->body[http_resp->number_of_body_fields], property);
  http_resp->number_of_body_fields++;
}

void FreeHttpRequestHeader(struct HttpRequestHeader *http_header) {
  if (http_header == NULL) {
    return;
  }
  if (http_header->method != NULL) {
    free(http_header->method);
    http_header->method = NULL;
  }
  if (http_header->path != NULL) {
    free(http_header->path);
    http_header->path = NULL;
  }
  if (http_header->version != NULL) {
    free(http_header->version);
    http_header->version = NULL;
  }
  if (http_header->body != NULL) {
    free(http_header->body);
    http_header->body = NULL;
  }
}

void FreeHttpResponsetHeader(struct HttpResponseHeader *http_header) {
  if (http_header == NULL) {
    return;
  }
  if (http_header->version != NULL) {
    free(http_header->version);
  }
  if (http_header->status_code != NULL) {
    free(http_header->status_code);
  }
  if (http_header->status != NULL) {
    free(http_header->status);
  }
  for (int i = 0; i < http_header->number_of_body_fields; i++) {
    free(http_header->body[i]);
  }
  if (http_header->body != NULL) {
    free(http_header->body);
  }
}

int FillHttpRequestHeader(struct HttpRequestHeader *http_header, char *buffer) {
  int http_header_length = GetKeywordIndex(buffer, "\r\n\r\n");

  if (http_header_length == OTHER_ERROR) {
    return kBadRequest;
  }

  char *http_header_raw = malloc(http_header_length + 1);
  strncpy(http_header_raw, buffer, http_header_length + 1);
  http_header_raw[http_header_length] = '\0';

  char *header_arg[HTTP_HEAD_COUNT];

  // The address of http_header_raw will be offset, so we need seg_start
  char *seg_start = http_header_raw;
  for (int i = 0; i < HTTP_HEAD_COUNT; i++) {
    char *token = strtok_r(http_header_raw, " \r", &http_header_raw);

    if (token == NULL || strlen(token) > MAX_URL_SIZE) {
      free(http_header_raw);
      FreeHttpRequestHeader(http_header);
      return kBadRequest;
    }

    header_arg[i] = malloc(strlen(token) + 1);
    strcpy(header_arg[i], token);
  }

  http_header->method = header_arg[0];
  http_header->path = header_arg[1];
  http_header->version = header_arg[2];

  if (stristr(http_header->method, "GET") == NULL) {
    return kNotImplemented;
  }

  char *body_start = strstr(http_header_raw, "\n");
  if (body_start != NULL) {
    body_start += 1;
    http_header->body = malloc(strlen(body_start) + 1);
    strcpy(http_header->body, body_start);
  } else {
    http_header->body = NULL;
  }
  free(seg_start);
  return 0;
}

int ResponseToBuff(struct HttpResponseHeader *http_resp, char **buff) {
  if (http_resp->version == NULL || http_resp->status_code == NULL ||
      http_resp->status == NULL) {
    return OTHER_ERROR;
  }
  *buff = malloc(http_resp->total_byte_length);
  strcpy(*buff, http_resp->version);
  strcat(*buff, " ");
  strcat(*buff, http_resp->status_code);
  strcat(*buff, " ");
  strcat(*buff, http_resp->status);
  strcat(*buff, "\r\n");
  for (int i = 0; i < http_resp->number_of_body_fields; i++) {
    strcat(*buff, http_resp->body[i]);
  }
  strcat(*buff, "\r\n");
  return 0;
}