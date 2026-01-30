/**
 * @file wEui_example.cpp
 * @brief wEui Library Usage Example
 *
 * This example demonstrates how to use the wEui library for embedded UI development.
 * It shows list management, button handling, and display rendering.
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include "wEui.h"

// Hardware configuration
#define I2C_SDA_PIN   4
#define I2C_SCL_PIN   5
#define BTN_UP_PIN    3
#define BTN_DOWN_PIN  2
#define BTN_OK_PIN    1
#define BTN_BACK_PIN  0

// Global variables
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ I2C_SCL_PIN, /* data=*/ I2C_SDA_PIN);
QueueHandle_t buttonQueue;
SemaphoreHandle_t listMutex;

// Example callback functions
void onMenuItem1(uint8_t itemIndex) {
    Serial.println("Menu Item 1 Selected");
}

void onMenuItem2(uint8_t itemIndex) {
    Serial.println("Menu Item 2 Selected");
}

void onMenuItem3(uint8_t itemIndex) {
    Serial.println("Menu Item 3 Selected");
}

void onSettings(uint8_t itemIndex) {
    Serial.println("Settings Selected");
}

void onAbout(uint8_t itemIndex) {
    Serial.println("About Selected");
}

// FreeRTOS Tasks
void vTaskUI(void *pvParameters) {
    for (;;) {
        wEui_render();
        wEui_update();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

void vTaskButtons(void *pvParameters) {
    for (;;) {
        wEui_button_scan();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vTaskGUIService(void *pvParameters) {
    for (;;) {
        wEui_processButtonEvents(portMAX_DELAY);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("wEui Example Starting...");

    // Initialize I2C
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // Create FreeRTOS objects
    buttonQueue = xQueueCreate(10, sizeof(const char*));
    listMutex = xSemaphoreCreateMutex();

    // Configure wEui
    wEui_Config_t config = {0};
    config.display = &u8g2;
    config.displayConfig.width = 128;
    config.displayConfig.height = 64;
    config.displayConfig.lineHeight = 12;
    config.displayConfig.font = u8g2_font_6x10_tf;
    config.buttonConfig.upPin = BTN_UP_PIN;
    config.buttonConfig.downPin = BTN_DOWN_PIN;
    config.buttonConfig.okPin = BTN_OK_PIN;
    config.buttonConfig.backPin = BTN_BACK_PIN;
    config.buttonQueue = buttonQueue;
    config.listMutex = listMutex;

    // Initialize wEui
    if (wEui_init(&config) != 0) {
        Serial.println("Failed to initialize wEui!");
        return;
    }

    // Initialize list
    wEui_list_init(listMutex);

    // Add menu items
    wEui_list_addItem("Menu Item 1", onMenuItem1);
    wEui_list_addItem("Menu Item 2", onMenuItem2);
    wEui_list_addItem("Menu Item 3", onMenuItem3);
    wEui_list_addItem("Settings", onSettings);
    wEui_list_addItem("About", onAbout);

    // Start UI
    wEui_begin();

    // Create FreeRTOS tasks
    xTaskCreate(vTaskUI, "UI_Task", 2048, NULL, 2, NULL);
    xTaskCreate(vTaskButtons, "Button_Task", 1024, NULL, 3, NULL);
    xTaskCreate(vTaskGUIService, "GUI_Service", 2048, NULL, 1, NULL);

    Serial.println("wEui Example Ready!");
}

void loop() {
    // FreeRTOS handles the tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
}
