#include "config_save.h"

unsigned int flash_target_offset = (
  (2048 * 1024) // End of the 2MB of the Pico's flash
  - (sizeof(AppConfig)/FLASH_PAGE_SIZE + 1)*FLASH_PAGE_SIZE  // Size of the config struct, aligned to 256bytes
  ) & 0xFFFFF000; // no fucking clue

void saveConfig(AppConfig config) {
  unsigned int dataSize = (sizeof(config)/FLASH_PAGE_SIZE + 1) * FLASH_PAGE_SIZE;

  unsigned char* dataPtr = (unsigned char*)malloc(dataSize);
  if (dataPtr){
    memcpy(dataPtr, &config, sizeof(config));
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_target_offset, FLASH_SECTOR_SIZE);
    flash_range_program(flash_target_offset, dataPtr, dataSize);
    restore_interrupts(ints);
  }
}

AppConfig* loadConfig() {
  const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + flash_target_offset);
  AppConfig *config = (AppConfig*)flash_target_contents;
  return config;
}