/**
 * @file basic_example.cpp
 * @brief wEui库基础使用示例
 *
 * 这个示例展示了wEui库的基本功能：
 * - 初始化wEui系统
 * - 创建简单的菜单列表
 * - 处理按钮事件
 * - 渲染显示
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include "wEui.h"

// ============================================================================
// 硬件配置
// ============================================================================

#define I2C_SDA_PIN   4
#define I2C_SCL_PIN   5
#define BTN_UP_PIN    3
#define BTN_DOWN_PIN  2
#define BTN_OK_PIN    1
#define BTN_BACK_PIN  0

// ============================================================================
// 全局变量
// ============================================================================

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, I2C_SCL_PIN, I2C_SDA_PIN);
QueueHandle_t buttonQueue;
SemaphoreHandle_t listMutex;

// ============================================================================
// 菜单回调函数
// ============================================================================

void onMenuItem1(uint8_t itemIndex) {
    Serial.println("选择了菜单项 1");
}

void onMenuItem2(uint8_t itemIndex) {
    Serial.println("选择了菜单项 2");
}

void onMenuItem3(uint8_t itemIndex) {
    Serial.println("选择了菜单项 3");
}

void onSettings(uint8_t itemIndex) {
    Serial.println("进入设置");
}

void onAbout(uint8_t itemIndex) {
    Serial.println("关于信息");
}

// ============================================================================
// FreeRTOS任务
// ============================================================================

/**
 * @brief UI渲染任务
 */
void vTaskUI(void *pvParameters) {
    for (;;) {
        wEui_render();
        wEui_update();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

/**
 * @brief 按钮扫描任务
 */
void vTaskButtons(void *pvParameters) {
    for (;;) {
        wEui_button_scan();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief GUI服务任务
 */
void vTaskGUIService(void *pvParameters) {
    for (;;) {
        wEui_processButtonEvents(portMAX_DELAY);
    }
}

// ============================================================================
// 主程序
// ============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("wEui基础示例启动中...");

    // 初始化I2C
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // 创建FreeRTOS对象
    buttonQueue = xQueueCreate(10, sizeof(const char*));
    listMutex = xSemaphoreCreateMutex();

    if (buttonQueue == NULL || listMutex == NULL) {
        Serial.println("创建FreeRTOS对象失败!");
        return;
    }

    // 配置wEui
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

    // 初始化wEui
    if (wEui_init(&config) != 0) {
        Serial.println("初始化wEui失败!");
        return;
    }

    // 初始化列表
    wEui_list_init(listMutex);

    // 添加菜单项
    wEui_list_addItem("菜单项 1", onMenuItem1);
    wEui_list_addItem("菜单项 2", onMenuItem2);
    wEui_list_addItem("菜单项 3", onMenuItem3);
    wEui_list_addItem("设置", onSettings);
    wEui_list_addItem("关于", onAbout);

    // 启动UI
    wEui_begin();

    // 创建FreeRTOS任务
    BaseType_t result;
    result = xTaskCreate(vTaskUI, "UI_Task", 2048, NULL, 2, NULL);
    if (result != pdPASS) {
        Serial.println("创建UI任务失败!");
        return;
    }

    result = xTaskCreate(vTaskButtons, "Button_Task", 1024, NULL, 3, NULL);
    if (result != pdPASS) {
        Serial.println("创建按钮任务失败!");
        return;
    }

    result = xTaskCreate(vTaskGUIService, "GUI_Service", 2048, NULL, 1, NULL);
    if (result != pdPASS) {
        Serial.println("创建GUI服务任务失败!");
        return;
    }

    Serial.println("wEui基础示例就绪!");
    Serial.println("使用按钮进行导航:");
    Serial.println("  上/下键: 选择菜单项");
    Serial.println("  确定键: 执行选中项");
    Serial.println("  返回键: 返回上级菜单");
}

void loop() {
    // FreeRTOS处理所有任务
    vTaskDelay(pdMS_TO_TICKS(1000));
}
