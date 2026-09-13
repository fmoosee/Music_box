#include <Arduino.h>
#include "display_driver.h"
#include <RTClib.h>
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>
#include "sun_icons.h"


RTC_DS1307 rtc; // Instância da biblioteca RTC
// Caracteres Unicode UTF-8 de Sol

typedef struct {
    const char* name;      
    const char* art_name;   
    int music_time;         
    int index_pos;          
} music_data;

int audio_volume = 15;

music_data playlist[] = {
    {"Back in Black",   "AC/DC",         255, 1},
    {"The Trooper",     "Iron Maiden",   250, 2},
    {"Master of Puppets","Metallica",    515, 3},
    {"Smoke on the Water","Deep Purple", 340, 4},
    {"Paranoid",        "Black Sabbath", 168, 5}
};

lv_obj_t* menuScr; // Tela principal
char buf[] = "DD/MM/YY - hh:mm";

lv_timer_t * data_att;
lv_obj_t * label_data;

bool button_play_pause_state = true; // Estado do botão play/stop
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
bool       status_repeat = false;
bool       status_random = false;

lv_obj_t * bar_vol;
lv_obj_t * icon_bar_vol_plus, * icon_bar_vol_minus;
lv_obj_t * bar_bright;
lv_obj_t * icon_bright_max;
lv_obj_t * icon_bright_min;
int        light_level = 100;
lv_obj_t * label_light;

lv_obj_t * list_music;
lv_obj_t * icon_ble, * icon_wifi;

// Defina o pino do backlight (caso ainda não esteja definido no seu display_driver.h)
#ifndef PWM_LIGHT
#define PWM_LIGHT 26       // O pino GPIO do ESP32 conectado ao pino LED do display
#endif

#define PWM_CHANNEL 0      // Canal interno do gerador PWM no ESP32 (0 a 15)
#define PWM_FREQ 300     // Frequência de 5 kHz (estabiliza o consumo de corrente)
#define PWM_RESOLUTION 8   // Resolução de 8 bits (permite valores de 0 a 255)

void set_display_brightness(int percent) {
    // Trava de segurança: Garante que o usuário não ultrapasse os limites visíveis
    if (percent < 10) percent = 10;
    if (percent > 100) percent = 100;

    // Mapeamento matemático proporcional:
    // Converte a porcentagem gráfica (10% a 100%) para a escala física do PWM (30 a 255)
    int pwm_val = map(percent, 10, 100, 30, 255);

    // Aplica o sinal PWM ao canal de hardware (substitui o analogWrite)
    ledcWrite(PWM_CHANNEL, pwm_val);
}

void play_track(const music_data &track) {
    Serial.println("\n--- CARREGANDO FAIXA ---");
    Serial.printf("Nome:      %s\n", track.name);
    Serial.printf("Artista:   %s\n", track.art_name);
    Serial.printf("Duracao:   %d segundos\n", track.music_time);
    Serial.printf("Indice SD: %d\n", track.index_pos);

    // 1. Atualiza os títulos na tela
    lv_label_set_text(label_song_name, track.name);
    lv_label_set_text(label_artist_name, track.art_name);

    // 2. Formata a duração máxima em minutos e segundos (mm:ss)
    int min = track.music_time / 60;
    int sec = track.music_time % 60;
    lv_label_set_text_fmt(label_song_time_max, "%d:%02d", min, sec);

    // 3. Reinicia o contador de tempo atual e a barra de progresso
    lv_label_set_text(label_song_time_now, "0:00");
    lv_bar_set_range(bar_prog_mus, 0, track.music_time); // Limite da barra = duração total
    lv_bar_set_value(bar_prog_mus, 0, LV_ANIM_OFF);

    // 4. Atualiza o ícone e o estado do botão Play/Pause para "TOCANDO"
    button_play_pause_state = true;
    lv_img_set_src(img_play_pause, LV_SYMBOL_PAUSE);

    // 5. Envia o comando serial para o módulo DFPlayer Mini tocar a faixa
    //myDFPlayer.play(track.index_pos);
}

static void song_item_clicked_cb(lv_event_t * e) {
    // Recupera o ponteiro da struct associada a este botao
    const music_data * track = (const music_data *)lv_event_get_user_data(e);

    if (track == NULL) return;

    // Como 'name' e 'art_name' sao const char*, passamos a variavel direta sem .c_str()
    Serial.println("\n--- FAIXA SELECIONADA ---");
    Serial.printf("Nome:     %s\n", track->name);
    Serial.printf("Artista:  %s\n", track->art_name);
    Serial.printf("Duracao:  %d segundos\n", track->music_time);
    Serial.printf("Indice SD:%d\n", track->index_pos);

    // Atualiza os labels principais da tela
    lv_label_set_text(label_song_name, track->name);
    lv_label_set_text(label_artist_name, track->art_name);
    
    // Formata o tempo maximo em mm:ss
    int min = track->music_time / 60;
    int sec = track->music_time % 60;
    lv_label_set_text_fmt(label_song_time_max, "%d:%02d", min, sec);
    lv_label_set_text(label_song_time_now, "0:00");
    lv_bar_set_value(bar_prog_mus, 0, LV_ANIM_OFF);

    // Toca a faixa selecionada no DFPlayer Mini
    //myDFPlayer.play(track->index_pos);
    button_play_pause_state = true;
    lv_img_set_src(img_play_pause, LV_SYMBOL_PAUSE);
}

lv_obj_t * add_audio_list_item(lv_obj_t * parent, const music_data * song) {
    // Cria o botao do item na lista
    lv_obj_t * btn = lv_list_add_btn(parent, NULL, NULL);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, 45);
    lv_obj_set_style_pad_left(btn, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_right(btn, 6, LV_PART_MAIN);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    // Container do icone composto
    lv_obj_t * icon_box = lv_obj_create(btn);
    lv_obj_set_size(icon_box, 30, 32);
    lv_obj_set_style_bg_opa(icon_box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(icon_box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(icon_box, 0, LV_PART_MAIN);
    lv_obj_clear_flag(icon_box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(icon_box, LV_ALIGN_LEFT_MID, 0, 0);

    // Simbolo base: Folha de Arquivo
    lv_obj_t * icon_file = lv_label_create(icon_box);
    lv_label_set_text(icon_file, LV_SYMBOL_FILE);
    lv_obj_set_style_text_font(icon_file, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon_file, lv_color_hex(0x9E9E9E), LV_PART_MAIN);
    lv_obj_center(icon_file);

    // Simbolo interno: Nota de Audio
    lv_obj_t * icon_audio = lv_label_create(icon_box);
    lv_label_set_text(icon_audio, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(icon_audio, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon_audio, lv_color_hex(0x101010), LV_PART_MAIN);
    lv_obj_align(icon_audio, LV_ALIGN_CENTER, 0, 2);

    // Label do titulo da musica
    lv_obj_t * label_title = lv_label_create(btn);
    lv_label_set_text(label_title, song->name); // Passa o ponteiro puro diretamente
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    // Texto com rolagem suave
    lv_label_set_long_mode(label_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label_title, lv_pct(85));
    lv_obj_align_to(label_title, icon_box, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
    lv_obj_set_style_anim_speed(label_title, 20, LV_PART_MAIN);

    // Vincula o evento de clique com o ponteiro do struct como user_data
    lv_obj_add_event_cb(btn, song_item_clicked_cb, LV_EVENT_CLICKED, (void *)song);

    return btn;
}

void btn_play_pause_cb(lv_event_t * e) {
    button_play_pause_state = !button_play_pause_state;
    lv_img_set_src(img_play_pause, button_play_pause_state ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
}

void att_timer(lv_timer_t * timer){
    DateTime now = rtc.now();
    lv_label_set_text(label_data, rtc.begin() ? now.toString(buf) : "--/--/-- - --:--");
}

static void btn_volume_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    int step = (int)(intptr_t)lv_event_get_user_data(e);
    if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
        audio_volume += step;
        if (audio_volume > 30) audio_volume = 30;
        if (audio_volume < 0)  audio_volume = 0;
        lv_bar_set_value(bar_vol, audio_volume, LV_ANIM_ON);
        LV_LOG_INFO("Audio Modified %02d", audio_volume);
        //myDFPlayer.volume(audio_volume)
    }
}
static void btn_repeat_cb(lv_event_t * e){
    status_repeat = !status_repeat;
    lv_obj_set_style_opa(icon_repeat, status_repeat ? LV_OPA_100 : LV_OPA_30, LV_PART_MAIN);
    LV_LOG_INFO("Status repeat %s", status_repeat ? "Ativado" : "Desativado");
}
static void btn_random_cb(lv_event_t * e){
    status_random = !status_random;
    lv_obj_set_style_opa(icon_random, status_random ? LV_OPA_100 : LV_OPA_30, LV_PART_MAIN);
    LV_LOG_INFO("Musica Aleatória %s", status_random ? "Ativado" : "Desativado");
}

void create_ui() {
    // =========================================================================
    // 1. TELA PRINCIPAL (Fundo)
    // =========================================================================
    menuScr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(menuScr, lv_color_hex(0x1E1E1E), LV_PART_MAIN);
    lv_scr_load(menuScr);


    // =========================================================================
    // 2. BARRA SUPERIOR (Conectividade e Relógio)
    // =========================================================================
    icon_wifi = lv_img_create(menuScr);
    lv_img_set_src(icon_wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_opa(icon_wifi, LV_OPA_50, LV_PART_MAIN);
    lv_obj_align(icon_wifi, LV_ALIGN_TOP_LEFT, 5, 5);

    icon_ble = lv_img_create(menuScr);
    lv_img_set_src(icon_ble, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_opa(icon_ble, LV_OPA_50, LV_PART_MAIN);
    lv_obj_align_to(icon_ble, icon_wifi, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    label_data = lv_label_create(menuScr);
    lv_obj_set_style_text_font(label_data, &lv_font_unscii_8, LV_PART_MAIN);
    
    // Carga inicial do relógio para garantir o alinhamento correto da barra de brilho
    DateTime now = rtc.now();
    lv_label_set_text(label_data, rtc.begin() ? now.toString(buf) : "--/--/-- - --:--");
    lv_obj_align_to(label_data, icon_ble, LV_ALIGN_OUT_RIGHT_MID, 6, 0);
    
    // Força o cálculo do tamanho do texto na tela imediatamente
    lv_obj_update_layout(label_data); 
    
    data_att = lv_timer_create(att_timer, 30000, NULL);


    // =========================================================================
    // 3. BARRA SUPERIOR (Controle de Brilho Degradê Azul)
    // =========================================================================
    // Ícone Sol Mínimo
    icon_bright_min = lv_img_create(menuScr);
    lv_img_set_src(icon_bright_min, &img_sun_min); // Referência ao bitmap 15x15
    lv_obj_set_style_img_recolor(icon_bright_min, lv_color_hex(0x1976D2), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon_bright_min, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_opa(icon_bright_min, LV_OPA_70, LV_PART_MAIN);
    lv_obj_align_to(icon_bright_min, label_data, LV_ALIGN_OUT_RIGHT_MID, 16, 0);

    // Slider de Brilho
    bar_bright = lv_slider_create(menuScr);
    lv_obj_set_size(bar_bright, 75, 6);
    lv_slider_set_range(bar_bright, 10, 100);
    lv_slider_set_value(bar_bright, light_level, LV_ANIM_OFF);
    lv_obj_set_ext_click_area(bar_bright, 10); // Expande área de toque
    
    // Trilho (Fundo Escuro Azulado)
    lv_obj_set_style_bg_color(bar_bright, lv_color_hex(0x222B36), LV_PART_MAIN);
    lv_obj_set_style_radius(bar_bright, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    
    // Preenchimento (Degradê Azul Escuro -> Azul Claro Ciano)
    lv_obj_set_style_bg_grad_dir(bar_bright, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar_bright, lv_color_hex(0x0D47A1), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(bar_bright, lv_color_hex(0x00E5FF), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar_bright, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    
    // Knob Circular (Puxador)
    lv_obj_set_style_bg_color(bar_bright, lv_color_hex(0x00E5FF), LV_PART_KNOB);
    lv_obj_set_style_pad_all(bar_bright, 2, LV_PART_KNOB);
    
    // ESPAÇAMENTO DE 15px PARA EVITAR COLISÃO DE ÁREA DE DESENHO (Bounding Box)
    lv_obj_align_to(bar_bright, icon_bright_min, LV_ALIGN_OUT_RIGHT_MID, 15, 0);

    // Ícone Sol Máximo
    icon_bright_max = lv_img_create(menuScr);
    lv_img_set_src(icon_bright_max, &img_sun_max); // Referência ao bitmap 15x15
    lv_obj_set_style_img_recolor(icon_bright_max, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(icon_bright_max, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_opa(icon_bright_max, LV_OPA_100, LV_PART_MAIN);
    
    // ESPAÇAMENTO DE 15px
    lv_obj_align_to(icon_bright_max, bar_bright, LV_ALIGN_OUT_RIGHT_MID, 15, 0);

    // Porcentagem do Brilho
    label_light = lv_label_create(menuScr);
    lv_label_set_text_fmt(label_light, "%d%%", light_level);
    lv_obj_set_style_text_font(label_light, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_light, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_align_to(label_light, icon_bright_max, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    // Evento do Brilho (Altera o texto e a variável interna)
    lv_obj_add_event_cb(bar_bright, [](lv_event_t * e) {
        lv_obj_t * slider = lv_event_get_target(e);
        lv_obj_t * label  = (lv_obj_t *)lv_event_get_user_data(e);
        if (label != NULL) {
            // Atualiza a variável global
            light_level = (int)lv_slider_get_value(slider);
            // Atualiza o texto na tela ("80%")
            lv_label_set_text_fmt(label, "%d%%", light_level);
            // >>> NOVA CHAMADA <<<
            // Envia o novo valor em tempo real para o hardware
            set_display_brightness(light_level);
        }
    }, LV_EVENT_VALUE_CHANGED, (void *)label_light);


    // =========================================================================
    // 4. LATERAL ESQUERDA (Slider de Volume)
    // =========================================================================
    bar_vol = lv_bar_create(menuScr);
    lv_obj_set_size(bar_vol, 5, 100);
    lv_bar_set_range(bar_vol, 0, 30);
    lv_bar_set_value(bar_vol, audio_volume, LV_ANIM_ON);
    lv_obj_set_style_bg_color(bar_vol, lv_color_hex(0x484848), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar_vol, lv_color_hex(0x00E5FF), LV_PART_INDICATOR);
    lv_obj_align(bar_vol, LV_ALIGN_TOP_LEFT, 10, 65);

    icon_bar_vol_plus = lv_img_create(menuScr);
    lv_img_set_src(icon_bar_vol_plus, LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_opa(icon_bar_vol_plus, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align_to(icon_bar_vol_plus, bar_vol, LV_ALIGN_OUT_TOP_MID, 0, -5);

    icon_bar_vol_minus = lv_img_create(menuScr);
    lv_img_set_src(icon_bar_vol_minus, LV_SYMBOL_MUTE);
    lv_obj_set_style_opa(icon_bar_vol_minus, LV_OPA_80, LV_PART_MAIN);
    lv_obj_align_to(icon_bar_vol_minus, bar_vol, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);


    // =========================================================================
    // 5. PAINEL CENTRAL (Info da Música e Progresso)
    // =========================================================================
    label_song_name = lv_label_create(menuScr);
    lv_obj_set_width(label_song_name, 220);
    lv_obj_set_style_text_font(label_song_name, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_obj_set_style_text_align(label_song_name, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_label_set_long_mode(label_song_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_align(label_song_name, LV_ALIGN_LEFT_MID, 70, -40);

    label_artist_name = lv_label_create(menuScr);
    lv_obj_set_width(label_artist_name, 240);
    lv_obj_set_style_text_align(label_artist_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
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


    // =========================================================================
    // 6. CONTROLES DE MÍDIA (Botões Centrais)
    // =========================================================================
    // Play / Pause
    button_play_pause = lv_btn_create(menuScr);
    lv_obj_set_size(button_play_pause, 40, 40);
    lv_obj_set_style_radius(button_play_pause, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align_to(button_play_pause, bar_prog_mus, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    img_play_pause = lv_img_create(button_play_pause);
    lv_img_set_src(img_play_pause, LV_SYMBOL_PLAY);
    lv_obj_center(img_play_pause);
    lv_obj_add_event_cb(button_play_pause, btn_play_pause_cb, LV_EVENT_CLICKED, NULL);

    // Anterior
    btn_previous = lv_btn_create(menuScr);
    lv_obj_set_size(btn_previous, 30, 30);
    lv_obj_set_style_radius(btn_previous, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align_to(btn_previous, button_play_pause, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    icon_previous = lv_img_create(btn_previous);
    lv_img_set_src(icon_previous, LV_SYMBOL_PREV);
    lv_obj_center(icon_previous);

    // Próximo
    btn_next = lv_btn_create(menuScr);
    lv_obj_set_size(btn_next, 30, 30);
    lv_obj_set_style_radius(btn_next, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align_to(btn_next, button_play_pause, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    icon_next = lv_img_create(btn_next);
    lv_img_set_src(icon_next, LV_SYMBOL_NEXT);
    lv_obj_center(icon_next);

    // Diminuir Volume
    btn_vol_minus = lv_btn_create(menuScr);
    lv_obj_set_size(btn_vol_minus, 30, 30);
    lv_obj_set_style_radius(btn_vol_minus, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align_to(btn_vol_minus, btn_previous, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    icon_vol_minus = lv_img_create(btn_vol_minus);
    lv_img_set_src(icon_vol_minus, LV_SYMBOL_VOLUME_MID);
    lv_obj_center(icon_vol_minus);
    lv_obj_add_event_cb(btn_vol_minus, btn_volume_cb, LV_EVENT_SHORT_CLICKED, (void *)(intptr_t)-1);
    lv_obj_add_event_cb(btn_vol_minus, btn_volume_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)-1);

    // Aumentar Volume
    btn_vol_plus = lv_btn_create(menuScr);
    lv_obj_set_size(btn_vol_plus, 30, 30);
    lv_obj_set_style_radius(btn_vol_plus, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_align_to(btn_vol_plus, btn_next, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    icon_vol_plus = lv_img_create(btn_vol_plus);
    lv_img_set_src(icon_vol_plus, LV_SYMBOL_VOLUME_MAX);
    lv_obj_center(icon_vol_plus);
    lv_obj_add_event_cb(btn_vol_plus, btn_volume_cb, LV_EVENT_SHORT_CLICKED, (void *)(intptr_t)1);
    lv_obj_add_event_cb(btn_vol_plus, btn_volume_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)1);


    // =========================================================================
    // 7. MODIFICADORES (Random e Repeat) - Botões Transparentes
    // =========================================================================
    btn_random = lv_btn_create(menuScr);
    lv_obj_set_size(btn_random, 45, 45);
    
    // Removemos os fundos e as sombras para ficar apenas o ícone clicável
    lv_obj_set_style_bg_opa(btn_random, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn_random, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(btn_random, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(btn_random, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_shadow_opa(btn_random, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(btn_random, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_align_to(btn_random, btn_previous, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    
    icon_random = lv_img_create(btn_random);
    lv_img_set_src(icon_random, LV_SYMBOL_SHUFFLE);
    lv_obj_set_style_opa(icon_random, LV_OPA_30, LV_PART_MAIN); // Inicia escurecido (Desligado)
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
    lv_obj_set_style_opa(icon_repeat, LV_OPA_30, LV_PART_MAIN); // Inicia escurecido (Desligado)
    lv_obj_center(icon_repeat);
    lv_obj_add_event_cb(btn_repeat, btn_repeat_cb, LV_EVENT_CLICKED, NULL);


    // =========================================================================
    // 8. LATERAL DIREITA (Lista de Músicas)
    // =========================================================================
    list_music = lv_list_create(menuScr);
    lv_obj_set_size(list_music, 110, 320);
    lv_obj_set_style_radius(list_music, 0, LV_PART_MAIN);
    lv_obj_align(list_music, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_list_add_text(list_music, "Musicas Salvas:");
    for(size_t i = 0; i < (sizeof(playlist)/sizeof(playlist[0])); i++) {
        add_audio_list_item(list_music, &playlist[i]);
    }
}


void setup() {
    Serial.begin(115200);
    rtc.begin();

    // 1. Configura o gerador de PWM de hardware do ESP32
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

    // 2. Vincula o pino físico ao canal PWM configurado
    ledcAttachPin(PWM_LIGHT, PWM_CHANNEL);

    // 3. Aplica o brilho inicial (ex: 50%) ANTES de iniciar a tela gráficamente.
    // Isso impede que o display inicie piscando ou com brilho máximo indesejado.
    set_display_brightness(light_level);

    // Inicialização do restante do sistema
    display_init();
    create_ui();
    lv_refr_now(NULL);
    play_track(playlist[0]);
}
void loop()
{
    display_update();
    delay(2);
}