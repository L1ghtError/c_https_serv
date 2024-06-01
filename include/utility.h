#ifndef UTILITY_H
#define UTILITY_H

#define LOGGER_FILENAME "log.txt"
#define MAX_PORTLEN 6

#define NETWORK_ERROR -1
#define OTHER_ERROR NETWORK_ERROR

#include <netdb.h>
#include <stdio.h>

struct ErrorDescription {
  char *error_code;
  char *error_status;
};

enum HttpErrorCode { kNotFound = -1, kBadRequest = -2, kNotImplemented = -3 };

static const struct ErrorDescription kErrorDescriptions[] = {
    {"200", "OK"},
    {"404", "NOT FOUND"},
    {"400", "BAD REQUEST"},
    {"501", "NOT IMPLEMENTED"}};

#define ARRLEN(x) (sizeof(x) / sizeof(x[0]))

// return sin_addr or sin6_addr dependent on passed sockaddr
void *GetInAddr(const struct sockaddr_storage *sa);

// Fills *s with Ip addres of sa
void GetSockaddrIp(const struct sockaddr_storage *sa, char *s, int buflen);

// This function returns an integer value that needs to be split into two parts,
// each consisting of two bytes. The first part represents the starting position
// of the filename, while the second part represents the length of the filename.
int GetFilePosition(char *src);

// Open the static file.
// If http_header->path is corrupted, return kBadRequest.
// If the file is not found, return kNotFound.
// If the file is successfully opened, return the size of the file.
long OpenStaticFile(char *file_name, const char *static_file_dir,
                    FILE **static_file);

int GetKeywordIndex(const char *src_buffer, const char *keyword);

// https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
int NextHighestPowerOfTwo(int val);

// Return 1 if to char* identical and 0 if not, until one ends
int CompareAnyCaseSym(const char *X, const char *Y);

// Same as strstr but case insensitive
char *stristr(const char *str1, const char *str2);

// returns the size of a number to represent in hexadecimal
int PowerOfHex(int number);

// returns the maximum value with the max_value threshold, or zero for negative
// values
int SaturateValue(int value, int max_value);

#endif  // UTILITY_H