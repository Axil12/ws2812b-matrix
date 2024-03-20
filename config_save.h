#ifndef CONFIG_SAVE_H
#define CONFIG_SAVE_H
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <cstdlib>
#include <cstring>

struct AppConfig {
  int selected_program;
  float brightness;
};

void saveConfig(AppConfig config);
AppConfig* loadConfig();

#endif