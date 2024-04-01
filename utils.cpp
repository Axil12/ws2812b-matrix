#include "utils.h"
#include <math.h>
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

void fadeToBlack(Adafruit_NeoMatrix &matrix, float fade_factor) {
  uint32_t color;
  uint8_t r, g, b;
  for (int i = 0; i < matrix.width() * matrix.height(); i++) {  // Fade to black (to create trails)
    color = matrix.getPixelColor(i);
    r = ((color >> 16) & 0x0000ff);
    g = ((color >> 8) & 0x0000ff);
    b = color & 0x0000ff;
    r = round(r * fade_factor);
    g = round(g * fade_factor);
    b = round(b * fade_factor);
    r = r+g+b > 4 ? r : 0;
    g = r+g+b > 4 ? g : 0;
    b = r+g+b > 4 ? b : 0;
    matrix.setPixelColor(i, r, g, b);
  }
}

GaussianBlur::GaussianBlur(float sigma) {
  double min_weight = 10000.0f;
  float kernel[this->kernel_h][this->kernel_w] = {0, 0, 0, 0 ,0, 0, 0, 0, 0};
  for (int i = 0; i < this->kernel_h; i++) {
    for (int j = 0; j < this->kernel_w; j++) {
      kernel[i][j] = exp(-((i - 1)*(i - 1)+(j - 1)*(j - 1))/(2*sigma*sigma))/(2*3.14159*sigma*sigma);
      if (kernel[i][j] < min_weight)
        min_weight = kernel[i][j];
    }
  }

  for (int i = 0; i < this->kernel_h; i++) {
    for (int j = 0; j < this->kernel_w; j++) {
      this->kernel[i][j] = (uint)(kernel[i][j] / min_weight + 0.01f); // Adding 0.1 to make sure that the truncation is robust to floating point errors
      this->kernel_sum += this->kernel[i][j];
    }
  }
}

void GaussianBlur::blur(Adafruit_NeoMatrix &matrix) {
  uint32_t col_1, col_2, col_3, col_4, col_5, col_6, col_7, col_8, col_9;
  uint8_t r1, r2, r3, r4, r5, r6, r7, r8, r9;
  uint8_t g1, g2, g3, g4, g5, g6, g7, g8, g9;
  uint8_t b1, b2, b3, b4, b5, b6, b7, b8, b9;
  uint32_t color;
  uint8_t r, g, b;
  uint8_t w = matrix.width();
  uint8_t h = matrix.height();
  uint32_t colors[w*h]; memset(colors, 0, sizeof(colors));
  uint16_t idx, idx_neigh_top, idx_neigh_bot;
  for (int y = 0; y < matrix.height(); y++) {
    for (int x = 0; x < matrix.width(); x++) {
      if (y%2 == 0) {  // Weird indexing, but that's because the WS2812b 16x16 matrix is arranged in zigzag
        idx = y*h + x;
        idx_neigh_top = (y-1)*h + (w - 1 - x);
        idx_neigh_bot = (y+1)*h + (w - 1 - x);
      }
      else {
        idx = y*h + (w - 1 - x);
        idx_neigh_top = (y-1)*h + x;
        idx_neigh_bot = (y+1)*h + x;
      }

      // Apparently I don't need to accound for index errors to to the edges. Not sure why. Maybe the NeoMatrix library properly safeguards ?
      col_1 = matrix.getPixelColor(idx_neigh_top - 1); r1 = (col_1 & 0xff0000) >> 16; g1 = (col_1 & 0x00ff00) >> 8; b1 = col_1 & 0x0000ff;
      col_2 = matrix.getPixelColor(idx_neigh_top);     r2 = (col_2 & 0xff0000) >> 16; g2 = (col_2 & 0x00ff00) >> 8; b2 = col_2 & 0x0000ff;
      col_3 = matrix.getPixelColor(idx_neigh_top + 1); r3 = (col_3 & 0xff0000) >> 16; g3 = (col_3 & 0x00ff00) >> 8; b3 = col_3 & 0x0000ff;
      col_4 = matrix.getPixelColor(idx - 1);     r4 = (col_4 & 0xff0000) >> 16; g4 = (col_4 & 0x00ff00) >> 8; b4 = col_4 & 0x0000ff;
      col_5 = matrix.getPixelColor(idx);         r5 = (col_5 & 0xff0000) >> 16; g5 = (col_5 & 0x00ff00) >> 8; b5 = col_5 & 0x0000ff;
      col_6 = matrix.getPixelColor(idx + 1);     r6 = (col_6 & 0xff0000) >> 16; g6 = (col_6 & 0x00ff00) >> 8; b6 = col_6 & 0x0000ff;
      col_7 = matrix.getPixelColor(idx_neigh_bot - 1); r7 = (col_7 & 0xff0000) >> 16; g7 = (col_7 & 0x00ff00) >> 8; b7 = col_7 & 0x0000ff;
      col_8 = matrix.getPixelColor(idx_neigh_bot);     r8 = (col_8 & 0xff0000) >> 16; g8 = (col_8 & 0x00ff00) >> 8; b8 = col_8 & 0x0000ff;
      col_9 = matrix.getPixelColor(idx_neigh_bot + 1); r9 = (col_9 & 0xff0000) >> 16; g9 = (col_9 & 0x00ff00) >> 8; b9 = col_9 & 0x0000ff;

      r = (
        this->k(0, 0)*r1 + this->k(0, 1)*r2 + this->k(0, 2)*r3
        + this->k(1, 0)*r4 + this->k(1, 1)*r5 + this->k(1, 2)*r6
        + this->k(2, 0)*r7 + this->k(2, 1)*r8 + this->k(2, 2)*r9
        ) / this->kernel_sum;
      g = (
        this->k(0, 0)*g1 + this->k(0, 1)*g2 + this->k(0, 2)*g3
        + this->k(1, 0)*g4 + this->k(1, 1)*g5 + this->k(1, 2)*g6
        + this->k(2, 0)*g7 + this->k(2, 1)*g8 + this->k(2, 2)*g9
        ) / this->kernel_sum;
      b = (
        this->k(0, 0)*b1 + this->k(0, 1)*b2 + this->k(0, 2)*b3
        + this->k(1, 0)*b4 + this->k(1, 1)*b5 + this->k(1, 2)*b6
        + this->k(2, 0)*b7 + this->k(2, 1)*b8 + this->k(2, 2)*b9
        ) / this->kernel_sum;
      color = (r << 16 | g << 8 | b);
      colors[idx] = color;
    }
  }

  for (int y = 0; y < matrix.height(); y++) {
    for (int x = 0; x < matrix.width(); x++) {
      if (y%2 == 0)
        idx = y*h + x;
      else
        idx = y*h + (w - 1 - x);
      matrix.setPixelColor(idx, colors[idx]);
    }
  }
}

void drawLine(Adafruit_NeoMatrix &matrix, float x1, float y1, float x2, float y2, uint16_t hue, uint8_t sat, uint8_t val, bool grad, uint8_t resolution) {
  float dx, dy, rate;
  uint8_t value;
  uint16_t color;
  float steps;

  if (resolution == 0) {
    float xsteps = abs(x2 - x1) + 1;
    float ysteps = abs(y2 - y1) + 1;
    steps = max(xsteps, ysteps);
  } else {
    steps = resolution;
  }

  for (int j = 1; j <= steps; j++) {
    rate = j / steps;
    dx = x1 + rate * (x2 - x1);
    dy = y1 + rate * (y2 - y1);
    value = grad ? (uint8_t)(val*rate) : val;
    color = ColorHSV(hue, sat, value);
    matrix.drawPixel((uint8_t)dx, (uint8_t)dy, color);
  }
}
