#include "utility.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

void *GetInAddr(const struct sockaddr_storage *sa) {
  if (sa != NULL) {
    if (sa->ss_family == AF_INET) {
      return &((struct sockaddr_in *)sa)->sin_addr;
    } else {
      return &((struct sockaddr_in6 *)sa)->sin6_addr;
    }
  }
  return NULL;
}

void GetSockaddrIp(const struct sockaddr_storage *sa, char *s, int buflen) {
  inet_ntop(sa->ss_family, GetInAddr(sa), s, buflen);
}

int GetFilePosition(char *src) {
  char *sus_word = "../";
  char *it = src;
  while ((it = strstr(src, sus_word)) != NULL) {
    memset(&src[it - src], ' ', strlen(sus_word));
  }

  it = src;
  for (int i = 0; i < (int)strlen(src); i++) {
    if (src[i] != ' ') {
      if (&src[i] != it) {
        *it = src[i];
      }
      it++;
    }
  }
  if (*it != '\0') {
    *it = '\0';
  }
  int resault = 0;
  resault = resault << 16;
  resault |= it - src;
  return resault;
}

long OpenStaticFile(char *file_name, const char *static_file_dir,
                    FILE **static_file) {
  int position = GetFilePosition(file_name);
  if (position == 0) {
    printf("parse path: %s\n", file_name);
    return kBadRequest;
  }

  int start = position >> 16;
  int length = (__INT_MAX__ >> 16) & position;

  char *filename = malloc(strlen(static_file_dir) + length + 1);
  strcpy(filename, static_file_dir);
  strncat(filename, file_name + start, length + 1);

  *static_file = fopen(filename, "rb");
  free(filename);
  if (*static_file == NULL) {
    return kNotFound;
  }

  fseek(*static_file, 0, SEEK_END);
  long file_size = ftell(*static_file);
  fseek(*static_file, 0, SEEK_SET);

  return file_size;
}

int GetKeywordIndex(const char *src_buffer, const char *keyword) {
  char *header_end = strstr(src_buffer, keyword);
  if (header_end == NULL) {
    return OTHER_ERROR;
  }
  return (header_end - src_buffer) + strlen(keyword);
}

int NextHighestPowerOfTwo(int v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
}

int CompareAnyCaseSym(const char *X, const char *Y) {
  while (*X && *Y) {
    if (tolower(*X) != tolower(*Y)) {
      return 0;
    }

    X++;
    Y++;
  }

  return (*Y == '\0');
}

char *stristr(const char *str1, const char *str2) {
  while (*str1 != '\0') {
    if ((tolower(*str1) == tolower(*str2)) && CompareAnyCaseSym(str1, str2)) {
      return (char *)str1;
    }
    str1++;
  }

  return NULL;
}

int PowerOfHex(int number) {
  number = abs(number);
  int pow_of_hex = 0;
  while (number) {
    number = number >> 4;
    pow_of_hex++;
  }
  return pow_of_hex;
}

int SaturateValue(int value, int max_value) {
  if (value <= 0 || max_value <= 0) {
    return 0;
  }
  if (value >= max_value) {
    return ((value - (value % max_value)) / ((value / max_value)));
  } else {
    return value % max_value;
  }
}