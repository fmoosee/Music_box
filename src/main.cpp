#include <Arduino.h>
#include "display_driver.h"
#include <RTClib.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>
#include "sun_icons.h"

// -------------------------------------------------------------------------
// DEFINIÇÕES DO MÓDULO DE ÁUDIO E RTC
// -------------------------------------------------------------------------
#define RX_PIN 25
#define TX_PIN 26

HardwareSerial myHardwareSerial(2);
DFRobotDFPlayerMini myDFPlayer;

RTC_DS1307 rtc; // Instância da biblioteca RTC

// Estrutura de dados das músicas
typedef struct {
    const char* name;
    const char* art_name;
    int music_time; // Tempo em segundos
    int index_pos;  // Esse índice buscará exatamente arquivos "000X.mp3" dentro da pasta "mp3"
} music_data;

bool rtcisrunning = false;
int audio_volume = 15;

// Define a quantidade total de músicas fixa para padronizar os arrays
#define TOTAL_TRACKS 13

// Playlist expandida de acordo com os seus arquivos
music_data playlist[TOTAL_TRACKS] = {
    {"Bolsonaro na Cabeca",        "Bolsonaro",      67, 1},
    {"Bolsonaro e Lula",      "Remix",             90,  2},
    {"Garota Nacional",       "Skank",             318, 3},
    {"Strategy",              "TWICE",             170, 4},
    {"Amiga da minha mulher", "Bolsonaro",         247, 5},
    {"Magica",                "Calcinha Preta",    217, 6},
    {"Levitating",            "Dua Lipa",          229, 7},
    {"Aguas De Marco",        "Elis & Tom",        209, 8},
    {"Epidemo",               "Christos Kiriazis", 278, 9},
    {"Boate Azul",            "BolsoLula",         199, 10},
    {"Billie Jean",           "Bolsonaro",         377, 11},
    {"Evidencias",            "BolsoLula",         275, 12},
    {"Naruto",                "Triste",            46,  13}
};

// -------------------------------------------------------------------------
// VARIÁVEIS DO NOVO SISTEMA "SPOTIFY SHUFFLE" (FISHER-YATES)
// -------------------------------------------------------------------------
int shuffle_queue[TOTAL_TRACKS]; // O "Baralho" de músicas embaralhadas
int shuffle_pos = 0;             // A posição atual dentro do baralho

// Array global para armazenar os ponteiros dos botões da lista lateral
lv_obj_t * list_btns[TOTAL_TRACKS];
int current_track_index = 0; // Guarda a posição da música atual na lista

// Objetos de Telas do LVGL
lv_obj_t* menuScr;       
lv_obj_t* standbyScr;    
lv_obj_t* label_standby_clock; 

char buf[] = "DD/MM/YY - hh:mm";
lv_timer_t * data_att;
lv_obj_t * label_data;

// Variáveis de controle de áudio
bool button_play_pause_state = false; 
int current_song_time = 0;            
int current_song_max_time = 0;        

// Variáveis de controle do Standby
int standby_idle_seconds = 0;        
bool is_in_standby = false;          

lv_obj_t * label_artist_name, * label_song_name;
lv_obj_t * label_song_time_max, * label_song_time_now;
lv_obj_t * button_play_pause, * img_play_pause;
lv_obj_t * bar_prog_mus;
lv_obj_t * btn_vol_plus, * btn_vol_minus;
lv_obj_t * icon_vol_plus, * icon_vol_minus;
lv_obj_t * btn_next, * btn_previous;
lv_obj_t * icon_next, * icon_previous;
lv_obj_t * btn_random, * btn_repeat;
lv_obj_t * icon_random, * icon_repeat;

bool status_repeat = false;
bool status_random = false;

lv_obj_t * bar_vol; // Agora funcionará como um Slider no LVGL
lv_obj_t * icon_bar_vol_plus, * icon_bar_vol_minus;

lv_obj_t * list_music;
lv_obj_t * icon_ble, * icon_wifi;

// -------------------------------------------------------------------------
// ALGORITMO FISHER-YATES SHUFFLE (A Lógica Spotify)
// -------------------------------------------------------------------------
void generate_shuffle_queue() {
    // 1. Preenche o baralho em ordem (0, 1, 2, 3...)
    for (int i = 0; i < TOTAL_TRACKS; i++) {
        shuffle_queue[i] = i;
    }
    
    // 2. Embaralha perfeitamente as posições
    for (int i = TOTAL_TRACKS - 1; i > 0; i--) {
        int j = random(0, i + 1);
        int temp = shuffle_queue[i];
        shuffle_queue[i] = shuffle_queue[j];
        shuffle_queue[j] = temp;
    }
    
    // 3. Regra de Ouro Spotify: Evita que a primeira música do NOVO baralho 
    // seja idêntica à música que está tocando agora (o final do baralho velho)
    if (TOTAL_TRACKS > 1 && shuffle_queue[0] == current_track_index) {
        int temp = shuffle_queue[0];
        shuffle_queue[0] = shuffle_queue[TOTAL_TRACKS - 1]; // Joga pro final
        shuffle_queue[TOTAL_TRACKS - 1] = temp;
    }
    
    // Reseta a posição de leitura do baralho
    shuffle_pos = 0;
}

// -------------------------------------------------------------------------
// FUNÇÕES DE ÁUDIO, HARDWARE E LÓGICA DE FUNDO
// -------------------------------------------------------------------------

void initAudioModule() {
    myHardwareSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.println("Inicializando DFPlayer Mini...");

    if (!myDFPlayer.begin(myHardwareSerial, false, true)) {
        Serial.println("Falha ao iniciar. Verifique conexoes e cartao SD.");
    } else {
        Serial.println("DFPlayer Mini online.");
        myDFPlayer.volume(audio_volume);
        myDFPlayer.playMp3Folder(1);
    }
}

void play_track(int index, bool auto_play = true);
void update_standby_clock();
void enter_standby();
void exit_standby();

void update_standby_clock() {
    DateTime now = rtc.now();
    char time_str[10];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", now.hour(), now.minute());
    lv_label_set_text(label_standby_clock, rtcisrunning ? time_str : "--:--");
}

void enter_standby() {
    is_in_standby = true;
    update_standby_clock();
    lv_scr_load(standbyScr);
    Serial.println("\n[STANDBY] Modo descanso ativado");
}

void exit_standby() {
    is_in_standby = false;
    standby_idle_seconds = 0;
    lv_scr_load(menuScr);
    Serial.println("\n[STANDBY] Retomando tela principal");
}

static void reset_standby_cb(lv_event_t * e) {
    standby_idle_seconds = 0;
}

static void standby_touch_cb(lv_event_t * e) {
    if (is_in_standby) {
        exit_standby();
    }
}

void update_progress_timer_cb(lv_timer_t * timer) {
    if (button_play_pause_state == false) {
        standby_idle_seconds++;
        
        if (standby_idle_seconds >= 20 && !is_in_standby) {
            enter_standby();
        }
    } else {
        standby_idle_seconds = 0;
    }

    if (is_in_standby) {
        update_standby_clock();
        return; 
    }

    if (button_play_pause_state == true) {
        if (current_song_time < current_song_max_time) {
            current_song_time++; 
            lv_bar_set_value(bar_prog_mus, current_song_time, LV_ANIM_ON);
            int min = current_song_time / 60;
            int sec = current_song_time % 60;
            lv_label_set_text_fmt(label_song_time_now, "%d:%02d", min, sec);
        } else {
            lv_refr_now(NULL);
            
            if (status_repeat) {
                play_track(current_track_index);
            } 
            else if (status_random) {
                shuffle_pos++;
                if (shuffle_pos >= TOTAL_TRACKS) {
                    generate_shuffle_queue(); 
                }
                play_track(shuffle_queue[shuffle_pos]);
            } 
            else {
                int next_idx = current_track_index + 1;
                
                if (next_idx < TOTAL_TRACKS) {
                    play_track(next_idx);
                } else {
                    button_play_pause_state = false;
                    lv_img_set_src(img_play_pause, LV_SYMBOL_PLAY);
                    current_song_time = 0;
                    lv_bar_set_value(bar_prog_mus, 0, LV_ANIM_OFF);
                    lv_label_set_text(label_song_time_now, "0:00");
                    myDFPlayer.stop(); 
                }
            }
        }
    }
}

// -------------------------------------------------------------------------
// CONTROLE DE PLAYER E CALLBACKS UI
// -------------------------------------------------------------------------

void play_track(int index, bool auto_play) {
    if (index < 0 || index >= TOTAL_TRACKS) return;
    current_track_index = index;
    const music_data &track = playlist[current_track_index];
    
    Serial.println("\n--- CARREGANDO FAIXA ---");
    Serial.printf("Nome:      %s\n", track.name);
    Serial.printf("Artista:   %s\n", track.art_name);
    Serial.printf("Duracao:   %d segundos\n", track.music_time);
    Serial.printf("Indice SD: %d\n", track.index_pos);

    standby_idle_seconds = 0;

    for (int i = 0; i < TOTAL_TRACKS; i++) {
        if (i == index) {
            lv_obj_set_style_bg_color(list_btns[i], lv_color_hex(0x2A2A2A), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(list_btns[i], LV_OPA_COVER, LV_PART_MAIN);
        } else {
            lv_obj_set_style_bg_opa(list_btns[i], LV_OPA_TRANSP, LV_PART_MAIN);
        }
    }

    // Passa o texto original. O LVGL cuidará do centralizamento ou rolagem automaticamente.
    lv_label_set_text(label_song_name, track.name);
    lv_label_set_text(label_artist_name, track.art_name);
    
    current_song_time = 0;
    current_song_max_time = track.music_time;
    int min = track.music_time / 60;
    int sec = track.music_time % 60;
    
    lv_label_set_text_fmt(label_song_time_max, "%d:%02d", min, sec);
    lv_label_set_text(label_song_time_now, "0:00");
    
    lv_bar_set_range(bar_prog_mus, 0, current_song_max_time);
    lv_bar_set_value(bar_prog_mus, 0, LV_ANIM_OFF);

    if (auto_play) {
        button_play_pause_state = true;
        lv_img_set_src(img_play_pause, LV_SYMBOL_PAUSE);
        myDFPlayer.playMp3Folder(track.index_pos);
    } else {
        button_play_pause_state = false;
        lv_img_set_src(img_play_pause, LV_SYMBOL_PLAY);
        myDFPlayer.stop(); 
    }
}

void btn_next_cb(lv_event_t * e) {
    standby_idle_seconds = 0;
    int next_idx;
    
    if (status_random) {
        shuffle_pos++;
        if (shuffle_pos >= TOTAL_TRACKS) {
            generate_shuffle_queue();
        }
        next_idx = shuffle_queue[shuffle_pos];
    } else {
        next_idx = current_track_index + 1;
        if (next_idx >= TOTAL_TRACKS) {
            next_idx = 0;
        }
    }
    
    play_track(next_idx);
}

void btn_previous_cb(lv_event_t * e) {
    standby_idle_seconds = 0;
    
    if (current_song_time > 3) {
        play_track(current_track_index);
    } else {
        int prev_idx;
        
        if (status_random) {
            shuffle_pos--;
            if (shuffle_pos < 0) {
                shuffle_pos = TOTAL_TRACKS - 1; 
            }
            prev_idx = shuffle_queue[shuffle_pos];
        } else {
            prev_idx = current_track_index - 1;
            if (prev_idx < 0) {
                prev_idx = TOTAL_TRACKS - 1; 
            }
        }
        
        play_track(prev_idx);
    }
}

static void song_item_clicked_cb(lv_event_t * e) {
    standby_idle_seconds = 0;
    const music_data * track = (const music_data *)lv_event_get_user_data(e);
    if (track == NULL) return;
    
    int index = track - playlist; 
    
    if (status_random) {
        for(int i = 0; i < TOTAL_TRACKS; i++){
            if(shuffle_queue[i] == index){
                shuffle_pos = i;
                break;
            }
        }
    }
    
    play_track(index);
}

lv_obj_t * add_audio_list_item(lv_obj_t * parent, const music_data * song) {
    lv_obj_t * btn = lv_list_add_btn(parent, NULL, NULL);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, 45);
    
    lv_obj_set_style_pad_left(btn, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_right(btn, 2, LV_PART_MAIN);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);

    int display_index = (song - playlist) + 1; 
    lv_obj_t * label_index = lv_label_create(btn);
    lv_label_set_text_fmt(label_index, "%d -", display_index);
    lv_obj_set_style_text_font(label_index, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_index, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_width(label_index, 26); 
    lv_obj_align(label_index, LV_ALIGN_LEFT_MID, 2, 1);

    lv_obj_t * icon_box = lv_obj_create(btn);
    lv_obj_set_size(icon_box, 30, 32);
    lv_obj_set_style_bg_opa(icon_box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(icon_box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(icon_box, 0, LV_PART_MAIN);
    lv_obj_clear_flag(icon_box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align_to(icon_box, label_index, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    lv_obj_t * icon_file = lv_label_create(icon_box);
    lv_label_set_text(icon_file, LV_SYMBOL_FILE);
    lv_obj_set_style_text_font(icon_file, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon_file, lv_color_hex(0x9E9E9E), LV_PART_MAIN);
    lv_obj_center(icon_file);

    lv_obj_t * icon_audio = lv_label_create(icon_box);
    lv_label_set_text(icon_audio, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(icon_audio, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon_audio, lv_color_hex(0x101010), LV_PART_MAIN);
    lv_obj_align(icon_audio, LV_ALIGN_CENTER, 0, 2);

    lv_obj_t * label_title = lv_label_create(btn);
    lv_label_set_text(label_title, song->name); 
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_label_set_long_mode(label_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align_to(label_title, icon_box, LV_ALIGN_OUT_RIGHT_MID, 2, 0);
    lv_obj_set_width(label_title, 45); 
    lv_obj_set_style_anim_speed(label_title, 20, LV_PART_MAIN);

    lv_obj_add_event_cb(btn, song_item_clicked_cb, LV_EVENT_CLICKED, (void *)song);
    
    return btn; 
}

void btn_play_pause_cb(lv_event_t * e) {
    standby_idle_seconds = 0;
    button_play_pause_state = !button_play_pause_state;
    lv_img_set_src(img_play_pause, button_play_pause_state ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    
    if (button_play_pause_state) {
        if (current_song_time == 0) {
            myDFPlayer.playMp3Folder(playlist[current_track_index].index_pos);
        } else {
            myDFPlayer.start();
        }
    } else {
        myDFPlayer.pause();
    }
}

void att_timer(lv_timer_t * timer){
    DateTime now = rtc.now();
    lv_label_set_text(label_data, rtcisrunning ? now.toString(buf) : "--/--/-- - --:--");
}

// Novo Callback de Botões Volume (+ e -)
static void btn_volume_cb(lv_event_t * e) {
    standby_idle_seconds = 0;
    lv_event_code_t code = lv_event_get_code(e);
    int step = (int)(intptr_t)lv_event_get_user_data(e);

    if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
        audio_volume += step;
        if (audio_volume > 30) audio_volume = 30;
        if (audio_volume < 0)  audio_volume = 0;

        lv_slider_set_value(bar_vol, audio_volume, LV_ANIM_ON); // Sincroniza com a nova UI de Slider
        LV_LOG_INFO("Audio Modified %02d", audio_volume);
        myDFPlayer.volume(audio_volume);
    }
}

// Novo Callback do Slider Deslizante de Volume
static void slider_volume_cb(lv_event_t * e) {
    standby_idle_seconds = 0;
    lv_obj_t * slider = lv_event_get_target(e);
    audio_volume = (int)lv_slider_get_value(slider);
    
    myDFPlayer.volume(audio_volume);
    LV_LOG_INFO("Volume deslizado para: %d", audio_volume);
}

static void btn_repeat_cb(lv_event_t * e){
    standby_idle_seconds = 0;
    status_repeat = !status_repeat;
    lv_obj_set_style_opa(icon_repeat, status_repeat ? LV_OPA_100 : LV_OPA_30, LV_PART_MAIN);
    LV_LOG_INFO("Status repeat %s", status_repeat ? "Ativado" : "Desativado");
}

static void btn_random_cb(lv_event_t * e){
    standby_idle_seconds = 0;
    status_random = !status_random;
    lv_obj_set_style_opa(icon_random, status_random ? LV_OPA_100 : LV_OPA_30, LV_PART_MAIN);
    LV_LOG_INFO("Musica Aleatoria %s", status_random ? "Ativado" : "Desativado");

    if(status_random) {
        generate_shuffle_queue();
        for(int i = 0; i < TOTAL_TRACKS; i++){
            if(shuffle_queue[i] == current_track_index){
                shuffle_pos = i;
                break;
            }
        }
    }
}

// -------------------------------------------------------------------------
// CRIAÇÃO DA INTERFACE (UI)
// -------------------------------------------------------------------------
void create_ui() {
    menuScr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(menuScr, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
    
    lv_obj_add_event_cb(menuScr, reset_standby_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(menuScr, reset_standby_cb, LV_EVENT_SCROLL, NULL);
    
    lv_scr_load(menuScr);

    icon_wifi = lv_img_create(menuScr);
    lv_img_set_src(icon_wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_opa(icon_wifi, LV_OPA_10, LV_PART_MAIN); 
    lv_obj_align(icon_wifi, LV_ALIGN_TOP_LEFT, 5, 5);

    icon_ble = lv_img_create(menuScr);
    lv_img_set_src(icon_ble, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_opa(icon_ble, LV_OPA_50, LV_PART_MAIN);
    lv_obj_align_to(icon_ble, icon_wifi, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    label_data = lv_label_create(menuScr);
    lv_obj_set_style_text_font(label_data, &lv_font_unscii_8, LV_PART_MAIN);
    DateTime now = rtc.now();
    lv_label_set_text(label_data, rtcisrunning ? now.toString(buf) : "--/--/-- - --:--");
    lv_obj_align_to(label_data, icon_ble, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
    lv_obj_update_layout(label_data);
    
    data_att = lv_timer_create(att_timer, 30000, NULL);

    // Substituição da Barra passiva pelo Slider Deslizante
    bar_vol = lv_slider_create(menuScr);
    lv_obj_set_size(bar_vol, 5, 100); 
    lv_slider_set_range(bar_vol, 0, 30);
    lv_slider_set_value(bar_vol, audio_volume, LV_ANIM_OFF);
    lv_obj_set_ext_click_area(bar_vol, 20); // Aumenta a área de toque invisível para facilitar pegar com o dedo
    lv_obj_set_style_bg_color(bar_vol, lv_color_hex(0x484848), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_vol, lv_color_hex(0x00E5FF), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar_vol, LV_OPA_TRANSP, LV_PART_KNOB); // Esconde a "bolinha" do slider para manter o visual limpo de barra
    lv_obj_align(bar_vol, LV_ALIGN_TOP_LEFT, 15, 65);
    
    // Associa o evento de arraste
    lv_obj_add_event_cb(bar_vol, slider_volume_cb, LV_EVENT_VALUE_CHANGED, NULL);

    icon_bar_vol_plus = lv_img_create(menuScr);
    lv_img_set_src(icon_bar_vol_plus, LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_opa(icon_bar_vol_plus, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align_to(icon_bar_vol_plus, bar_vol, LV_ALIGN_OUT_TOP_MID, 0, -5);

    icon_bar_vol_minus = lv_img_create(menuScr);
    lv_img_set_src(icon_bar_vol_minus, LV_SYMBOL_MUTE);
    lv_obj_set_style_opa(icon_bar_vol_minus, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align_to(icon_bar_vol_minus, bar_vol, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    // Ajuste de Alinhamento Dinâmico para Títulos e Artistas
    label_song_name = lv_label_create(menuScr);
    lv_obj_set_width(label_song_name, 220);
    lv_obj_set_style_text_font(label_song_name, &lv_font_montserrat_28, LV_PART_MAIN);
    // Configura texto para centralizar. O Scroll_Circular só é ativado pelo LVGL se a largura do texto bater os 220px.
    lv_obj_set_style_text_align(label_song_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN); 
    lv_label_set_long_mode(label_song_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(label_song_name, LV_ALIGN_LEFT_MID, 70, -40);

    label_artist_name = lv_label_create(menuScr);
    lv_obj_set_width(label_artist_name, 220);
    lv_obj_set_style_text_align(label_artist_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_label_set_long_mode(label_artist_name, LV_LABEL_LONG_SCROLL_CIRCULAR); 
    lv_obj_align_to(label_artist_name, label_song_name, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    bar_prog_mus = lv_bar_create(menuScr);
    lv_obj_set_size(bar_prog_mus, 240, 5);
    lv_obj_align_to(bar_prog_mus, label_artist_name, LV_ALIGN_OUT_BOTTOM_MID, 0, 30);

    label_song_time_now = lv_label_create(menuScr);
    lv_obj_set_style_text_font(label_song_time_now, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align_to(label_song_time_now, bar_prog_mus, LV_ALIGN_OUT_LEFT_MID, -8, 0);

    label_song_time_max = lv_label_create(menuScr);
    lv_obj_set_style_text_font(label_song_time_max, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align_to(label_song_time_max, bar_prog_mus, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    button_play_pause = lv_btn_create(menuScr);
    lv_obj_set_size(button_play_pause, 40, 40);
    lv_obj_set_style_radius(button_play_pause, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align_to(button_play_pause, bar_prog_mus, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    img_play_pause = lv_img_create(button_play_pause);
    lv_img_set_src(img_play_pause, LV_SYMBOL_PLAY);
    lv_obj_center(img_play_pause);
    lv_obj_add_event_cb(button_play_pause, btn_play_pause_cb, LV_EVENT_CLICKED, NULL);

    btn_previous = lv_btn_create(menuScr);
    lv_obj_set_size(btn_previous, 30, 30);
    lv_obj_set_style_radius(btn_previous, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align_to(btn_previous, button_play_pause, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    icon_previous = lv_img_create(btn_previous);
    lv_img_set_src(icon_previous, LV_SYMBOL_PREV);
    lv_obj_center(icon_previous);
    lv_obj_add_event_cb(btn_previous, btn_previous_cb, LV_EVENT_CLICKED, NULL);

    btn_next = lv_btn_create(menuScr);
    lv_obj_set_size(btn_next, 30, 30);
    lv_obj_set_style_radius(btn_next, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align_to(btn_next, button_play_pause, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    icon_next = lv_img_create(btn_next);
    lv_img_set_src(icon_next, LV_SYMBOL_NEXT);
    lv_obj_center(icon_next);
    lv_obj_add_event_cb(btn_next, btn_next_cb, LV_EVENT_CLICKED, NULL);

    btn_vol_minus = lv_btn_create(menuScr);
    lv_obj_set_size(btn_vol_minus, 30, 30);
    lv_obj_set_style_radius(btn_vol_minus, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align_to(btn_vol_minus, btn_previous, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    icon_vol_minus = lv_img_create(btn_vol_minus);
    lv_img_set_src(icon_vol_minus, LV_SYMBOL_VOLUME_MID);
    lv_obj_center(icon_vol_minus);
    lv_obj_add_event_cb(btn_vol_minus, btn_volume_cb, LV_EVENT_SHORT_CLICKED, (void *)(intptr_t)-1);
    lv_obj_add_event_cb(btn_vol_minus, btn_volume_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)-1);

    btn_vol_plus = lv_btn_create(menuScr);
    lv_obj_set_size(btn_vol_plus, 30, 30);
    lv_obj_set_style_radius(btn_vol_plus, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align_to(btn_vol_plus, btn_next, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    icon_vol_plus = lv_img_create(btn_vol_plus);
    lv_img_set_src(icon_vol_plus, LV_SYMBOL_VOLUME_MAX);
    lv_obj_center(icon_vol_plus);
    lv_obj_add_event_cb(btn_vol_plus, btn_volume_cb, LV_EVENT_SHORT_CLICKED, (void *)(intptr_t)1);
    lv_obj_add_event_cb(btn_vol_plus, btn_volume_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)1);

    btn_random = lv_btn_create(menuScr);
    lv_obj_set_size(btn_random, 45, 45);
    lv_obj_set_style_bg_opa(btn_random, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn_random, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(btn_random, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(btn_random, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_opa(btn_random, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(btn_random, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_align_to(btn_random, btn_previous, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    icon_random = lv_img_create(btn_random);
    lv_img_set_src(icon_random, LV_SYMBOL_SHUFFLE);
    lv_obj_set_style_opa(icon_random, LV_OPA_30, LV_PART_MAIN);
    lv_obj_center(icon_random);
    lv_obj_add_event_cb(btn_random, btn_random_cb, LV_EVENT_CLICKED, NULL);

    btn_repeat = lv_btn_create(menuScr);
    lv_obj_set_size(btn_repeat, 45, 45);
    lv_obj_set_style_bg_opa(btn_repeat, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn_repeat, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(btn_repeat, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(btn_repeat, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_opa(btn_repeat, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(btn_repeat, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_align_to(btn_repeat, btn_next, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    icon_repeat = lv_img_create(btn_repeat);
    lv_img_set_src(icon_repeat, LV_SYMBOL_LOOP);
    lv_obj_set_style_opa(icon_repeat, LV_OPA_30, LV_PART_MAIN);
    lv_obj_center(icon_repeat);
    lv_obj_add_event_cb(btn_repeat, btn_repeat_cb, LV_EVENT_CLICKED, NULL);

    list_music = lv_list_create(menuScr);
    lv_obj_set_size(list_music, 135, 320);
    lv_obj_set_style_radius(list_music, 0, LV_PART_MAIN);
    lv_obj_align(list_music, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_list_add_text(list_music, "Musicas Salvas:");
    
    lv_obj_add_event_cb(list_music, reset_standby_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(list_music, reset_standby_cb, LV_EVENT_SCROLL, NULL);
    
    for(size_t i = 0; i < TOTAL_TRACKS; i++) {
        list_btns[i] = add_audio_list_item(list_music, &playlist[i]);
    }

    standbyScr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(standbyScr, lv_color_hex(0x000000), LV_PART_MAIN);
    
    label_standby_clock = lv_label_create(standbyScr);
    lv_label_set_text(label_standby_clock, "00:00");
    lv_obj_set_style_text_font(label_standby_clock, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_standby_clock, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_center(label_standby_clock);
    lv_obj_add_flag(standbyScr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(standbyScr, standby_touch_cb, LV_EVENT_CLICKED, NULL);

    lv_timer_create(update_progress_timer_cb, 1000, NULL);
    lv_timer_create([](lv_timer_t * timer){rtcisrunning = rtc.begin();}, 10000, NULL);
}

// -------------------------------------------------------------------------
// FUNÇÃO DE FEEDBACK DO MÓDULO (Para debug no Serial Monitor)
// -------------------------------------------------------------------------
void printDFPlayerStatus() {
    if (myDFPlayer.available()) {
        uint8_t type = myDFPlayer.readType();
        int value = myDFPlayer.read();

        Serial.print("DFPlayer: ");
        switch (type) {
            case TimeOut: Serial.println("Tempo esgotado (Time Out)!"); break;
            case WrongStack: Serial.println("Falha na pilha de comandos!"); break;
            case DFPlayerCardInserted: Serial.println("Cartao SD Inserido!"); break;
            case DFPlayerCardRemoved: Serial.println("Cartao SD Removido!"); break;
            case DFPlayerCardOnline: Serial.println("Cartao SD Online!"); break;
            case DFPlayerPlayFinished: 
                Serial.print("Terminou a faixa: "); 
                Serial.println(value); 
                break;
            case DFPlayerError: 
                Serial.print("Erro! Codigo: "); 
                Serial.println(value); 
                break;
            default: break;
        }
    }
}

// -------------------------------------------------------------------------
// INICIALIZAÇÃO PRINCIPAL E LOOP
// -------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    rtcisrunning = rtc.begin();
    
    randomSeed(analogRead(0));

    display_init();
    create_ui();
    lv_refr_now(NULL);
    
    generate_shuffle_queue();
    
    initAudioModule();
    play_track(0, true); 
}

void loop() {
    display_update();
    printDFPlayerStatus(); 
    delay(2);
}