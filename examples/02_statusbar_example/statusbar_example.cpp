/**
 * @file statusbar_example.cpp
 * @brief wEui库状态栏功能完整示例
 *
 * 这个示例演示如何使用wEui库的状态栏功能：
 * - 启用和配置状态栏
 * - 动态更新状态信息
 * - 显示系统状态信息
 * - 状态栏的各种使用场景
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

// 系统状态变量
typedef enum {
    STATE_IDLE,
    STATE_LOADING,
    STATE_CONNECTED,
    STATE_ERROR,
    STATE_BATTERY_LOW
} SystemState_t;

SystemState_t currentState = STATE_IDLE;
uint8_t batteryLevel = 85;
bool isConnected = false;
bool isCharging = false;

// ============================================================================
// 菜单回调函数
// ============================================================================

void onToggleStatusBar(uint8_t itemIndex) {
    if (wEui_statusBar_isEnabled()) {
        wEui_statusBar_setEnabled(false);
        Serial.println("状态栏已禁用");
    } else {
        wEui_statusBar_setEnabled(true);
        wEui_statusBar_setText("状态栏已启用");
        Serial.println("状态栏已启用");
    }
}

void onToggleBorder(uint8_t itemIndex) {
    bool currentBorder = wEui_statusBar_getShowBorder();
    wEui_statusBar_setShowBorder(!currentBorder);

    char statusText[64];
    snprintf(statusText, sizeof(statusText), "边框: %s", !currentBorder ? "开启" : "关闭");
    wEui_statusBar_setText(statusText);

    Serial.printf("状态栏边框: %s\n", !currentBorder ? "开启" : "关闭");
}

void onSimulateStates(uint8_t itemIndex) {
    // 循环切换不同的系统状态
    currentState = (SystemState_t)((currentState + 1) % 5);

    char statusText[64];
    switch (currentState) {
        case STATE_IDLE:
            snprintf(statusText, sizeof(statusText), "状态: 空闲");
            break;
        case STATE_LOADING:
            snprintf(statusText, sizeof(statusText), "状态: 加载中...");
            break;
        case STATE_CONNECTED:
            snprintf(statusText, sizeof(statusText), "状态: 已连接");
            isConnected = true;
            break;
        case STATE_ERROR:
            snprintf(statusText, sizeof(statusText), "状态: 错误!");
            break;
        case STATE_BATTERY_LOW:
            snprintf(statusText, sizeof(statusText), "状态: 电量低!");
            batteryLevel = 15;
            break;
    }

    wEui_statusBar_setText(statusText);
    Serial.printf("模拟状态切换: %s\n", statusText);
}

void onShowSystemInfo(uint8_t itemIndex) {
    uint32_t freeHeap = esp_get_free_heap_size();
    char statusText[64];
    snprintf(statusText, sizeof(statusText), "内存: %lu bytes", freeHeap);
    wEui_statusBar_setText(statusText);

    Serial.printf("系统信息 - 可用内存: %lu bytes\n", freeHeap);
}

void onToggleConnection(uint8_t itemIndex) {
    isConnected = !isConnected;

    char statusText[64];
    snprintf(statusText, sizeof(statusText), "WiFi: %s", isConnected ? "已连接" : "断开");
    wEui_statusBar_setText(statusText);

    Serial.printf("连接状态: %s\n", isConnected ? "已连接" : "断开");
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

/**
 * @brief 动态状态栏更新任务
 */
void vTaskDynamicStatusBar(void *pvParameters) {
    TickType_t lastUpdate = xTaskGetTickCount();
    uint32_t counter = 0;

    for (;;) {
        TickType_t currentTick = xTaskGetTickCount();

        // 每2秒更新一次状态栏
        if ((currentTick - lastUpdate) > pdMS_TO_TICKS(2000)) {
            lastUpdate = currentTick;

            if (wEui_statusBar_isEnabled()) {
                char statusText[64];

                // 根据当前状态显示不同信息
                switch (currentState) {
                    case STATE_IDLE:
                        snprintf(statusText, sizeof(statusText), "运行时间: %lus", counter * 2);
                        break;
                    case STATE_LOADING:
                        snprintf(statusText, sizeof(statusText), "加载中 %s", (counter % 4 == 0) ? "." :
                                                                               (counter % 4 == 1) ? ".." :
                                                                               (counter % 4 == 2) ? "..." : "");
                        break;
                    case STATE_CONNECTED:
                        snprintf(statusText, sizeof(statusText), "在线 | 电量: %d%%", batteryLevel);
                        break;
                    case STATE_ERROR:
                        snprintf(statusText, sizeof(statusText), "错误 #%lu", counter % 100);
                        break;
                    case STATE_BATTERY_LOW:
                        snprintf(statusText, sizeof(statusText), "低电量警告! %d%%", batteryLevel);
                        break;
                }

                wEui_statusBar_setText(statusText);
            }

            counter++;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief 系统监控任务
 */
void vTaskSystemMonitor(void *pvParameters) {
    for (;;) {
        // 模拟电池电量变化
        if (isCharging && batteryLevel < 100) {
            batteryLevel++;
        } else if (!isCharging && batteryLevel > 0) {
            if (xTaskGetTickCount() % pdMS_TO_TICKS(10000) == 0) {
                batteryLevel--;
            }
        }

        // 检查低电量状态
        if (batteryLevel < 20 && currentState != STATE_BATTERY_LOW) {
            currentState = STATE_BATTERY_LOW;
            wEui_statusBar_setText("警告: 电量过低!");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ============================================================================
// 主程序
// ============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("wEui状态栏示例启动中...");

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

    // 启用状态栏
    wEui_statusBar_setEnabled(true);
    wEui_statusBar_setShowBorder(true);
    wEui_statusBar_setText("状态栏示例就绪");

    // 添加菜单项
    wEui_list_addItem("切换状态栏", onToggleStatusBar);
    wEui_list_addItem("切换边框", onToggleBorder);
    wEui_list_addItem("模拟状态", onSimulateStates);
    wEui_list_addItem("系统信息", onShowSystemInfo);
    wEui_list_addItem("连接状态", onToggleConnection);

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

    result = xTaskCreate(vTaskDynamicStatusBar, "StatusBar_Task", 1024, NULL, 1, NULL);
    if (result != pdPASS) {
        Serial.println("创建状态栏任务失败!");
        return;
    }

    result = xTaskCreate(vTaskSystemMonitor, "Monitor_Task", 1024, NULL, 1, NULL);
    if (result != pdPASS) {
        Serial.println("创建监控任务失败!");
        return;
    }

    Serial.println("wEui状态栏示例就绪!");
    Serial.println("功能说明:");
    Serial.println("  切换状态栏: 启用/禁用状态栏显示");
    Serial.println("  切换边框: 显示/隐藏状态栏边框");
    Serial.println("  模拟状态: 循环演示不同系统状态");
    Serial.println("  系统信息: 显示当前系统内存信息");
    Serial.println("  连接状态: 切换WiFi连接状态");
}

void loop() {
    // FreeRTOS处理所有任务
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 定期输出状态信息到串口
    static uint32_t lastPrint = 0;
    uint32_t currentTime = millis();
    if (currentTime - lastPrint > 5000) {
        lastPrint = currentTime;
        Serial.printf("状态栏: %s | 当前状态: %d | 电量: %d%% | 连接: %s\n",
                     wEui_statusBar_isEnabled() ? "启用" : "禁用",
                     currentState,
                     batteryLevel,
                     isConnected ? "是" : "否");
    }
}
