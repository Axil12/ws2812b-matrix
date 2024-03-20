#ifndef UTILS_H
#define UTILS_H
#include <Arduino.h>
#include <Adafruit_NeoMatrix.h>

uint16_t ColorHSV(uint16_t hue, uint8_t sat, uint8_t val);
float matrixCurrentDraw(Adafruit_NeoMatrix &matrix, float current_per_channel);
float matrixPowerDraw(Adafruit_NeoMatrix &matrix, float current_per_channel, float voltage);
void matrixApplyBrightness(Adafruit_NeoMatrix &matrix, float brightness);
uint32_t color565To888(uint16_t color565);
uint16_t color888To565(uint32_t color888);
uint16_t interpolateColors565(uint16_t col1, uint16_t col2, float frac); // Interpolates between 2 16 bits colors

#endif