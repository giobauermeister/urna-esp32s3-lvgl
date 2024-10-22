#include "keypad.h"
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "i2c_init.h"
#include "freertos/FreeRTOS.h"
#include "ui.h"

static const char* TAG = "Keypad";

TaskHandle_t keypad_task_handle = NULL;
QueueHandle_t keypad_queue;  // Queue for keys

// Write to PCF8574 function
esp_err_t pcf8574_write(uint8_t data)
{
    esp_err_t ret = i2c_master_transmit(pcf8574_i2c_device_handle, &data, 1, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE("PCF8574", "I2C write failed with error code: 0x%x", ret);
    }
    return ret;
}

// Read from PCF8574 function
esp_err_t pcf8574_read(uint8_t *data)
{
    esp_err_t ret = i2c_master_receive(pcf8574_i2c_device_handle, data, 1, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE("PCF8574", "I2C read failed with error code: 0x%x", ret);
    }
    return ret;
}

void IRAM_ATTR pcf8574_isr_handler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR((TaskHandle_t)arg, 0, eNoAction, &xHigherPriorityTaskWoken);
    if(xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

void setup_interrupt(TaskHandle_t task_handle)
{
    gpio_config_t io_interrupt_cfg = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PCF8574_INT_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };
    gpio_config(&io_interrupt_cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PCF8574_INT_PIN, pcf8574_isr_handler, (void*)task_handle);
}

char scanKeypad()
{
    ESP_LOGI(TAG, "Scanning key...");

    const char keypad_keys[KEYPAD_ROWS][KEYPAD_COLS] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };
    uint8_t row_mask[KEYPAD_ROWS] = {0xFE, 0xFD, 0xFB, 0xF7};
    uint8_t col_state = 0xFF;
    for (int row = 0; row < KEYPAD_ROWS; row++)
    {
        ESP_ERROR_CHECK(pcf8574_write(row_mask[row]));
        ESP_ERROR_CHECK(pcf8574_read(&col_state));
        for (int col = 0; col < KEYPAD_COLS; col++)
        {
            if(!(col_state & (1 << (col + 4)))) {
                return keypad_keys[row][col];
            }
        }        
    }
    return '\0';
}

void keypad_task(void *arg)
{
    ESP_ERROR_CHECK(pcf8574_write(0xF0));
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        char key = scanKeypad();
        ESP_ERROR_CHECK(pcf8574_write(0xF0));
        if(key != '\0') {
            ESP_LOGI(TAG, "Key pressed: %c", key);
            xQueueSend(keypad_queue, &key, portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }    
}

void create_keypad_interrupt_task(void)
{
    keypad_queue = xQueueCreate(10, sizeof(char));
    ESP_LOGI(TAG, "Task created");
    xTaskCreate(keypad_task, "keypad_task", 4096, NULL, 10, &keypad_task_handle);
    setup_interrupt(keypad_task_handle);
}