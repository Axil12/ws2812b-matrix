#include <math.h>
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
  float fade_factor = 0.75f;
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

  float t = time * this->speed;
  uint min_x, max_x, min_y, max_y, x, y;
  uint max_i = 2;
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
}