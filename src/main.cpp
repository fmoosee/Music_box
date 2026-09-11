#include <Arduino.h>
#include "display_driver.h"
#include <RTClib.h>
#include <DFRobotDFPlayerMini.h>

RTC_DS1307 rtc; // Instância da biblioteca RTC

lv_obj_t* menuScr; // Tela principal

bool button_play_pause_state = false; // Estado do botão play/stop
lv_obj_t * label_artist_name, * label_song_name;
lv_obj_t * label_song_time_max, * label_song_time_now;
lv_obj_t * button_play_pause, * img_play_pause;
lv_obj_t * bar_prog_mus;


void btn_play_pause_cb(lv_event_t * e) {
    button_play_pause_state = !button_play_pause_state;
    lv_img_set_src(img_play_pause, button_play_pause_state ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    Serial.printf("[AUDIO] Play/Pause alternado para: %s\n", button_play_pause_state ? "PLAY" : "PAUSE");
}

void create_ui(){
    menuScr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(menuScr, lv_color_hex(0x1E1E1E), 0);
    lv_scr_load(menuScr);

    label_artist_name    = lv_label_create(menuScr);
    label_song_name      = lv_label_create(menuScr);
    label_song_time_max  = lv_label_create(menuScr);
    label_song_time_now  = lv_label_create(menuScr);
    bar_prog_mus         = lv_bar_create(menuScr);
    button_play_pause    = lv_btn_create(menuScr);
    img_play_pause       = lv_img_create(button_play_pause);

    lv_img_set_src(img_play_pause, LV_SYMBOL_PLAY);


    lv_obj_set_style_text_font(label_song_name    , &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_font(label_song_time_now, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_font(label_song_time_max, &lv_font_montserrat_12, LV_PART_MAIN);
    
    lv_obj_add_event_cb(button_play_pause, btn_play_pause_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_set_size(bar_prog_mus, 240, 5);
    lv_obj_set_size(button_play_pause, 50, 50);
    lv_obj_set_width(label_artist_name, 240);
    
    lv_obj_set_style_text_align(label_artist_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    lv_obj_center(img_play_pause);

    lv_obj_set_style_radius(button_play_pause, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    
    lv_label_set_text(label_song_name    , "Nome da Musica");
    lv_label_set_text(label_artist_name  , "Artista");
    lv_label_set_text(label_song_time_now, "0:00");
    lv_label_set_text(label_song_time_max, "3:45");
    
    lv_obj_align(label_song_name, LV_ALIGN_LEFT_MID, 60, -40);
    
    lv_obj_align_to(label_artist_name, label_song_name, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_align_to(bar_prog_mus, label_artist_name,    LV_ALIGN_OUT_BOTTOM_MID, 0, 15);
    lv_obj_align_to(button_play_pause, bar_prog_mus,    LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_align_to(label_song_time_now, bar_prog_mus,  LV_ALIGN_OUT_LEFT_MID,  -8, 0);
    lv_obj_align_to(label_song_time_max, bar_prog_mus,  LV_ALIGN_OUT_RIGHT_MID,  8, 0);
}

void setup()
{
    Serial.begin(115200);

    // Inicializa ST7796, FT6336, buffers e LVGL
    display_init();
    create_ui();
}

void loop()
{
    display_update();
    delay(2);
}