#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_types.h"
#include "lcd_init.h"

// Global panel handle
esp_lcd_panel_handle_t panel_handle = NULL;

// Backlight control setup
void lcd_set_backlight(bool on) {
    gpio_set_level(GPIO_LCD_BACKLIGHT, on ? 1 : 0);
}

// Initialize the backlight GPIO
void init_lcd_backlight(void) {
    gpio_config_t bk_gpio_config = {
        .pin_bit_mask = 1ULL << GPIO_LCD_BACKLIGHT,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&bk_gpio_config);
    lcd_set_backlight(true);  // Turn on backlight
}

// Initialize the RGB LCD panel
void init_lcd_panel(void) {
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = 15000000,       // Pixel clock frequency
            .h_res = LCD_WIDTH,        // Horizontal resolution
            .v_res = LCD_HEIGHT,       // Vertical resolution
            .hsync_back_porch = 43,
            .hsync_front_porch = 8,
            .hsync_pulse_width = 4,
            .vsync_back_porch = 12,
            .vsync_front_porch = 8,
            .vsync_pulse_width = 4,
            .flags.pclk_active_neg = true,
            .flags.hsync_idle_low = true,
        },
        .data_width = 16,                      // 16-bit color depth
        .psram_trans_align = 64,
        .sram_trans_align = 64,
        .hsync_gpio_num = GPIO_LCD_HSYNC,
        .vsync_gpio_num = GPIO_LCD_VSYNC,
        .de_gpio_num = GPIO_LCD_DE,
        .pclk_gpio_num = GPIO_LCD_PCLK,
        .data_gpio_nums = {
            LCD_PIN_D0, LCD_PIN_D1, LCD_PIN_D2, LCD_PIN_D3,
            LCD_PIN_D4, LCD_PIN_D5, LCD_PIN_D6, LCD_PIN_D7,
            LCD_PIN_D8, LCD_PIN_D9, LCD_PIN_D10, LCD_PIN_D11,
            LCD_PIN_D12, LCD_PIN_D13, LCD_PIN_D14, LCD_PIN_D15
        },
        .disp_gpio_num = -1,                   // No display on/off pin
        .flags.fb_in_psram = true,             // Framebuffer in PSRAM
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
}