#ifndef LVGL_INIT_H
#define LVGL_INIT_H

// Function declaration
void init_lvgl(void);
void lvgl_timer_handler_task(void *pvParameter);
void create_lvgl_task(void);

#endif  // LVGL_INIT_H