#include "lvgl.h"
#include "ui.h"

// Create UI elements (buttons, labels, etc.)
void create_ui(void) {

    lv_obj_t *screen = lv_screen_active();
    // lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

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
}
