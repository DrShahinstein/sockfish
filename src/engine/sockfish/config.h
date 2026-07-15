#pragma once

#include "sockfish.h"

#define SOCKFISH_INI "sockfish.ini"

void config_init_default(SF_Config *config);
void config_load(const char *filepath, SF_Config *config);
U64 config_get_modification_time(const char *filepath);
void config_save(const char *filepath, const SF_Config *config);

