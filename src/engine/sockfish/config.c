#include "config.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

static void trim_key(char *key);

void config_init_default(SF_Config *config) {
  if (config == NULL)
    return;

  config->tt_size_mb   = SF_TT_SIZE_MB_DEFAULT;
  config->threads      = SF_THREADS_DEFAULT;
  config->move_time_ms = SF_MOVE_TIME_MS_DEFAULT;
}

void config_load(const char *filepath, SF_Config *config) {
  if (config == NULL)
    return;

  config_init_default(config);

  if (filepath == NULL)
    return;

  FILE *file = fopen(filepath, "r");

  if (!file) {
    config_save(filepath, config);
    return;
  }

  char line[256];
  while (fgets(line, sizeof(line), file)) {
    char key[128], value[128];

    if (sscanf(line, " %127[^=]= %127s", key, value) != 2)
      continue;

    trim_key(key);

    int parsed;
    if (strcmp(key, "tt_size_mb") == 0 && parse_bounded_int(value, SF_TT_SIZE_MB_MIN, SF_TT_SIZE_MB_MAX, &parsed)) {
      config->tt_size_mb = parsed;
    }
    else if (strcmp(key, "threads") == 0 && parse_bounded_int(value, SF_THREADS_MIN, SF_THREADS_MAX, &parsed)) {
      config->threads = parsed;
    }
    else if (strcmp(key, "move_time_ms") == 0 && parse_bounded_int(value, SF_MOVE_TIME_MS_MIN, SF_MOVE_TIME_MS_MAX, &parsed)) {
      config->move_time_ms = parsed;
    }
  }

  fclose(file);
}

U64 config_get_modification_time(const char *filepath) {
  if (filepath == NULL)
    return 0;

  struct stat attr;
  if (stat(filepath, &attr) == 0)
    return (U64)attr.st_mtime;
  return 0;
}

void config_save(const char *filepath, const SF_Config *config) {
  if (filepath == NULL || config == NULL)
    return;

  FILE *file = fopen(filepath, "w");

  if (file) {
    fprintf(file, "tt_size_mb=%d\n",   config->tt_size_mb);
    fprintf(file, "threads=%d\n",      config->threads);
    fprintf(file, "move_time_ms=%d\n", config->move_time_ms);
    fclose(file);
  }
}

bool parse_bounded_int(const char *text, int min, int max, int *value) {
  if (text == NULL || value == NULL)
    return false;

  errno       = 0;
  char *end   = NULL;
  long parsed = strtol(text, &end, 10);

  if (end == text || errno == ERANGE || parsed < min || parsed > max)
    return false;

  while (isspace((unsigned char)*end))
    end++;

  if (*end != '\0')
    return false;

  *value = (int)parsed;
  return true;
}

static void trim_key(char *key) {
  size_t length = strlen(key);
  while (length > 0 && (key[length - 1] == ' ' || key[length - 1] == '\t')) {
    key[--length] = '\0';
  }
}
