#include "display_driver.h"
#include "touch.h"
#include <SPI.h>

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10];

static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv_1;
static lv_indev_drv_t indev_drv_2;

static inline void tft_cmd(uint8_t cmd) {
    digitalWrite(PIN_TFT_DC, LOW);
    digitalWrite(PIN_TFT_CS, LOW);
    SPI.transfer(cmd);
    digitalWrite(PIN_TFT_CS, HIGH);
}

static inline void tft_data(uint8_t data) {
    digitalWrite(PIN_TFT_DC, HIGH);
    digitalWrite(PIN_TFT_CS, LOW);
    SPI.transfer(data);
    digitalWrite(PIN_TFT_CS, HIGH);
}

static void tft_init_hardware() {
    pinMode(PIN_TFT_CS, OUTPUT);
    pinMode(PIN_TFT_DC, OUTPUT);
    digitalWrite(PIN_TFT_CS, HIGH);
    digitalWrite(PIN_TFT_DC, HIGH);

    pinMode(PIN_TFT_RST, OUTPUT);
    digitalWrite(PIN_TFT_RST, HIGH);
    delay(20);
    digitalWrite(PIN_TFT_RST, LOW);
    delay(50);
    digitalWrite(PIN_TFT_RST, HIGH);
    delay(150);

    SPI.begin(PIN_TFT_SCLK, -1, PIN_TFT_MOSI, PIN_TFT_CS);
    SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));

    tft_cmd(0x01); // Reset
    delay(120);

    tft_cmd(0x11); // Sleep Out
    delay(120);

    tft_cmd(0x3A); // 16-bit RGB565
    tft_data(0x55);
    delay(10);

    tft_cmd(0xB4); // 1-dot column inversion
    tft_data(0x01);

    tft_cmd(0x36); // Modo Paisagem (Landscape)
    tft_data(0xE8);
    delay(10);

    tft_cmd(0x20); // Inversão OFF
    tft_cmd(0x29); // Display ON
    delay(50);
}

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    uint32_t len = w * h;

    tft_cmd(0x2A);
    tft_data(area->x1 >> 8);
    tft_data(area->x1 & 0xFF);
    tft_data(area->x2 >> 8);
    tft_data(area->x2 & 0xFF);

    tft_cmd(0x2B);
    tft_data(area->y1 >> 8);
    tft_data(area->y1 & 0xFF);
    tft_data(area->y2 >> 8);
    tft_data(area->y2 & 0xFF);

    tft_cmd(0x2C);
    digitalWrite(PIN_TFT_DC, HIGH);
    digitalWrite(PIN_TFT_CS, LOW);

    uint16_t *p = (uint16_t *)color_p;
    for (uint32_t i = 0; i < len; i++) {
        SPI.transfer(p[i] >> 8);
        SPI.transfer(p[i] & 0xFF);
    }

    digitalWrite(PIN_TFT_CS, HIGH);
    lv_disp_flush_ready(disp);
}

// Callback do Toque 1
static void my_touchpad_read_1(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    touch_read_hardware(); // Atualiza os dois pontos via I2C
    if (touch_has_signal() && touch_touched_1()) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch_last_x;
        data->point.y = touch_last_y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// Callback do Toque 2
static void my_touchpad_read_2(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    if (touch_has_signal() && touch_touched_2()) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch2_last_x;
        data->point.y = touch2_last_y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void display_init() {
    tft_init_hardware();
    touch_init(SCREEN_WIDTH, SCREEN_HEIGHT, 3);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * SCREEN_HEIGHT / 10);

    // Registro do Display
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Registro do Primeiro Indev (Ponteiro 1)
    lv_indev_drv_init(&indev_drv_1);
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;
    indev_drv_1.read_cb = my_touchpad_read_1;
    lv_indev_drv_register(&indev_drv_1);

    // Registro do Segundo Indev (Ponteiro 2)
    lv_indev_drv_init(&indev_drv_2);
    indev_drv_2.type = LV_INDEV_TYPE_POINTER;
    indev_drv_2.read_cb = my_touchpad_read_2;
    lv_indev_drv_register(&indev_drv_2);
}

void display_update() {
    lv_timer_handler();
}