#ifndef SUN_ICONS_H
#define SUN_ICONS_H

#include <lvgl.h>

// A macro LV_IMG_DECLARE exporta a struct 'extern const lv_img_dsc_t'
// permitindo que o arquivo principal use &img_sun_max e &img_sun_min
LV_IMG_DECLARE(img_sun_max);
LV_IMG_DECLARE(img_sun_min);

#endif // SUN_ICONS_H