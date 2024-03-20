#include "utils.h"
#include <Adafruit_NeoMatrix.h>

uint16_t ColorHSV(uint16_t hue, uint8_t sat, uint8_t val) {
    uint8_t r, g, b, r2, g2, b2;

    // Remap 0-65535 to 0-1529. Pure red is CENTERED on the 64K rollover;
    // 0 is not the start of pure red, but the midpoint...a few values above
    // zero and a few below 65536 all yield pure red (similarly, 32768 is the
    // midpoint, not start, of pure cyan). The 8-bit RGB hexcone (256 values
    // each for red, green, blue) really only allows for 1530 distinct hues
    // (not 1536, more on that below), but the full unsigned 16-bit type was
    // chosen for hue so that one's code can easily handle a contiguous color
    // wheel by allowing hue to roll over in either direction.
    hue = (hue * 1530L + 32768) / 65536;

    // Convert hue to R,G,B (nested ifs faster than divide+mod+switch):
    if(hue < 510) {         // Red to Green-1
      b = 0;
      if(hue < 255) {       //   Red to Yellow-1
        r = 255;
        g = hue;            //     g = 0 to 254
      } else {              //   Yellow to Green-1
        r = 510 - hue;      //     r = 255 to 1
        g = 255;
      }
    } else if(hue < 1020) { // Green to Blue-1
      r = 0;
      if(hue <  765) {      //   Green to Cyan-1
        g = 255;
        b = hue - 510;      //     b = 0 to 254
      } else {              //   Cyan to Blue-1
        g = 1020 - hue;     //     g = 255 to 1
        b = 255;
      }
    } else if(hue < 1530) { // Blue to Red-1
      g = 0;
      if(hue < 1275) {      //   Blue to Magenta-1
        r = hue - 1020;     //     r = 0 to 254
        b = 255;
      } else {              //   Magenta to Red-1
        r = 255;
        b = 1530 - hue;     //     b = 255 to 1
      }
    } else {                // Last 0.5 Red (quicker than % operator)
      r = 255;
      g = b = 0;
    }

    // Apply saturation and value to R,G,B
    uint32_t v1 =   1 + val; // 1 to 256; allows >>8 instead of /255
    uint16_t s1 =   1 + sat; // 1 to 256; same reason
    uint8_t  s2 = 255 - sat; // 255 to 0

    r2 = ((((r * s1) >> 8) + s2) * v1) >> 8;
    g2 = ((((g * s1) >> 8) + s2) * v1) >> 8;
    b2 = ((((b * s1) >> 8) + s2) * v1) >> 8;
    return ((uint16_t)(r2 & 0xF8) << 8) | ((uint16_t)(g2 & 0xFC) << 3) | (b2 >> 3);
}

float matrixCurrentDraw(Adafruit_NeoMatrix &matrix, float current_per_channel) {  // fairly sure this is correctly implemented
  int numLEDs = matrix.numPixels();
  int sum = 0;
  uint32_t color;
  for (int n = 0; n < numLEDs; n++) {
    color = matrix.getPixelColor(n);
    sum += (color & 0x0000ff) + ((color >> 8) & 0x0000ff) + ((color >> 16) & 0x0000ff); 
  }
  return (sum / 255.0) * current_per_channel;
}

float matrixPowerDraw(Adafruit_NeoMatrix &matrix, float current_per_channel, float voltage) {
  return matrixCurrentDraw(matrix, current_per_channel) * voltage;
}

void matrixApplyBrightness(Adafruit_NeoMatrix &matrix, float brightness) {
  int numLEDs = matrix.numPixels();
  uint32_t color;
  uint8_t r, g, b;
  for (int n = 0; n < numLEDs; n++) {
    color = matrix.getPixelColor(n);
    r = ((color >> 16) & 0x0000ff);
    g = ((color >> 8) & 0x0000ff);
    b = (color & 0x0000ff);
    matrix.setPixelColor(n, (int)(r*brightness), (int)(g*brightness), (int)(b*brightness));
  }
}

uint32_t color565To888(uint16_t color565) {
  uint8_t r = ((color565 >> 11) & 0x1F);
  uint8_t g = ((color565 >> 5) & 0x3F);
  uint8_t b = (color565 & 0x1F);

  r = ((((color565 >> 11) & 0x1F) * 527) + 23) >> 6;
  g = ((((color565 >> 5) & 0x3F) * 259) + 33) >> 6;
  b = (((color565 & 0x1F) * 527) + 23) >> 6;

  return (r << 16 | g << 8 | b);
}

uint16_t color888To565(uint32_t color888) {
    return ((color888 & 0xf80000) >> 8) | ((color888 & 0xfc00) >> 5) | ((color888 & 0xf8) >> 3);
}

uint16_t interpolateColors565(uint16_t col1, uint16_t col2, float frac) {
  uint32_t col_1 = color565To888(col1);
  uint32_t col_2 = color565To888(col2);
  uint8_t r1 = (col_1 & 0xff0000) >> 16;
  uint8_t r2 = (col_2 & 0xff0000) >> 16;
  uint8_t g1 = (col_1 & 0x00ff00) >> 8;
  uint8_t g2 = (col_2 & 0x00ff00) >> 8;
  uint8_t b1 = col_1 & 0x0000ff;
  uint8_t b2 = col_2 & 0x0000ff;

  return (
    Adafruit_NeoMatrix::Color(
      (uint8_t)((r2 - r1)*frac + r1),
      (uint8_t)((g2 - g1)*frac + g1),
      (uint8_t)((b2 - b1)*frac + b1)
    )  // Converts back to RGB565
  );
}