#include <math.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <stdlib.h>
#include "utils.h"
#include "ws2812_program.h"
#include "RotaryEncoder.h"
#include "ezButton.h"
#include "config_save.h"

#define NUMBER_OF_PROGRAMS 12
#define FRAMERATE 31  // Frames per second
#define HEIGHT 16
#define WIDTH 16
#define NEOMATRIX_PIN 29
#define MATRIX_VOLTAGE 5  // Volts
#define MATRIX_CURRENT_DRAW_PER_CHANNEL 0.020  // Amperes
#define MAX_CURRENT_DRAW 2.0 // Amperes
#define ROT_ENC_BUTTON_PIN 0 
#define ROTARY_ENC_DT_PIN 1
#define ROT_ENC_CLK_PIN 2

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(WIDTH, HEIGHT, NEOMATRIX_PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);
RotaryEncoder rotary_encoder(ROT_ENC_CLK_PIN, ROTARY_ENC_DT_PIN, RotaryEncoder::LatchMode::TWO03);
ezButton button(ROT_ENC_BUTTON_PIN);  // create ezButton object that attach to pin 7;

const float timestep = 1.0 / FRAMERATE;
float brightness = 0.1f;
unsigned long t0;
unsigned long t1;
double t = 0.0f;
double time_of_last_encoder_use = 0.0f;
float current_draw;
RotaryEncoder::Direction rotary_encoder_direction;
int selected_program = 0;
bool is_selecting_program = false;  // If true : selects the program. If false. Selects the brightness
bool button_has_been_released = true;

WS2812MatrixProgram *programs[NUMBER_OF_PROGRAMS];
StaticProgram static_white_prog = StaticProgram(0, Adafruit_NeoMatrix::Color(255, 255, 255));
StaticProgram static_warm_yellow_prog = StaticProgram(0, Adafruit_NeoMatrix::Color(255, 182, 78));
StaticProgram static_red_prog = StaticProgram(0, Adafruit_NeoMatrix::Color(255, 0, 0));
StaticProgram static_green_prog = StaticProgram(0, Adafruit_NeoMatrix::Color(0, 255, 0));
StaticProgram static_blue_prog = StaticProgram(0, Adafruit_NeoMatrix::Color(0, 0, 255));
SpectralProgram spectral_prog = SpectralProgram(0.025);
RainbowWaveProgram rainbow_wave_prog = RainbowWaveProgram(0.2, 1);
RainbowPlasmaProgram rainbow_plasma_prog = RainbowPlasmaProgram(.125, 15);
PerlinFireProgram perlin_fire_prog = PerlinFireProgram(.5, 15, matrix.width(), matrix.height(), 3.5, 5);
FallingSandProgram falling_sand_prog = FallingSandProgram(1/3.0f, matrix.width(), matrix.height());
LavaLampProgram lava_lamp_prog = LavaLampProgram(0.15, matrix.width(), matrix.height(), 11, 125);
MatrixEffectProgram matrix_effect_prog = MatrixEffectProgram(1, matrix.width(), matrix.height());
//VortexProgram vortex_program = VortexProgram(1.0f);

void setup() {
  randomSeed(micros() + analogRead(NEOMATRIX_PIN));
  t += random(10000);

  programs[0] = &static_white_prog;
  programs[1] = &static_warm_yellow_prog;
  programs[2] = &static_red_prog;
  programs[3] = &static_green_prog;
  programs[4] = &static_blue_prog;
  programs[5] = &spectral_prog;
  programs[6] = &rainbow_wave_prog;
  programs[7] = &rainbow_plasma_prog;
  programs[8] = &perlin_fire_prog;
  programs[9] = &falling_sand_prog;
  programs[10] = &lava_lamp_prog;
  programs[11] = &matrix_effect_prog;
  //programs[12] = &vortex_program;

  AppConfig* config = loadConfig();
  brightness = config->brightness;
  selected_program = config->selected_program;
  brightness = max(0, min(1, brightness));
  selected_program = max(0, min(NUMBER_OF_PROGRAMS - 1, selected_program));

  matrix.begin();
  matrix.setBrightness(255);
}

void loop() {
  t0 = millis();

  button.loop();
  if (button.isPressed() && button_has_been_released) {
    button_has_been_released = false;
    is_selecting_program = !is_selecting_program;
  }
  if (!button.isPressed()) {
    button_has_been_released = true;
  }

  rotary_encoder.tick();
  rotary_encoder_direction = rotary_encoder.getDirection();
  if (
    rotary_encoder_direction != RotaryEncoder::Direction::NOROTATION
    && (t - time_of_last_encoder_use) > 0.10f
    ) {
    if (is_selecting_program) {
      matrix.fillScreen(0);
      switch (rotary_encoder_direction) {
        case RotaryEncoder::Direction::CLOCKWISE:
          selected_program += 1;
          break;
        case RotaryEncoder::Direction::COUNTERCLOCKWISE:
          selected_program -= 1;
          break;
      }
      selected_program = (selected_program + NUMBER_OF_PROGRAMS) % NUMBER_OF_PROGRAMS;
    }
    else {
      switch (rotary_encoder_direction) {
        case RotaryEncoder::Direction::CLOCKWISE:
          brightness *= 0.8;
          break;
        case RotaryEncoder::Direction::COUNTERCLOCKWISE:
          brightness *= 1.2;
          break;
      }
      brightness = max(0.01, min(1.0, brightness));
    }
    AppConfig config;
    config.selected_program = selected_program;
    config.brightness = brightness;
    saveConfig(config);
    time_of_last_encoder_use = t;
  }

  programs[selected_program]->iterate(matrix, t);

  matrixApplyBrightness(matrix, brightness);
  current_draw = matrixCurrentDraw(matrix, MATRIX_CURRENT_DRAW_PER_CHANNEL);
  if (current_draw > MAX_CURRENT_DRAW) {
    brightness = brightness * (MAX_CURRENT_DRAW / current_draw) - 0.01;
    matrixApplyBrightness(matrix, brightness);
  }
  matrix.show();

  t1 = millis();
  delay(timestep * 1000 - (t1 - t0));

  Serial.print(matrixCurrentDraw(matrix, MATRIX_CURRENT_DRAW_PER_CHANNEL)); Serial.print(" A | ");
  Serial.print(matrixPowerDraw(matrix, MATRIX_CURRENT_DRAW_PER_CHANNEL, MATRIX_VOLTAGE)); Serial.print(" W | ");
  char c[2];
  sprintf(c, "%2d", t1 - t0);
  Serial.print("Time spent in cycle : "); Serial.print(c); Serial.print("ms | ");
  Serial.print(1000.0 / (millis() - t0)); Serial.print(" fps | ");
  Serial.print("Brightness : "); Serial.print(brightness); Serial.print(" | ");
  Serial.print("Program n° : "); Serial.print(selected_program); Serial.print(" | ");
  Serial.println("");

  t += timestep;
}