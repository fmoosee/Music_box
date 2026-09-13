#include "sun_icons.h"

#define _ 0x00
#define X 0xFF

// Sol Máximo (15x15) - Núcleo grande (7x7) e raios longos
const uint8_t sun_max_map[] = {
    _, _, _, _, _, _, _, X, _, _, _, _, _, _, _,
    _, _, _, _, _, _, _, X, _, _, _, _, _, _, _,
    _, _, X, _, _, _, _, _, _, _, _, _, X, _, _,
    _, _, _, X, _, _, _, _, _, _, _, X, _, _, _,
    _, _, _, _, _, X, X, X, X, X, _, _, _, _, _,
    _, _, _, _, X, X, X, X, X, X, X, _, _, _, _,
    _, _, _, _, X, X, X, X, X, X, X, _, _, _, _,
    X, X, _, _, X, X, X, X, X, X, X, _, _, X, X,
    _, _, _, _, X, X, X, X, X, X, X, _, _, _, _,
    _, _, _, _, X, X, X, X, X, X, X, _, _, _, _,
    _, _, _, _, _, X, X, X, X, X, _, _, _, _, _,
    _, _, _, X, _, _, _, _, _, _, _, X, _, _, _,
    _, _, X, _, _, _, _, _, _, _, _, _, X, _, _,
    _, _, _, _, _, _, _, X, _, _, _, _, _, _, _,
    _, _, _, _, _, _, _, X, _, _, _, _, _, _, _
};

// Sol Mínimo (15x15) - Núcleo pequeno (5x5) e raios curtos pontilhados
const uint8_t sun_min_map[] = {
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
    _, _, _, _, _, _, _, X, _, _, _, _, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
    _, _, _, X, _, _, _, _, _, _, _, X, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
    _, _, _, _, _, X, X, X, X, X, _, _, _, _, _,
    _, _, _, _, _, X, X, X, X, X, _, _, _, _, _,
    _, X, _, _, _, X, X, X, X, X, _, _, _, X, _,
    _, _, _, _, _, X, X, X, X, X, _, _, _, _, _,
    _, _, _, _, _, X, X, X, X, X, _, _, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
    _, _, _, X, _, _, _, _, _, _, _, X, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _,
    _, _, _, _, _, _, _, X, _, _, _, _, _, _, _,
    _, _, _, _, _, _, _, _, _, _, _, _, _, _, _
};

#undef _
#undef X

const lv_img_dsc_t img_sun_max = {
    .header = {
        .cf = LV_IMG_CF_ALPHA_8BIT,
        .always_zero = 0,
        .reserved = 0,
        .w = 15, // Aumentado para 15
        .h = 15  // Aumentado para 15
    },
    .data_size = 15 * 15,
    .data = sun_max_map
};

const lv_img_dsc_t img_sun_min = {
    .header = {
        .cf = LV_IMG_CF_ALPHA_8BIT,
        .always_zero = 0,
        .reserved = 0,
        .w = 15, 
        .h = 15
    },
    .data_size = 15 * 15,
    .data = sun_min_map
};