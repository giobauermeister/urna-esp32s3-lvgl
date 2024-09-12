#include "lvgl.h"
#include "ui.h"
#include "esp_log.h"

static const char* TAG = "UI";

#define NUM_RECTANGLES 4  // Number of rectangles

// Declare an array to hold the rectangle objects
lv_obj_t *rectangles[NUM_RECTANGLES];

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

    ESP_LOGI(TAG, "Updating border opacity: %ld", opacity);

    // Apply the new border opacity to the rectangle
    lv_obj_set_style_border_opa(rect, opacity, 0);
}

void create_row_rectangles(lv_obj_t *parent, int16_t n_rect) {
    int16_t rect_width = 70;
    int16_t rect_height = 100;
    int16_t spacing = 20;
    lv_obj_t *previous_rect = NULL;

    // Create style for rectangles
    static lv_style_t rect_style;
    lv_style_init(&rect_style);
    lv_style_set_border_color(&rect_style, lv_color_black()); // Set border color to black
    lv_style_set_border_width(&rect_style, 4);                // Border width of 4 pixels
    lv_style_set_radius(&rect_style, 5);                      // Set border radius to 5
    lv_style_set_border_opa(&rect_style, LV_OPA_COVER);       // Set border opacity to fully opaque
    lv_style_set_bg_opa(&rect_style, LV_OPA_TRANSP);          // Set background opacity to transparent

    for (uint8_t i = 0; i < n_rect; i++) {
        rectangles[i] = lv_obj_create(parent);

        lv_obj_set_size(rectangles[i], rect_width, rect_height);

        if(i == 0) {
            lv_obj_align(rectangles[i], LV_ALIGN_LEFT_MID, 100, 0); // Position on the screen
        } else {
            lv_obj_align_to(rectangles[i], previous_rect, LV_ALIGN_OUT_RIGHT_MID, spacing, 0);
        }

        lv_obj_add_style(rectangles[i], &rect_style, 0);

        // Update the previous_rect to the current one
        previous_rect = rectangles[i];
    }
}

void blink_rectangle(uint8_t rect_id, bool run) {
    if(run == true) {
        lv_anim_t rect_anim;
        lv_anim_init(&rect_anim);
        lv_anim_set_var(&rect_anim, rectangles[rect_id]);
        lv_anim_set_values(&rect_anim, LV_OPA_COVER, LV_OPA_TRANSP);
        lv_anim_set_duration(&rect_anim, 500);
        lv_anim_set_playback_duration(&rect_anim, 500);
        lv_anim_set_repeat_count(&rect_anim, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_path_cb(&rect_anim, lv_anim_path_ease_in_out);
        lv_anim_set_exec_cb(&rect_anim, anim_border_opacity_cb);
        lv_anim_start(&rect_anim);
    } else {
        lv_anim_del(rectangles[rect_id], NULL);
    }
}

// Create UI elements (buttons, labels, etc.)
void create_ui(void) {

    lv_obj_t *screen = lv_screen_active();
    // lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    // Set screen background to white
    set_screen_background_white(screen);

    // // Create a button
    // lv_obj_t *btn = lv_button_create(screen);
    // lv_obj_set_size(btn, 120, 50);  // Set button size
    // lv_obj_set_pos(btn, 50, 50);    // Position the button

    // // Add a label to the button
    // lv_obj_t *label = lv_label_create(btn);
    // lv_label_set_text(label, "Click Me");
    // lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // // // Create a style and set a larger font size
    // static lv_style_t style;
    // lv_style_init(&style);
    // lv_style_set_text_font(&style, &lv_font_montserrat_30);  // Use a larger built-in font

    // // Create a static label
    // lv_obj_t *txt = lv_label_create(screen);
    // lv_label_set_text(txt, "Hello World!");
    // lv_obj_align(txt, LV_ALIGN_CENTER, 0, 0);
    // lv_obj_add_style(txt, &style, 0);

    create_row_rectangles(screen, NUM_RECTANGLES);

    blink_rectangle(0, true);
    blink_rectangle(1, false);
    blink_rectangle(2, true);
    blink_rectangle(3, false);
}
