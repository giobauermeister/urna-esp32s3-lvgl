#include "lvgl.h"
#include "lcd_init.h"
#include "lvgl_init.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LVGL_TICK_PERIOD_MS 1

// LVGL flush callback
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p) {
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_p);
    lv_display_flush_ready(disp);
}

static void lvgl_port_tick_increment(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

static esp_err_t lvgl_port_tick_init(void)
{
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lvgl_port_tick_increment,
        .name = "LVGL tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    return esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000);
}

// LVGL task function
void lvgl_timer_handler_task(void *pvParameter)
{
    while (1) {
        lv_timer_handler();   // Let LVGL handle UI updates
        vTaskDelay(pdMS_TO_TICKS(10));  // Delay to avoid overload (adjust delay as needed)
    }
}

// Initialize LVGL and setup display buffers
void init_lvgl(void) {
    lv_init();  // Initialize the LVGL library

    ESP_ERROR_CHECK(lvgl_port_tick_init());

    // Create the LVGL display
    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);

    // Set flush callback for rendering
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    // Allocate draw buffers in PSRAM
    size_t draw_buffer_sz = LCD_WIDTH * LCD_HEIGHT * lv_color_format_get_size(lv_display_get_color_format(disp));
    void *buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
    void *buf2 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM);
    assert(buf1 && buf2);  // Ensure buffers are allocated

    // Set display buffers
    lv_display_set_buffers(disp, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_FULL);
}

void create_lvgl_task(void)
{
    // Create a task for LVGL handling
    xTaskCreate(lvgl_timer_handler_task, "LVGL Task", 4096, NULL, 5, NULL);
}

