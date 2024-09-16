#ifndef KEYPAD_H 
#define KEYPAD_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

#define PCF8574_INT_PIN GPIO_NUM_38  // Interrupt pin for PCF8574

#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4

extern TaskHandle_t keypad_task_handle;
extern QueueHandle_t keypad_queue;

void create_keypad_interrupt_task(void);

#endif // KEYPAD_H