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
void fadeToBlack(Adafruit_NeoMatrix &matrix, float fade_factor);
void drawLine(Adafruit_NeoMatrix &matrix, float x1, float y1, float x2, float y2, uint16_t hue, uint8_t sat, uint8_t val, bool grad, uint8_t resolution);

class GaussianBlur {
  private:
    static const int kernel_h = 3;
    static const int kernel_w = 3;
    uint kernel[kernel_h][kernel_w] = {0, 0, 0, 0 ,0, 0, 0, 0, 0};
    uint kernel_sum = 0;
  public:
    GaussianBlur(float sigma);
    uint k(int i, int j) {return this->kernel[i][j];}
    uint norm() {return this->kernel_sum;}
    void blur(Adafruit_NeoMatrix &matrix);
};

#endif
