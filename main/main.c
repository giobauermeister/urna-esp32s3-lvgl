#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_types.h"
#include "esp_heap_caps.h"
#include "../include/lcd_defines.h"
#include "lvgl.h"


// Initialize the RGB panel
esp_lcd_panel_handle_t panel_handle = NULL;

// Your flush callback function
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p) {
    // Transfer the rendered buffer to the actual display
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);

    // Notify LVGL that flushing is done
    lv_display_flush_ready(disp);
}

// Configure display timings (typically found in the display datasheet)
esp_lcd_rgb_panel_config_t panel_config = {
    .clk_src = LCD_CLK_SRC_PLL160M,
    .timings = {
        .pclk_hz = 10 * 1000 * 1000,       // Pixel clock frequency (adjust based on specs)
        .h_res = LCD_WIDTH,                // Horizontal resolution
        .v_res = LCD_HEIGHT,               // Vertical resolution
        .hsync_back_porch = 40,            // Horizontal back porch
        .hsync_front_porch = 40,           // Horizontal front porch
        .hsync_pulse_width = 48,           // Horizontal sync pulse width
        .vsync_back_porch = 13,            // Vertical back porch
        .vsync_front_porch = 32,           // Vertical front porch
        .vsync_pulse_width = 3,            // Vertical sync pulse width
        .flags.pclk_active_neg = true,     // Pixel clock active on the falling edge
    },
    .data_width = 16,                      // 16-bit color depth
    .psram_trans_align = 64,
    .sram_trans_align = 4,
    .hsync_gpio_num = GPIO_LCD_HSYNC,
    .vsync_gpio_num = GPIO_LCD_VSYNC,
    .de_gpio_num = GPIO_LCD_DE,
    .pclk_gpio_num = GPIO_LCD_PCLK,
    .data_gpio_nums = {
        GPIO_LCD_R3,
        GPIO_LCD_R4,
        GPIO_LCD_R5,
        GPIO_LCD_R6,
        GPIO_LCD_R7,
        GPIO_LCD_G2,
        GPIO_LCD_G3,
        GPIO_LCD_G4,
        GPIO_LCD_G5,
        GPIO_LCD_G6,
        GPIO_LCD_G7,
        GPIO_LCD_B3,
        GPIO_LCD_B4,
        GPIO_LCD_B5,
        GPIO_LCD_B6,
        GPIO_LCD_B7,
    },
    .disp_gpio_num = -1,                   // If no display on/off pin, set to -1
    .flags.fb_in_psram = true,             // Framebuffer stored in PSRAM (optional)
};

// Backlight control setup
static void lcd_set_backlight(bool on) {
    gpio_set_level(GPIO_LCD_BACKLIGHT, on ? 1 : 0);
}

void app_main(void)
{
    printf("Hello World!\n");

    // fill_color_bitmap(0xFFFF);  // Fill with white

    // Initialize backlight GPIO
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = 1ULL << GPIO_LCD_BACKLIGHT,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&bk_gpio_config);
    lcd_set_backlight(true);  // Turn on backlight

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    // Initialize the RGB panel
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // Initialize LVGL library
    lv_init();

    // Allocate buffer
    lv_color_t *buf1 = heap_caps_malloc(LCD_WIDTH * 10 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lv_color_t *buf2 = heap_caps_malloc(LCD_WIDTH * 10 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

     if (!buf1 || !buf2) {
        ESP_LOGE("MEMORY", "Failed to allocate buffers");
        return;
    }

    // Create the display
    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);

    // Set flush callback
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    // Set buffers for rendering
    lv_display_set_buffers(disp, buf1, buf2, LCD_WIDTH * 10 * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Create UI elements (e.g., a button)
    // lv_obj_t *btn = lv_button_create(lv_screen_active());  // Create a button on the active screen
    // lv_obj_set_size(btn, 120, 50);                            // Set the button size
    // lv_obj_set_pos(btn, 400, 240);
    // // lv_obj_center(btn);                                       // Center the button

    // Add a label to the button
    // lv_obj_t *label = lv_label_create(btn);
    // lv_label_set_text(label, "Click Me");

    lv_obj_t *txt = lv_label_create(lv_screen_active());
    lv_label_set_text(txt, "Hello World!");
    lv_obj_set_height(txt, 310);
    lv_obj_set_pos(txt, 50, 50);

    // Start handling LVGL tasks
    while (1) {
        lv_timer_handler();     // Let LVGL handle UI updates
        vTaskDelay(pdMS_TO_TICKS(10));  // Delay to avoid overload
    }

    // uint16_t *color_bitmap = heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM);

    // if (color_bitmap == NULL) {
    //     ESP_LOGE("TAG", "Failed to allocate memory in PSRAM");
    //     return;
    // }

    // // Fill the buffer with a solid color (e.g., white)
    // for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
    //     color_bitmap[i] = 0x1D00;  // Example: Fill with white color (RGB565)
    // }

    // // Fill the screen with a solid color (optional, testing purposes)
    // ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, color_bitmap));

    // // Free the memory when done
    // free(color_bitmap);
}