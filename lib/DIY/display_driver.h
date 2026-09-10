#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include <lvgl.h>

// Resolução configurada no modo Paisagem (Landscape)
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 320

// Mapeamento dos pinos de hardware SPI
#define PIN_TFT_MOSI 23
#define PIN_TFT_SCLK 18
#define PIN_TFT_CS    5
#define PIN_TFT_DC    2
#define PIN_TFT_RST   4

/**
 * @brief Inicializa o barramento SPI, o controlador ST7796, o Touch I2C e o LVGL.
 */
void display_init();

/**
 * @brief Executa as tarefas periódicas do LVGL. Deve ser chamada dentro de loop().
 */
void display_update();

#endif // DISPLAY_DRIVER_H