#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include <lvgl.h>

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 320

// Nova pinagem sequencial para cabo flat
#define PIN_TFT_CS    5
#define PIN_TFT_RST  17
#define PIN_TFT_DC   16
#define PIN_TFT_MOSI 23
#define PIN_TFT_SCLK 18

void display_init();
void display_update();

#endif // DISPLAY_DRIVER_H