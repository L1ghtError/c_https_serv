#include "logger.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define DATE_STRING_SIZE 30
#define LOGGER_VERSION "#Version: 1.0\n"
#define LOGGER_FIELDS "#Fields: time cs-method cs-uri\n"

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;

void LogInit(char *filename) {
  time_t current_time;
  struct tm *time_info = malloc(sizeof(struct tm));

  time(&current_time);
  time_info = localtime_r(&current_time, time_info);

  char logger_date[DATE_STRING_SIZE];
  strftime(logger_date, DATE_STRING_SIZE, "#Date: %d-%b-%Y %H:%M:%S\n",
           time_info);
  free(time_info);
  char *logger_version = LOGGER_VERSION;
  char *logger_fields = LOGGER_FIELDS;
  char *logger_header = malloc(strlen(logger_version) + strlen(logger_date) +
                               strlen(logger_fields) + 1);

  strcpy(logger_header, logger_version);
  strcat(logger_header, logger_date);
  strcat(logger_header, logger_fields);

  if (access(filename, F_OK) == -1) {
    LogMessage(filename, logger_header);
  }
  free(logger_header);
}

int LogMessage(char *filename, char *message) {
  pthread_mutex_lock(&server_mutex);
  FILE *log_fd;
  log_fd = fopen(filename, "a");

  if (log_fd == NULL) {
    printf("Error opening file");
    return LOGGER_ERROR;
  }
  fprintf(log_fd, "%s", message);

  fclose(log_fd);
  pthread_mutex_unlock(&server_mutex);
  return 0;
}

void log_perror(const char *msg) {
  pthread_mutex_lock(&stdout_mutex);
  perror(msg);
  pthread_mutex_unlock(&server_mutex);
}

void log_printf(const char *format, ...) {
  pthread_mutex_lock(&stdout_mutex);

  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  pthread_mutex_unlock(&stdout_mutex);
}