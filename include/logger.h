#ifndef LOGGER_H
#define LOGGER_H
#define LOGGER_ERROR -1
void LogInit(char *filename);
#include <stdarg.h>
// Opens the filename and writes a message to it, returns LOGGER_ERROR in case
// of error
int LogMessage(char *filename, char *message);

// A wrapper over an perror with a built-in mutex
void log_perror(const char *msg);

// A wrapper over an printf with a built-in mutex
void log_printf(const char *format, ...);
#endif  // LOGGER_H