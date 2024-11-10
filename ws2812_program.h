#ifndef WS2812_PROGRAM_H
#define WS2812_PROGRAM_H
#include <Arduino.h>
#include <Adafruit_NeoMatrix.h>
#include <vector>
#include <tuple>
#include <math.h>
#include "utils.h"
#include "simplex_noise.h"


class WS2812MatrixProgram {
  public:
    float speed;

    WS2812MatrixProgram(float speed){this->speed = speed;};
    virtual void iterate(Adafruit_NeoMatrix &matrix, float time);
};

class StaticProgram: public WS2812MatrixProgram {
  private:
    uint16_t color;
  public:
    StaticProgram(float speed, uint16_t color) : WS2812MatrixProgram(speed), color(color) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time) {matrix.fillScreen(this->color);}
};

class SpectralProgram: public WS2812MatrixProgram {
  public:
    SpectralProgram(float speed) : WS2812MatrixProgram(speed) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};

class RainbowWaveProgram: public WS2812MatrixProgram {
  public:
    int n_waves = 1;

    RainbowWaveProgram(float speed, int n_waves) : WS2812MatrixProgram(speed), n_waves(n_waves) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};

class RainbowPlasmaProgram: public WS2812MatrixProgram {
  public:
    float scale;

    RainbowPlasmaProgram(float speed, float scale) : WS2812MatrixProgram(speed), scale(scale) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};

class FirePlasmaProgram: public WS2812MatrixProgram {  // very similar to RainbowPlasmaProgram, but with a fiery color palette
  private:
    const uint16_t COLOR_PALETTE_565 [37] = {
      0x0000, 0x1820, 0x2860, 0x4060, 0x50A0, 0x60E0, 0x70E0,
      0x8920, 0x9960, 0xA9E0, 0xBA20, 0xC220, 0xDA60, 0xDAA0,
      0xDAA0, 0xD2E0, 0xD2E0, 0xD321, 0xCB61, 0xCBA1, 0xCBE1,
      0xCC22, 0xC422, 0xC462, 0xC4A3, 0xBCE3, 0xBCE3, 0xBD24,
      0xBD24, 0xBD65, 0xB565, 0xB5A5, 0xB5A6, 0xCE6D, 0xDEF3,
      0xEF78, 0xFFFF,
    };
    
  public:
    float scale;

    FirePlasmaProgram(float speed, float scale) : WS2812MatrixProgram(speed), scale(scale) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};

class PerlinFireProgram: public WS2812MatrixProgram {
  private:
    const uint16_t COLOR_PALETTE_565 [37] = {
      0x0000, 0x1820, 0x2860, 0x4060, 0x50A0, 0x60E0, 0x70E0,
      0x8920, 0x9960, 0xA9E0, 0xBA20, 0xC220, 0xDA60, 0xDAA0,
      0xDAA0, 0xD2E0, 0xD2E0, 0xD321, 0xCB61, 0xCBA1, 0xCBE1,
      0xCC22, 0xC422, 0xC462, 0xC4A3, 0xBCE3, 0xBCE3, 0xBD24,
      0xBD24, 0xBD65, 0xB565, 0xB5A5, 0xB5A6, 0xCE6D, 0xDEF3,
      0xEF78, 0xFFFF,
    };

    class ValueMap {
      private:
          const int w;
          const int h;

      public:
          ValueMap(int width, int height) : w(width), h(height) {value_map.resize(h, std::vector<float>(w, 0));}
          int width() {return w;};
          int height() {return h;};
          std::vector<std::vector<float>> value_map;
      };

    const int w, h;
    float noise_scale;
    float flame_height;
    int octaves;
    uint cycles = 0;  // number of times iterate has been called
    ValueMap cooling_map;
    ValueMap heat_map;  // current heat map
    ValueMap heat_map_prev;  // previous heat map
    
    void createFireSource();
    void updateCoolingMap(float offset);
    void applyConvection();

  public: 
    PerlinFireProgram(float speed, float scale, int width, int height, float flame_height, int octaves) : 
      WS2812MatrixProgram(speed),
      noise_scale(scale),
      w(width),
      h(height),
      flame_height(flame_height),
      octaves(octaves),
      heat_map(width, height),
      cooling_map(width, height),
      heat_map_prev(width, height)
      {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};


class FallingSandProgram: public WS2812MatrixProgram {
  private:
    class Obstacle {
      private:
        uint8_t sprite[4][4] = {  
          {0, 1, 1, 0},   /*  initializers for row indexed by 0 */
          {1, 1, 1, 1},   /*  initializers for row indexed by 1 */
          {1, 1, 1, 1},   /*  initializers for row indexed by 2 */
          {0, 1, 1, 0}   /*  initializers for row indexed by 3 */
        };
      public:
        uint pos_x, pos_y;
        Obstacle(uint pos_x, uint pos_y) : pos_x(pos_x), pos_y(pos_y) {};
        bool isInHitbox(uint x, uint y) {  // Check if (x, y) is in the obstacle's sprite
          return (
            (this->pos_y <= y) && (y <= this->pos_y + 4 - 1) &&  // y is within the height of the sprite
            (this->pos_x <= x) && (x <= this->pos_x + 4 - 1) &&  // x is within the width of the sprite
            (this->sprite[y - this->pos_y][x - this->pos_x])  // (x, y) is falling onto a "1" in the sprite
          );
        };
    };

    class ValueMap {
      private:
          const uint w;
          const uint h;

      public:
          ValueMap(uint width, uint height) : w(width), h(height) {
              value_map.resize(h, std::vector<uint>(w, 0));
              for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                  value_map[y][x] = 0;
                }
              }
            }
          uint width() {return w;};
          uint height() {return h;};
          std::vector<std::vector<uint>> value_map;
      };

    const uint grain_generation_attemps = 1;
    const uint grain_generation_proba = 10;  // percentage
    Obstacle obstacle = Obstacle(6, 4);
    ValueMap matrix_curr; // We save the visuals of the matrix to do smooth interpolation. Not yet implemented.
    ValueMap matrix_prev; 
    uint cycles, period; // The period is the number of frames before each calculation of the next state of the matrix
  public:
    FallingSandProgram(float speed, uint width, uint height) :
      WS2812MatrixProgram(speed),
      matrix_curr(width, height),
      matrix_prev(width, height),
      cycles(0),
      period((uint)(1/speed)) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};


class LavaLampProgram: public WS2812MatrixProgram {  // Based on a MetaBalls approach
  private:
    class Ball {
      private:
        float heat;
        const float HEAT_ABSORPTION = 5e-6; //0.000005;
        const float GRAVITY = 1e-3;
        const float ATTRACTION = -1e-2;
        const float WALL_REPEL = 5e-6;
      public:
        float r, x, y, vx, vy, max_x, max_y;  // radius, position x and y, velocity x and y, max position x and y
        Ball(float radius, float x, float y, float vx, float vy, float max_x, float max_y) :
          r(radius), x(x), y(y), vx(vx), vy(vy), max_x(max_x), max_y(max_y) {};
        void update();  // Updates the heat, position, velocity, etc...
        void attractionToOtherBall(Ball otherBall);  // Updates the velocity of the ball based on the position with the other ball
    };
  
    const uint w, h; // matrix width/height
    const uint n_balls;
    std::vector<Ball> balls;
    const uint16_t backgroundColor = Adafruit_NeoMatrix::Color(0, 0, 168);
    const uint16_t COLOR_PALETTE_565 [8] = {
      0xC220, 0xD321, 0xCBA1, 0xCC22,
      0xC462, 0xBCE3, 0xBD24, 0xBD65
      };
  public:
    LavaLampProgram(float speed, uint width, uint height, uint n_balls, float ball_radius);
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};


class RipplesProgram: public WS2812MatrixProgram {
  private:
    class Ring {
      public:
        float r, x, y, amplitude, amplitude_init;  // radius, position, amplitude, initial amplitude
        Ring(float radius, float x, float y, float amplitude) : r(radius), x(x), y(y), amplitude(amplitude), amplitude_init(amplitude) {};
        void expand(float length) {this->r += length; this->amplitude = amplitude_init/this->r;}
    };
  float distToRipple(uint x, uint y, Ring ripple) {return abs(sqrt((ripple.x - x)*(ripple.x - x) + (ripple.y - y)*(ripple.x - y)) - ripple.r);}
  void spawnRandomRipple(uint min_x, uint max_x, uint min_y, uint max_y);
  std::vector<Ring> ripples;
  uint rippleGenerationAttempts = 1;
  uint rippleGenerationProba = 5;  // percentage

  public:
    RipplesProgram(float speed) : WS2812MatrixProgram(speed) {this->spawnRandomRipple(0, 15, 0, 15);};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};

class MatrixEffectProgram: public WS2812MatrixProgram {
  private:
    class ValueMap {
      private:
          const int w;
          const int h;

      public:
          ValueMap(int width, int height) : w(width), h(height) {value_map.resize(h, std::vector<uint32_t>(w, 0));}
          int width() {return w;};
          int height() {return h;};
          std::vector<std::vector<uint32_t>> value_map;
      };

    const uint16_t color = 0x00ff00;
    const float dimming_factor = 0.67f;
    const uint drop_generation_attemps = 3;
    const uint drop_generation_proba = 10;  // percentage
    ValueMap matrix_curr;

  public:
    MatrixEffectProgram(float speed, uint height, uint width) : WS2812MatrixProgram(speed), matrix_curr(height, width) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};


class VortexProgram: public WS2812MatrixProgram {
  private:
    GaussianBlur gaussian_blur = GaussianBlur(0.75f);
  public:
    VortexProgram(float speed) : WS2812MatrixProgram(speed) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};


class RotatingKaleidoscopeProgram: public WS2812MatrixProgram {
  private:
    GaussianBlur gaussian_blur = GaussianBlur(0.3f);
  public:
    RotatingKaleidoscopeProgram(float speed) : WS2812MatrixProgram(speed) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};


class OctopusProgram: public WS2812MatrixProgram {  // Taken from https://editor.soulmatelights.com/gallery/671-octopus
  private:
    GaussianBlur gaussian_blur = GaussianBlur(0.5f);
    float r_map_angle[16][16];
    float r_map_radius[16][16];
    uint8_t arms_min = 1;
    uint8_t arms_max = 5;
  public:
    OctopusProgram(float speed, int height, int width);
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};


class BurstsProgram: public WS2812MatrixProgram {  // Taken from https://github.com/Aircoookie/WLED/blob/main/wled00/FX.cpp#L4724
  private:
    uint8_t num_lines = 10;
    uint8_t min_lines = 5;
    uint8_t max_lines = 15;
    GaussianBlur gaussian_blur = GaussianBlur(0.45f);
  public:
    BurstsProgram(float speed) : WS2812MatrixProgram(speed) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};


class LissajousProgram: public WS2812MatrixProgram {
  private:
    //GaussianBlur gaussian_blur = GaussianBlur(0.3f);
  public:
    LissajousProgram(float speed) : WS2812MatrixProgram(speed) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};


class DnaSpiralProgram: public WS2812MatrixProgram {
  public:
    DnaSpiralProgram(float speed) : WS2812MatrixProgram(speed) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};


class TetrahedronProgram: public WS2812MatrixProgram {
  private:
    class Point {
      public:
          uint8_t id;
          float x0, y0, z0;
          float x, y, z;
          Point(float x0, float y0, float z0, uint8_t id) : x0(x0), y0(y0), z0(z0), x(x0), y(y0), z(z0), id(id) {};
          void rotateX(float x, float y, float z, float a);
          void rotateY(float x, float y, float z, float a);
          void rotateZ(float x, float y, float z, float a);
      };
    Point points[4] = {
      Point(1, 1, 1, 0),
      Point(1, -1, -1, 1),
      Point(-1, -1, 1, 2),
      Point(-1, 1, -1, 3)
      };
    const float camera_distance = 20.0f;
    const float tetrahedron_scale = 5.4f;
    GaussianBlur gaussian_blur = GaussianBlur(0.45f);
    uint16_t getEdgeHue(TetrahedronProgram::Point const & a, TetrahedronProgram::Point const & b);  // This is necessary to ensure that 2 connected vertices always maintain the same edge color
    float sign(TetrahedronProgram::Point const & p1, TetrahedronProgram::Point const & p2, TetrahedronProgram::Point const & p3);  // Taken from https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
    bool pointIsInTriangle(TetrahedronProgram::Point const & p, TetrahedronProgram::Point const & v1, TetrahedronProgram::Point const & v2, TetrahedronProgram::Point const & v3);
    bool isInBeetween(TetrahedronProgram::Point const & p, TetrahedronProgram::Point const & v1, TetrahedronProgram::Point const & v2, float epsilon);
  public:
    TetrahedronProgram(float speed) : WS2812MatrixProgram(speed) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};



class StretchyTetrahedronProgram: public WS2812MatrixProgram {
  private:
    GaussianBlur gaussian_blur = GaussianBlur(0.35f);
  public:
    StretchyTetrahedronProgram(float speed) : WS2812MatrixProgram(speed) {};
    void iterate(Adafruit_NeoMatrix &matrix, float time);
};

#endif
