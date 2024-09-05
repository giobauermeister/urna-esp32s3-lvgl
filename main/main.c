#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_types.h"
#include "esp_heap_caps.h"
#include "lvgl.h"

// Define GPIOs for the RGB interface (adjust based on your schematic)
#define GPIO_LCD_VSYNC      41
#define GPIO_LCD_HSYNC      39
#define GPIO_LCD_DE         40
#define GPIO_LCD_PCLK       0
#define GPIO_LCD_BACKLIGHT  2

#define GPIO_LCD_R3         45
#define GPIO_LCD_R4         48
#define GPIO_LCD_R5         47
#define GPIO_LCD_R6         21
#define GPIO_LCD_R7         14

#define GPIO_LCD_G2         5
#define GPIO_LCD_G3         6
#define GPIO_LCD_G4         7
#define GPIO_LCD_G5         15
#define GPIO_LCD_G6         16
#define GPIO_LCD_G7         4

#define GPIO_LCD_B3         8
#define GPIO_LCD_B4         3
#define GPIO_LCD_B5         46
#define GPIO_LCD_B6         9
#define GPIO_LCD_B7         1

#define LCD_WIDTH  800
#define LCD_HEIGHT 480

// // Define a solid color in RGB565 format (0xFFFF represents white)
// uint16_t color_bitmap[LCD_WIDTH * LCD_HEIGHT];

// // Fill the buffer with a single color (white in this case)
// void fill_color_bitmap(uint16_t color) {
//     for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
//         color_bitmap[i] = color;  // Fill each pixel with the same color
//     }
// }

// Declare the display driver flush function for LVGL
// static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p);

// LVGL flush callback for drawing to the display
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    // Transfer the LVGL rendered content to the display's framebuffer
    // esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);

    // Inform LVGL that flushing is done
    lv_disp_flush_ready(drv);
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

    // Initialize the RGB panel
    esp_lcd_panel_handle_t panel_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    // Initialize the RGB panel
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    uint16_t *color_bitmap = heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t), MALLOC_CAP_SPIRAM);

    if (color_bitmap == NULL) {
        ESP_LOGE("TAG", "Failed to allocate memory in PSRAM");
        return;
    }

    // Fill the buffer with a solid color (e.g., white)
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        color_bitmap[i] = 0x1D00;  // Example: Fill with white color (RGB565)
    }

    // Fill the screen with a solid color (optional, testing purposes)
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, color_bitmap));

    // Free the memory when done
    free(color_bitmap);
}
