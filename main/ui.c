#include "lvgl.h"
#include "ui.h"

void set_screen_background_white(lv_obj_t *screen) {
    static lv_style_t screen_style;
    lv_style_init(&screen_style);

    // Set background color to white
    lv_style_set_bg_color(&screen_style, lv_color_white());
    lv_style_set_bg_opa(&screen_style, LV_OPA_COVER);

    // Apply style to screen
    lv_obj_add_style(screen, &screen_style, 0);
}

// Function to update the border opacity during the animation
void anim_border_opacity_cb(void *rect_obj, int32_t opacity) {
    lv_obj_t *rect = (lv_obj_t *)rect_obj;

    // Apply the new border opacity to the rectangle
    lv_obj_set_style_border_opa(rect, opacity, 0);
}

// Create UI elements (buttons, labels, etc.)
void create_ui(void) {

    lv_obj_t *screen = lv_screen_active();
    // lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    // Set screen background to white
    set_screen_background_white(screen);

    // Create a button
    lv_obj_t *btn = lv_button_create(screen);
    lv_obj_set_size(btn, 120, 50);  // Set button size
    lv_obj_set_pos(btn, 50, 50);    // Position the button

    // Add a label to the button
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Click Me");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // // Create a style and set a larger font size
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_font(&style, &lv_font_montserrat_30);  // Use a larger built-in font

    // Create a static label
    lv_obj_t *txt = lv_label_create(screen);
    lv_label_set_text(txt, "Hello World!");
    lv_obj_align(txt, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(txt, &style, 0);

    // Create a simple object (rectangle)
    lv_obj_t *rect = lv_obj_create(screen);

    // Set the size and position of the rectangle
    lv_obj_set_size(rect, 80, 120);  // Width and height
    lv_obj_align(rect, LV_ALIGN_LEFT_MID, 50, 0); // Position on the screen

    static lv_style_t rect_style;
    lv_style_init(&rect_style);

    // Set the border properties (color, width)
    lv_style_set_border_color(&rect_style, lv_color_black()); // Set border color to black
    lv_style_set_border_width(&rect_style, 4);                // Border width of 5 pixels
    lv_style_set_radius(&rect_style, 5);
    lv_style_set_border_opa(&rect_style, LV_OPA_COVER);       // Set border opacity to fully opaque

    lv_style_set_bg_opa(&rect_style, LV_OPA_TRANSP);

    lv_obj_add_style(rect, &rect_style, 0);

    lv_anim_t rect_anim;
    lv_anim_init(&rect_anim);
    lv_anim_set_var(&rect_anim, rect);
    lv_anim_set_values(&rect_anim, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&rect_anim, 500);
    lv_anim_set_playback_time(&rect_anim, 500);
    lv_anim_set_repeat_delay(&rect_anim, 10);
    lv_anim_set_repeat_count(&rect_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&rect_anim, anim_border_opacity_cb);
    lv_anim_start(&rect_anim);
}
