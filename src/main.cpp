#include <Arduino.h>
#include "display_driver.h"
#include "RTClib.h"
#include <DFRobotDFPlayerMini.h>

RTC_DS1307 rtc;


lv_obj_t* menuScr;

bool btn_play_stop_state = false;
lv_obj_t * label_artist_name;
lv_obj_t * label_song_name;
lv_obj_t * label_song_time_max;
lv_obj_t * label_song_time_now;
// lv_obj_t * button_vol_plus =     lv_btn_create(menuScr);
// lv_obj_t * button_vol_minus =    lv_btn_create(menuScr);
lv_obj_t * btn_play_stop ;

lv_obj_t * bar_prog_mus;
// lv_obj_t * icone_volume =        lv_img_create(menuScr);
lv_obj_t * icon_play_pause;

void setup()
{
    Serial.begin(115200);
    // Uma única linha inicializa tudo: ST7796, FT6336, buffers e LVGL
    display_init();
    // --- Construção da Interface Gráfica ---
    menuScr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(menuScr, lv_color_hex(0x1E1E1E), 0);
    lv_scr_load(menuScr);

    label_artist_name = lv_label_create(menuScr);
    label_song_name = lv_label_create(menuScr);
    label_song_time_max = lv_label_create(menuScr);
    label_song_time_now = lv_label_create(menuScr);
    btn_play_stop = lv_btn_create(menuScr);
    bar_prog_mus = lv_bar_create(menuScr);
    icon_play_pause = lv_img_create(btn_play_stop);

    lv_label_set_text(label_artist_name, "Artista");
    lv_label_set_text(label_song_name, "Nome da Musica");
    lv_obj_align(label_song_name, LV_ALIGN_LEFT_MID, 60, -40);
    lv_obj_set_style_text_font(label_song_name, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_align_to(label_artist_name, label_song_name, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_width(bar_prog_mus, 260);
    lv_obj_set_height(bar_prog_mus, 5);
    lv_obj_align_to(bar_prog_mus, label_artist_name, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_text_font(label_song_time_now, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_font(label_song_time_max, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_label_set_text(label_song_time_now, "0:00");
    lv_label_set_text(label_song_time_max, "3:45");
    lv_obj_align_to(label_song_time_now, bar_prog_mus, LV_ALIGN_OUT_LEFT_MID, -3, 0);
    lv_obj_align_to(label_song_time_max, bar_prog_mus, LV_ALIGN_OUT_RIGHT_MID, 3, 0);

    lv_obj_align_to(btn_play_stop, bar_prog_mus, LV_ALIGN_OUT_BOTTOM_MID, -5, 20);
    lv_obj_set_size(btn_play_stop, 50, 50);
    lv_obj_set_style_radius(btn_play_stop, 25, LV_PART_MAIN);
    lv_img_set_src(icon_play_pause, LV_SYMBOL_PLAY);
    lv_obj_center(icon_play_pause);
    lv_obj_add_event_cb(btn_play_stop, [](lv_event_t * e) {
        btn_play_stop_state = !btn_play_stop_state;
        lv_img_set_src(icon_play_pause, btn_play_stop_state ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    }, LV_EVENT_CLICKED, NULL);
}

void loop()
{
    display_update();
    delay(2);
}