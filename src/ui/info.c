#include "ui.h"

static struct {
  char message[MAX_INFO_LENGTH];
  SDL_Mutex *mutex;
} info_state = {0};

void info_system_init(void) {
  info_state.mutex = SDL_CreateMutex();
  SDL_strlcpy(info_state.message, "INFO: none", MAX_INFO_LENGTH);
}

void info_system_cleanup(void) {
  if (info_state.mutex) {
    SDL_DestroyMutex(info_state.mutex);
    info_state.mutex = NULL;
  }
}

void ui_set_info(const char *msg, ...) {
  if (!info_state.mutex) return;

  SDL_LockMutex(info_state.mutex);

  char buf[MAX_INFO_LENGTH];
  va_list args;
  va_start(args, msg);
  SDL_vsnprintf(buf, MAX_INFO_LENGTH, msg, args);
  va_end(args);

  SDL_snprintf(info_state.message, MAX_INFO_LENGTH, "INFO: %s", buf);

  SDL_UnlockMutex(info_state.mutex);
}

inline const char *get_info_message(void) {
  return info_state.message;
}