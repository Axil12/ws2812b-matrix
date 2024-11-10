#include <math.h>
#include <algorithm>
#include <Adafruit_NeoMatrix.h>
#include <stdlib.h>
#include "ws2812_program.h"
#include "utils.h"
#include "simplex_noise.h"

void SpectralProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  matrix.fillScreen(
    ColorHSV(
      uint16_t(fmod(time * this->speed, 1.0) * 65536),
      255,
      255
      )
    );
}

void RainbowWaveProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  for (int x = 0; x < matrix.width(); x++) {
    float val = this->n_waves * (fmod(time * this->speed, 1.0) + x / float(matrix.width()));
    for (int y = 0; y < matrix.height(); y++) {
      matrix.drawPixel(x, y, ColorHSV(uint16_t(val * 65536), 255, 255));
    }
  }
}

void RainbowPlasmaProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  float hue;
  float hue_shift = time * this->speed * 0.05f + 1;
  hue_shift += SimplexNoise::noise(time * this->speed * 0.2f);
  hue_shift = fmod(hue_shift, 1.0);
  for (int x = 0; x < matrix.width(); x++) {
    for (int y = 0; y < matrix.height(); y++) {
      hue = (SimplexNoise::noise(x/this->scale, y/this->scale + time * this->speed, time * this->speed) + 1.0) / 2.0;
      hue = fmod(hue + hue_shift, 1.0); // Simplex noise is centered on 0, so we can do this to have a continuously changing mean value
      matrix.drawPixel(x, y, ColorHSV(uint16_t(hue * 65536), 255, 255));
    }
  }
}

void FirePlasmaProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  uint16_t color565;
  float hue;
  for (int x = 0; x < matrix.width(); x++) {
    for (int y = 0; y < matrix.height(); y++) {
      hue = (SimplexNoise::noise(x/this->scale, y/this->scale + time * this->speed, time * this->speed) + 1.0) / 2.0;
      //hue *= hue;
      color565 = this->COLOR_PALETTE_565[(int)round(36 * hue)];
      matrix.drawPixel(x, y, color565);
    }
  }
}

/*
###################################################################################################

Perlin Fire

###################################################################################################
*/
void PerlinFireProgram::createFireSource(){
  for (int x = 0; x < this->heat_map.width(); x++) {
    this->heat_map.value_map[this->heat_map.height()-1] = std::vector<float>(this->heat_map.width(), 1.0);
    }
}

void PerlinFireProgram::applyConvection() {
  
  for (int y = 1; y < this->heat_map.height(); y++) {
    this->heat_map.value_map[y-1] = this->heat_map.value_map[y];
    }
}

void PerlinFireProgram::updateCoolingMap(float offset) {
  for (int i = 1; i < this->cooling_map.height(); i++) {
    this->cooling_map.value_map[i - 1] = this->cooling_map.value_map[i];
    }

  for (int j = 0; j < this->cooling_map.width(); j++) {
    this->cooling_map.value_map[this->cooling_map.height() - 1][j] = (
      SimplexNoise::noise2dOctaves(
        j / this->noise_scale,
        this->cooling_map.height() / this->noise_scale + offset,
        this->octaves,
        0.5f
        ) + 1.0) / 2.0;
    }
}

void PerlinFireProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  float periodicity = 1.0f / this->speed;
  if (this->cycles >= periodicity || this->cycles == 0) {  // Update the heat map
    this->applyConvection();  // Maybe act here to reduce speed
    this->createFireSource();
    this->updateCoolingMap(time * this->speed);

    for (int y = 0; y < this->h; y++){ // First, we store heat map
        for (int x = 0; x < this->w; x++){
          this->heat_map_prev.value_map[y][x] = this->heat_map.value_map[y][x];
        }
      }
    for (int y = 0; y < this->h; y++){ // Then we update it
      for (int x = 0; x < this->w; x++){
        this->heat_map.value_map[y][x] -= this->cooling_map.value_map[y][x] / this->flame_height;
        this->heat_map.value_map[y][x] = max(0, this->heat_map.value_map[y][x]);
      }
    }

    this->cycles = 0;
  }

  float alpha = this->cycles / periodicity;
  float heat_val;
  uint16_t color565;
  for (int y = 0; y < this->h; y++){
    for (int x = 0; x < this->w; x++){
      heat_val = (0.5f * this->heat_map_prev.value_map[y][x] + 0.5f * this->heat_map.value_map[y][x]);
      color565 = this->COLOR_PALETTE_565[(int)round(36 * heat_val)];
      matrix.drawPixel(x, y, color565);
    }
  }

  this->cycles += 1;
}


/*
###################################################################################################

Falling sand

###################################################################################################
*/
void FallingSandProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  if (this->cycles >= this->period){
    for (int y = 0; y < matrix_curr.height(); y++) {  // Updating the value map at t-1 so that it contains the values of the value map at t before we update it
      for (int x = 0; x < matrix_curr.width(); x++) {
        matrix_prev.value_map[y][x] = matrix_curr.value_map[y][x];
      }
    }

    for (int i = 0; i < this->grain_generation_attemps; i++) {  // Deleting random grains on the bottom row
      if ((rand() % 10000) / 100.0f < this->grain_generation_proba * 1.05f) {
        matrix_curr.value_map[matrix_curr.height() - 1][rand() % matrix_curr.width()] = 0;
      }
    }

    for (int y = matrix_curr.height() - 2; y > -1; y--) {
      for (int x = matrix_curr.width() - 1; x > -1; x--) {
        if (this->obstacle.isInHitbox(x, y)) {  // If the considered pixel is in the obstacle's hitbox
          matrix_curr.value_map[y][x] = matrix.Color(255, 255, 255);
        }
        else if (matrix_curr.value_map[y+1][x] == 0) {  // If there's nothing below the considered pixel, move the considered pixel down
          matrix_curr.value_map[y+1][x] = matrix_curr.value_map[y][x];
          matrix_curr.value_map[y][x] = 0;
        }
        else if (x + 1 < matrix_curr.width() && matrix_curr.value_map[y+1][x+1] == 0 && matrix_curr.value_map[y+1][x-1] == 0) {  // if nothing to the right and the left
          matrix_curr.value_map[y+1][x + ((rand() % 2) * 2 - 1)] = matrix_curr.value_map[y][x];
          matrix_curr.value_map[y][x] = 0;
        }
        else if (x + 1 < matrix_curr.width() && matrix_curr.value_map[y+1][x+1] == 0 && matrix_curr.value_map[y][x+1] == 0) {  // if nothing to the right
          matrix_curr.value_map[y+1][x+1] = matrix_curr.value_map[y][x];
          matrix_curr.value_map[y][x] = 0;
        }
        else if (matrix_curr.value_map[y+1][x-1] == 0 && matrix_curr.value_map[y][x-1] == 0) {  // if nothing to the left
          matrix_curr.value_map[y+1][x-1] = matrix_curr.value_map[y][x];
          matrix_curr.value_map[y][x] = 0;
        }
      }
    }

    for (int x = matrix_curr.width() - 1; x > -1; x--) { // Safety to avoid overflow of grains of sand
      uint y = this->obstacle.pos_y + 4;
      if (
        matrix_curr.value_map[y][x] != 0
        && matrix_curr.value_map[y+1][x] != 0
        && matrix_curr.value_map[y+1][x-1] != 0
        && matrix_curr.value_map[y+1][x+1] != 0
        ) {
          //matrix_curr.value_map[y][x] = 0;  // Deleting the grain
          matrix_curr.value_map[matrix_curr.height() - 1][x] = 0;  // Deleting the bottom row's grain
      }
    }

    for (int i = 0; i < this->grain_generation_attemps; i++) {  // Creating new grains
      if ((rand() % 100) < this->grain_generation_proba) {
        uint color = ColorHSV(uint16_t(fmod(time * this->speed * 0.01f, 1.0) * 65536), 255, 255);
        matrix_curr.value_map[0][(matrix_curr.width()/2 - 1) + (rand() % 2)] = color;
      }
    }

    this->cycles = 0;
  }

  // Now that we're done calculating everything, we apply the values to the actual display matrix
  for (int y = 0; y < matrix.height(); y++) {
    for (int x = 0; x < matrix.width(); x++) {
      //matrix.drawPixel(x, y, matrix_curr.value_map[y][x]);
      //matrix.drawPixel(x, y, 0.5*matrix_curr.value_map[y][x] + 0.5*matrix_prev.value_map[y][x]);
      matrix.drawPixel(x, y, interpolateColors565(matrix_prev.value_map[y][x], matrix_curr.value_map[y][x], (float)this->cycles/this->period));
    }
  }

  this->cycles += 1;
}

/*
###################################################################################################

Lava Lamp

###################################################################################################
*/
LavaLampProgram::LavaLampProgram(float speed, uint width, uint height, uint n_balls, float ball_radius) :
      WS2812MatrixProgram(speed), w(width), h(height), n_balls(n_balls) {
  for (int i = 0; i < n_balls; i++) {
    this->balls.push_back(
      LavaLampProgram::Ball(
        ball_radius + (rand() % 101 - 50) / 10.0 * ball_radius,  // provided ball radius, plus/minus a random 10%
        rand() % (width - 3) + 1,
        rand() % (height - 3) + 1,
        speed * cos((rand() % 101) / 100.0 * 2 * 3.14159),
        speed * sin((rand() % 101) / 100.0 * 2 * 3.14159),
        width,
        height
      )
    );
  }
};

void LavaLampProgram::Ball::attractionToOtherBall(Ball otherBall) {
  float distSquared = (this->x - otherBall.x)*(this->x - otherBall.x) + (this->y - otherBall.y)*(this->y - otherBall.y);
  distSquared = max(1, distSquared);  // To avoid balls being considered too closed, and therefore to avoid division by almost 0
  float angle = atan2(otherBall.y - this->y, otherBall.x - this->x);
  this->vy += this->ATTRACTION * sin(angle) / distSquared;
  this->vx += this->ATTRACTION * cos(angle) / distSquared;
}

void LavaLampProgram::Ball::update() {
  if (this->y > this->max_y * 0.80f) {  // if ball is at the bottom, we need to heat it up
    this->heat += this->HEAT_ABSORPTION;
  }
  else {
    this->heat -= this->HEAT_ABSORPTION;
    this->heat = max(0, this->heat);
  }

  this->vy += this->GRAVITY - this->heat;
  this->vy *= 0.995f;  // Drag from the water
  this->vx *= 0.95f;  // Drag from the water

  // Repell the ball from the walls to get a more dynamic scene
  float distToWalls = (this->x - (-2.05f)) * (this->x - (-2.05f)) - (this->max_x*1.05f - this->x)*(this->max_x*1.05f - this->x);
  this->vx -= distToWalls * this->WALL_REPEL;

  if (this->x <= 0 || this->x >= this->max_x) {  // if x positon is out of bounds
    this->x = max(0, min(this->max_x, this->x));  // Place the ball back between bounds
    this->vx *= -1;  // Bounce in the opposite direction
  }
  if (this->y <= 0 || this->y >= this->max_y) {  // if y positon is out of bounds
    this->y = max(0, min(this->max_y, this->y));  // Place the ball back between bounds
    this->vy *= -0.33f;  // Bounce in the opposite direction
  }

  this->x += this->vx;
  this->y += this->vy;
}

void LavaLampProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  for (int i = 0; i < this->n_balls; i++) {
    this->balls[i].update();
  }

  for (int i = 0; i < this->n_balls; i++) {
    for (int j = 0; j < this->n_balls; j++) {
      if (i != j) {
        this->balls[i].attractionToOtherBall(this->balls[j]);
      }
    }
  }
  
  for (int y = 0; y < matrix.height(); y++) {
    for (int x = 0; x < matrix.width(); x++) {
      float intensity = 0.0f;
      for (int i = 0; i < this->n_balls; i++) {
        intensity += this->balls[i].r / ((x - this->balls[i].x)*(x - this->balls[i].x) + (y - this->balls[i].y)*(y - this->balls[i].y));
      }
      intensity = min(255, intensity) / 255.0;
      float range_min = 0.2f;
      float range_max = 0.9f;
      if (range_min < intensity && intensity < range_max) {
        uint idx = (intensity - range_min) / (range_max - range_min) * 8 - 1;  // The constant is the length of the color palette
        matrix.drawPixel(x, y, this->COLOR_PALETTE_565[idx]);
      }
      else if (intensity > range_max) {
        matrix.drawPixel(x, y, this->COLOR_PALETTE_565[7]);
      }
      else {
        matrix.drawPixel(x, y, this->backgroundColor);
      }
    }
  }
}

void RipplesProgram::spawnRandomRipple(uint min_x, uint max_x, uint min_y, uint max_y) {
  uint x = rand() % (max_x + 1 - min_x) + min_x;
  uint y = rand() % (max_y + 1 - min_y) + min_y;
  float radius = 0.01f;
  float amplitude = 1;
  this->ripples.push_back(RipplesProgram::Ring(radius, x, y, amplitude));
}

void RipplesProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  for (int i = 0; i < this->rippleGenerationAttempts; i++) {  // Creating new ripples
    if ((rand() % 101) < this->rippleGenerationProba) {
      this->spawnRandomRipple(1, matrix.width() - 1, 1, matrix.height() - 1);
    }
  }

  for (int y = 0; y < matrix.height(); y++) {
    for (int x = 0; x < matrix.width(); x++) {
      float intensity = 0.0f;
      for (int i = 0; i < this->ripples.size(); i++) {
        float dist = this->distToRipple(x, y, this->ripples[i]);
        intensity += this->ripples[i].amplitude / (dist * dist);
      }
      intensity = min(1.0f, intensity) * 255;
      matrix.drawPixel(x, y, ColorHSV(uint16_t(fmod(time * this->speed, 1.0) * 65536), 255, (uint)intensity));
    }
  }

  for (int i = 0; i < this->ripples.size(); i++) {
    this->ripples[i].expand(.2);
  }
  for (int i = 0; i < this->ripples.size(); i++) {
    if (this->ripples[i].amplitude < 0.01f) {
      this->ripples.erase(this->ripples.begin() + i);
      --i;
    }
  }
}


/*
###################################################################################################

Matrix Effect

###################################################################################################
*/
void MatrixEffectProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  uint8_t r, g, b;
  uint16_t col;
  for (int y = matrix_curr.height() - 1; y > -1; y--) {
      for (int x = 0; x < matrix_curr.width(); x++) {
        col = matrix_curr.value_map[x][y];
        if (col) {
          matrix_curr.value_map[x][y+1] = col;
          r = (col & 0xff0000) >> 16;
          g = (col & 0x00ff00) >> 8;
          b = col & 0xff0000;
          
          r = round(r * this->dimming_factor);
          g = round(g * this->dimming_factor);
          b = round(b * this->dimming_factor);

          matrix_curr.value_map[x][y] = (r << 16 | g << 8 | b);  
        }
      }
  }

  for (int i = 0; i < this->drop_generation_attemps; i++) {  // Creating new drops
      if ((rand() % 100) < this->drop_generation_proba) {
        matrix_curr.value_map[(rand() % matrix_curr.width())][0] = this->color;
      }
    }

  for (int y = matrix.height() - 1; y > -1; y--) {  // Writing to the matrix
      for (int x = 0; x < matrix.width(); x++) {
        matrix.drawPixel(x, y, color888To565(matrix_curr.value_map[x][y]));
      }
  }
}


/*
###################################################################################################

Vortex Effect

###################################################################################################
*/
void VortexProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  fadeToBlack(matrix, 0.6f);

  float t = time * this->speed;
  uint min_x, max_x, min_y, max_y, x, y;
  uint max_i = 3;
  uint max_j = 3;
  for (int i = 0; i < max_i; i++) {
    for (int j = 0; j < max_j; j++) {
      min_x = (j*2) + 1;
      max_x = matrix.width() - 1 - (j*2);
      x = (sin(t + ((j % 2) ? 128 : 0) + t * (i+j)) * 0.5f + 0.5f) * (max_x - min_x) + min_x;
      min_y = (j*2) + 1;
      max_y = matrix.height() - 1 - (j*2);
      y = (sin(1.1f * t + ((j % 2) ? 192 : 64) + t * (i+j)) * 0.5f + 0.5f) * (max_y - min_y) + min_y;
      matrix.drawPixel(x, y, ColorHSV((fmod(t / 10.0f, 1.0f) + (i + j)/(float)(max_i + max_j)) * 65536, 255, 255));
    }
  }
  this->gaussian_blur.blur(matrix);
}


void RotatingKaleidoscopeProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  uint8_t dim = matrix.width()/2 + 2;
  float x, y;
  uint16_t hue;
  uint8_t cx = matrix.width()/2;
  uint8_t cy = matrix.height()/2;
  for (float i = 1; i < dim; i += 0.25) {
    float angle = this->speed * time * (dim - i);
    x = (matrix.width()/2.0f) + sin(angle) * i;
    y = (matrix.height()/2.0f) + cos(angle) * i;
    hue = (uint16_t)(((x-cx)*(x-cx) + (y-cy)*(y-cy)) * 500 + 100*time * this->speed * 10) % 65535;
    matrix.drawPixel((uint8_t)x, (uint8_t)y, ColorHSV(hue, 255, 255));
  }
  this->gaussian_blur.blur(matrix);
}


OctopusProgram::OctopusProgram(float speed, int height, int width) : WS2812MatrixProgram(speed) {
  const uint8_t C_X = width / 2;
  const uint8_t C_Y = height / 2;
  const uint8_t MAPP = 255 / max(height, width);
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      this->r_map_angle[x][y] = atan2(y-C_Y, x-C_X);
      this->r_map_radius[x][y] = pow((x-C_X)*(x-C_X) + (y-C_Y)*(y-C_Y), 0.5f); //thanks Sutaburosu
    }
  }
}

void OctopusProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  float angle, radius, arms;
  for (int x = 0; x < matrix.width(); x++) {
    for (int y = 0; y < matrix.height(); y++) {
      angle = this->r_map_angle[x][y];
      radius = this->r_map_radius[x][y];
      arms = 0.5 * (sin(0.01f*time*this->speed) + 1) * (this->arms_max - this->arms_min) + this->arms_min;
      matrix.drawPixel(
        x,
        y,
        ColorHSV(
          (uint16_t)fmod(3000*radius + 1000*time*this->speed, 65536),
          255,
          (uint8_t)(127*(sin((sin((angle * 4 - radius) / 4 + time*this->speed) + 1) + 0.5 * radius - time*this->speed + angle * arms) + 1))
        )
      );
    }
  }
}


void BurstsProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  fadeToBlack(matrix, 0.25f);

  uint16_t color;
  uint16_t hue;
  float x1, y1, x2, y2;
  float xsteps, ysteps, steps;
  float rate;
  float dx, dy;
  uint16_t rand_n = rand() % 200;
  if (rand_n == 0) {
    this->num_lines++;
    this->num_lines = min(this->num_lines, this->max_lines);
  } else if (rand_n == 1) {
    this->num_lines--;
    this->num_lines = max(this->num_lines, this->min_lines);
  }

  for (int i = 0; i < this->num_lines; i++) {
    x1 = 0.5f * (sin(12 + 1.0*time * this->speed) + 1) * matrix.width();
    x2 = 0.5f * (sin(10 + 1.1*time * this->speed) + 1) * matrix.width();
    y1 = 0.5f * (sin(25 + 1.2*time * this->speed + i * 24) + 1) * matrix.height();
    y2 = 0.5f * (sin(20 + 1.3*time * this->speed + i * 48 + 64) + 1) * matrix.height();

    hue = (uint16_t)(fmod(1.0f * i / this->num_lines + 0.1f * time * this->speed, 1.0f) * 65535);
    drawLine(matrix, x1, x2, y1, y2, hue, 255, 255, true, 0);
    matrix.drawPixel(y1, y2, ColorHSV(0, 0, 255));  // Drawing a white dot at the tip of each line
  }
  this->gaussian_blur.blur(matrix);
}


void LissajousProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  //matrix.fill(0);
  uint16_t color;
  float phase = time * this->speed;
  float xlocn, ylocn;
  for (int i = 0; i < 256; i++) {
    xlocn = sin(phase/2 + 1 * i);
    ylocn = sin(phase/2 + 2 * i);

    xlocn = (xlocn + 1) * matrix.width() / 2.0f;
    ylocn = (ylocn + 1) * matrix.height() / 2.0f;

    color = ColorHSV((uint16_t)(100 * time * this->speed + i * 10), 255, 255);
    color = ColorHSV((uint16_t)(100 * time * this->speed), 255, 255);
    matrix.drawPixel((uint8_t)ylocn, (uint8_t)xlocn, color);
  }
}


void DnaSpiralProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  matrix.fill(0);
  uint16_t color, hue;
  uint8_t x1, x2;
  const uint8_t freq = 6;
  float steps, rate, dx;
  for (int i = 0; i < matrix.height(); i++) {
    x1 = (matrix.width() / 2) * 0.5f * ((sin(time * this->speed + i * freq) + 1) + sin(0.37f * time * this->speed + i * freq + 128) + 1);
    x2 = (matrix.width() / 2) * 0.5f * ((sin(time * this->speed + i * freq + 128) + 1) + sin(0.37f * time * this->speed + i * freq + 128 + 64) + 1);

    hue = (uint16_t)(-i * 2048 + 4096 * time * this->speed);
    drawLine(matrix, x1, i, x2, i, hue, 255, 255, true, abs(x2 - x1) + 1);

    matrix.drawPixel(x1, i, 0x8430);
    matrix.drawPixel(x2, i, 0xffff);
  }
}

/*
###################################################################################################

TetrahedronProgram

###################################################################################################
*/
void TetrahedronProgram::Point::rotateX(float x, float y, float z, float a) {
  this->x = x;
  this->y = y * cos(a) - z * sin(a);
  this->z = y * sin(a) + z * cos(a);
}
void TetrahedronProgram::Point::rotateY(float x, float y, float z, float a) {
  this->x = x * cos(a) + z * sin(a);
  this->y = y;
  this->z = x * -sin(a) + z * cos(a);
}
void TetrahedronProgram::Point::rotateZ(float x, float y, float z, float a) {
  this->x = x * cos(a) - y * sin(a);
  this->y = x * sin(a) + y * cos(a);
  this->z = z;
}

uint16_t TetrahedronProgram::getEdgeHue(TetrahedronProgram::Point const & a, TetrahedronProgram::Point const & b) {
  if ((a.id == 0 && b.id == 1) || (a.id == 1 && b.id == 0)) {return 0;}
  if ((a.id == 0 && b.id == 2) || (a.id == 2 && b.id == 0)) {return 5000;}
  if ((a.id == 0 && b.id == 3) || (a.id == 3 && b.id == 0)) {return 10000;}
  if ((a.id == 1 && b.id == 2) || (a.id == 2 && b.id == 1)) {return 15000;}
  if ((a.id == 1 && b.id == 3) || (a.id == 3 && b.id == 1)) {return 20000;}
  if ((a.id == 2 && b.id == 3) || (a.id == 3 && b.id == 2)) {return 25000;}
  return 0;
}

float TetrahedronProgram::sign(TetrahedronProgram::Point const & p1, TetrahedronProgram::Point const & p2, TetrahedronProgram::Point const & p3) {
  return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool TetrahedronProgram::pointIsInTriangle(TetrahedronProgram::Point const & p, TetrahedronProgram::Point const & v1, TetrahedronProgram::Point const & v2, TetrahedronProgram::Point const & v3) {
  float d1, d2, d3;
  bool has_neg, has_pos;
  d1 = this->sign(p, v1, v2);
  d2 = this->sign(p, v2, v3);
  d3 = this->sign(p, v3, v1);
  has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
  has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
  return !(has_neg && has_pos);
}

bool TetrahedronProgram::isInBeetween(TetrahedronProgram::Point const & p, TetrahedronProgram::Point const & v1, TetrahedronProgram::Point const & v2, float epsilon) {
  float dist1 = ((float)p.x - (float)v1.x)*((float)p.x - (float)v1.x) + ((float)p.y - (float)v1.y)*((float)p.y - (float)v1.y);
  float dist2 = ((float)p.x - (float)v2.x)*((float)p.x - (float)v2.x) + ((float)p.y - (float)v2.y)*((float)p.y - (float)v2.y);
  return (abs(dist1 - dist2) < epsilon);
}

void TetrahedronProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  float z, projected_x, projected_y;
  float angle_x = 3.14159 * (0.5f * SimplexNoise::noise(time * this->speed * 0.20f) + 0) + 1.00 * time * this->speed;
  float angle_y = 3.14159 * (0.5f * SimplexNoise::noise(time * this->speed * 0.25f) + 0) + 1.05 * time * this->speed;
  float angle_z = 3.14159 * (0.5f * SimplexNoise::noise(time * this->speed * 0.30f) + 0) + 1.10 * time * this->speed;
  for (uint8_t i = 0; i < 4; i++) {
    this->points[i].rotateX(this->points[i].x0, this->points[i].y0, this->points[i].z0, angle_x);
    this->points[i].rotateY(this->points[i].x, this->points[i].y, this->points[i].z, angle_y);
    this->points[i].rotateZ(this->points[i].x, this->points[i].y, this->points[i].z, angle_z);

    z = 1 / (this->camera_distance - this->points[i].z);
    projected_x = z * this->points[i].x;
    projected_y = z * this->points[i].y;

    // Scaling up the tetrahedron to fit the matrix. Because up to this point, we've worked with a tetrahedron centered on (0, 0, 0) scaled to the unit sphere.
    projected_x *= matrix.width() * this->tetrahedron_scale; 
    projected_y *= matrix.height() * this->tetrahedron_scale;

    // Moving the tetrahedron's center to the center of the matrix.
    projected_x += matrix.width() / 2 - 0.5f;
    projected_y += matrix.height() / 2 - 0.5f;

    // Now we store the projected coordinates into the Points' coordinates, just so we can reuse those for the drawing functions down below.
    this->points[i].x = (uint8_t)round(projected_x);
    this->points[i].y = (uint8_t)round(projected_y);
  }

  matrix.fill(0);

  // We sort points based on their distance so that we can draw lines back to front to avoid a line in the back being drawn on top of line in the front, which looks inconsistent.
  std::sort(
    this->points,
    this->points + 4,
    [](TetrahedronProgram::Point const & a, TetrahedronProgram::Point const & b) -> bool {return a.z < b.z;}
    );

  // Drawing lines between the points
  uint16_t hue = uint16_t(fmod(0.025 * time * this->speed, 1.0) * 65536);
  drawLine(matrix, this->points[3].x, this->points[3].y, this->points[2].x, this->points[2].y, hue + this->getEdgeHue(this->points[3], this->points[2]), 255, 128, false, 0);
  drawLine(matrix, this->points[3].x, this->points[3].y, this->points[1].x, this->points[1].y, hue + this->getEdgeHue(this->points[3], this->points[1]), 255, 128, false, 0);
  drawLine(matrix, this->points[3].x, this->points[3].y, this->points[0].x, this->points[0].y, hue + this->getEdgeHue(this->points[3], this->points[0]), 255, 128, false, 0);
  drawLine(matrix, this->points[2].x, this->points[2].y, this->points[1].x, this->points[1].y, hue + this->getEdgeHue(this->points[2], this->points[1]), 255, 128, false, 0);
  drawLine(matrix, this->points[2].x, this->points[2].y, this->points[0].x, this->points[0].y, hue + this->getEdgeHue(this->points[2], this->points[0]), 255, 128, false, 0);
  drawLine(matrix, this->points[1].x, this->points[1].y, this->points[0].x, this->points[0].y, hue + this->getEdgeHue(this->points[1], this->points[0]), 255, 128, false, 0);

  this->gaussian_blur.blur(matrix);

  // Drawing the closest 3 corners
  for (uint8_t i = 0; i < 3; i++)
    matrix.drawPixel(this->points[i].x, this->points[i].y, 0xffff);
  // Drawing the furthest corner, but only if it is not inside the triangle formed by the closest 3 corners.  This simulates the corner being obstructed by the drawn edges, reinforcing the impression of 3D.
  if (!this->pointIsInTriangle(this->points[3], this->points[0], this->points[1], this->points[2]))
    matrix.drawPixel(this->points[3].x, this->points[3].y, 0xffff);
}

/*
###################################################################################################

StretchyTetrahedronProgram

###################################################################################################
*/
void StretchyTetrahedronProgram::iterate(Adafruit_NeoMatrix &matrix, float time) {
  matrix.fill(0);

  uint8_t x1 = (uint8_t)(0.5f * (sin(18 + 1.0*time * this->speed) + 1) * matrix.width());
  uint8_t x2 = (uint8_t)(0.5f * (sin(23 + 1.1*time * this->speed) + 1) * matrix.width());
  uint8_t x3 = (uint8_t)(0.5f * (sin(27 + 1.2*time * this->speed) + 1) * matrix.width());
  uint8_t x4 = (uint8_t)(0.5f * (sin(31 + 1.3*time * this->speed) + 1) * matrix.width());

  uint8_t y1 = (uint8_t)(0.5f * (sin(20 + 1.00*time * this->speed) + 1) * matrix.height());
  uint8_t y2 = (uint8_t)(0.5f * (sin(26 + 1.05*time * this->speed) + 1) * matrix.height());
  uint8_t y3 = (uint8_t)(0.5f * (sin(15 + 1.10*time * this->speed) + 1) * matrix.height());
  uint8_t y4 = (uint8_t)(0.5f * (sin(27 + 1.15*time * this->speed) + 1) * matrix.height());

  uint16_t hue = uint16_t(fmod(0.025 * time * this->speed, 1.0) * 65536);

  drawLine(matrix, x1, y1, x2, y2, hue, 255, 255, false, 0);
  drawLine(matrix, x1, y1, x3, y3, hue, 255, 255, false, 0);
  drawLine(matrix, x1, y1, x4, y4, hue, 255, 255, false, 0);
  drawLine(matrix, x2, y2, x3, y3, hue, 255, 255, false, 0);
  drawLine(matrix, x2, y2, x4, y4, hue, 255, 255, false, 0);
  drawLine(matrix, x3, y3, x4, y4, hue, 255, 255, false, 0);

  this->gaussian_blur.blur(matrix);

  matrix.drawPixel(x1, y1, 0xffff);
  matrix.drawPixel(x2, y2, 0xffff);
  matrix.drawPixel(x3, y3, 0xffff);
  matrix.drawPixel(x4, y4, 0xffff);
}
