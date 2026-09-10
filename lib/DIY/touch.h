#ifndef TOUCH_H
#define TOUCH_H

#include <FT6336.h>

#define TOUCH_FT6336_SCL 22
#define TOUCH_FT6336_SDA 21
#define TOUCH_FT6336_INT 14
#define TOUCH_FT6336_RST 12

// Coordenadas para o toque 1
int touch_last_x = 0;
int touch_last_y = 0;

// Coordenadas para o toque 2
int touch2_last_x = 0;
int touch2_last_y = 0;

uint16_t touch_width = 0;
uint16_t touch_height = 0;

// Chip nativo em 320x480
FT6336 ts = FT6336(TOUCH_FT6336_SDA, TOUCH_FT6336_SCL, TOUCH_FT6336_INT, TOUCH_FT6336_RST, 320, 480);

void touch_init(uint16_t w, uint16_t h, uint8_t r)
{
    touch_width = w;
    touch_height = h;
    ts.begin();
    ts.setRotation(r);
}

// Atualiza os registradores do FT6336
void touch_read_hardware(void)
{
    ts.read();
    if (ts.isTouched && ts.touches > 0)
    {
        // Ponto 1
        touch_last_x = constrain(ts.points[0].x, 0, touch_width - 1);
        touch_last_y = constrain(ts.points[0].y, 0, touch_height - 1);

        // Ponto 2 (se presente)
        if (ts.touches >= 2)
        {
            touch2_last_x = constrain(ts.points[1].x, 0, touch_width - 1);
            touch2_last_y = constrain(ts.points[1].y, 0, touch_height - 1);
        }
    }
}

// Retorna se o primeiro dedo está pressionando
bool touch_touched_1(void)
{
    return ts.isTouched && (ts.touches >= 1);
}

// Retorna se o segundo dedo está pressionando
bool touch_touched_2(void)
{
    return ts.isTouched && (ts.touches >= 2);
}

bool touch_has_signal(void)
{
    return true;
}

#endif