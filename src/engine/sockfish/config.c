#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

void config_init_default(SF_Config *config) {
  config->tt_size_mb   = 16;
  config->threads      = 1;
  config->move_time_ms = 3000;
}

void config_load(const char *filepath, SF_Config *config) {
  config_init_default(config);

  FILE *file = fopen(filepath, "r");

  if (!file) {
    config_save(filepath, config);
    return;
  }

  char line[256];
  while (fgets(line, sizeof(line), file)) {
    char key[128], value[128];

    if (sscanf(line, " %127[^=] = %127s ", key, value) == 2) {
      if (strcmp(key, "tt_size_mb") == 0)   config->tt_size_mb   = atoi(value);
      if (strcmp(key, "threads") == 0)      config->threads      = atoi(value);
      if (strcmp(key, "move_time_ms") == 0) config->move_time_ms = atoi(value);
    }
  }

  fclose(file);
}

U64 config_get_modification_time(const char *filepath) {
  struct stat attr;
  if (stat(filepath, &attr) == 0)
    return (U64)attr.st_mtime;
  return 0;
}

void config_save(const char *filepath, const SF_Config *config) {
  FILE *file = fopen(filepath, "w");

  if (file) {
    fprintf(file, "tt_size_mb=%d\n",   config->tt_size_mb);
    fprintf(file, "threads=%d\n",      config->threads);
    fprintf(file, "move_time_ms=%d\n", config->move_time_ms);
    fclose(file);
  }
}

