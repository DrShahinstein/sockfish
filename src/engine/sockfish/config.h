#pragma once

#include "sockfish.h"

#define SOCKFISH_INI "sockfish.ini"

#define SF_TT_SIZE_MB_DEFAULT 16
#define SF_TT_SIZE_MB_MIN      1
#define SF_TT_SIZE_MB_MAX      1024

#define SF_THREADS_DEFAULT 1
#define SF_THREADS_MIN     1
#define SF_THREADS_MAX     128

#define SF_MOVE_TIME_MS_DEFAULT 1000
#define SF_MOVE_TIME_MS_MIN     1
#define SF_MOVE_TIME_MS_MAX     3600000

void config_init_default(SF_Config *config);
void config_load(const char *filepath, SF_Config *config);
U64 config_get_modification_time(const char *filepath);
void config_save(const char *filepath, const SF_Config *config);

bool parse_bounded_int(const char *text, int min, int max, int *value);
