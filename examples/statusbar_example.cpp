/**
 * @file statusbar_example.cpp
 * @brief wEui 底部状态栏功能示例
 *
 * 这个文件展示了如何在您的项目中使用wEui库的底部状态栏功能。
 * 状态栏会始终显示在屏幕底部，用于显示系统状态、电量、时间等信息。
 */

#include "../lib/wEui/include/wEui.h"
#include <Arduino.h>

// ============================================================================
// 示例1: 基本的状态栏使用
// ============================================================================

void example_basic_statusbar(void) {
    // 启用状态栏
    wEui_statusBar_setEnabled(true);

    // 显示静态文本
    wEui_statusBar_setText("System Ready");

    // 如果需要隐藏顶部边界线
    wEui_statusBar_setShowBorder(false);
}

// ============================================================================
// 示例2: 动态更新状态栏（在FreeRTOS任务中）
// ============================================================================

void vTaskDynamicStatusBar(void *pvParameters) {
    // 初始化状态栏
    wEui_statusBar_setEnabled(true);
    wEui_statusBar_setShowBorder(true);

    uint32_t counter = 0;

    for (;;) {
        // 每1秒更新一次状态栏
        vTaskDelay(pdMS_TO_TICKS(1000));

        char statusText[64];
        snprintf(statusText, sizeof(statusText), "Count: %lu", counter++);

        // 设置新的状态文本
        wEui_statusBar_setText(statusText);

        Serial.println(statusText);
    }
}

// ============================================================================
// 示例3: 根据系统状态显示不同的内容
// ============================================================================

typedef enum {
    STATE_IDLE,
    STATE_LOADING,
    STATE_CONNECTED,
    STATE_ERROR,
    STATE_BATTERY_LOW
} SystemState_t;

void updateStatusBarByState(SystemState_t state) {
    char statusText[64];

    switch (state) {
        case STATE_IDLE:
            snprintf(statusText, sizeof(statusText), "Ready");
            break;
        case STATE_LOADING:
            snprintf(statusText, sizeof(statusText), "Loading...");
            break;
        case STATE_CONNECTED:
            snprintf(statusText, sizeof(statusText), "WiFi: Connected");
            break;
        case STATE_ERROR:
            snprintf(statusText, sizeof(statusText), "Error!");
            break;
        case STATE_BATTERY_LOW:
            snprintf(statusText, sizeof(statusText), "Battery Low!");
            break;
    }

    wEui_statusBar_setText(statusText);
}

// ============================================================================
// 示例4: 显示系统信息（如内存、电池、时间等）
// ============================================================================

void vTaskSystemInfoStatusBar(void *pvParameters) {
    wEui_statusBar_setEnabled(true);

    for (;;) {
        // 获取系统信息（示例）
        uint32_t freeHeap = esp_get_free_heap_size();
        uint32_t heapSize = esp_get_heap_size();
        uint8_t heapPercent = (freeHeap * 100) / heapSize;

        char statusText[64];
        snprintf(statusText, sizeof(statusText),
                 "Memory: %d%%", heapPercent);

        wEui_statusBar_setText(statusText);

        // 每2秒更新一次
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// ============================================================================
// 示例5: 条件状态栏更新
// ============================================================================

void vTaskConditionalStatusBar(void *pvParameters) {
    wEui_statusBar_setEnabled(true);

    bool isConnected = false;
    bool isCharging = false;
    uint8_t batteryLevel = 85;

    for (;;) {
        char statusText[64];

        // 根据多个条件组合状态信息
        if (!isConnected) {
            snprintf(statusText, sizeof(statusText), "WiFi: Disconnected");
        } else if (isCharging) {
            snprintf(statusText, sizeof(statusText),
                     "Charging [%d%%]", batteryLevel);
        } else if (batteryLevel < 20) {
            snprintf(statusText, sizeof(statusText),
                     "Low Battery [%d%%]", batteryLevel);
        } else {
            snprintf(statusText, sizeof(statusText),
                     "Battery: %d%%", batteryLevel);
        }

        wEui_statusBar_setText(statusText);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ============================================================================
// 示例6: 在列表选择时更新状态栏
// ============================================================================

void onMenuItemSelected(uint8_t itemIndex) {
    // 获取选中项的名称
    const char* itemName = wEui_list_getItemName(itemIndex);

    // 更新状态栏显示选中项信息
    char statusText[64];
    snprintf(statusText, sizeof(statusText),
             "Selected: %s", itemName);

    wEui_statusBar_setText(statusText);

    Serial.printf("Menu Item: %s\n", itemName);
}

// ============================================================================
// 示例7: 使用状态栏显示进度
// ============================================================================

void vTaskProgressStatusBar(void *pvParameters) {
    wEui_statusBar_setEnabled(true);

    // 模拟某个长期操作的进度
    uint8_t progress = 0;

    for (;;) {
        if (progress < 100) {
            char statusText[64];
            snprintf(statusText, sizeof(statusText),
                     "Progress: %d%%", progress);

            wEui_statusBar_setText(statusText);

            progress += 10;
            vTaskDelay(pdMS_TO_TICKS(500));
        } else {
            wEui_statusBar_setText("Complete!");
            progress = 0;
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }
}

// ============================================================================
// 示例8: 启用/禁用状态栏的逻辑
// ============================================================================

void toggleStatusBar(void) {
    // 检查状态栏当前状态
    if (wEui_statusBar_isEnabled()) {
        // 如果已启用，则禁用
        wEui_statusBar_setEnabled(false);
        Serial.println("Status bar disabled");
    } else {
        // 如果未启用，则启用
        wEui_statusBar_setEnabled(true);
        wEui_statusBar_setText("Status bar enabled");
        Serial.println("Status bar enabled");
    }
}

// ============================================================================
// 完整集成示例：在您的项目中使用
// ============================================================================

/*
在您的main.cpp或任何任务中，您可以这样使用：

void setup() {
    Serial.begin(115200);

    // ... 初始化其他组件 ...

    // 初始化wEui库
    // ... wEui配置代码 ...

    // 启用状态栏
    wEui_statusBar_setEnabled(true);
    wEui_statusBar_setText("Initializing...");

    // 创建一个专门的任务来更新状态栏
    xTaskCreate(vTaskDynamicStatusBar, "StatusBar", 2048, NULL, 1, NULL);

    // 创建其他任务...
}

// 或者在GUI服务任务中更新状态栏：
void vTaskGUIService(void *pvParameters) {
    // 启用状态栏
    wEui_statusBar_setEnabled(true);
    wEui_statusBar_setShowBorder(true);
    wEui_statusBar_setText("Ready");

    TickType_t lastUpdate = xTaskGetTickCount();

    for (;;) {
        // 定期更新状态栏
        TickType_t currentTick = xTaskGetTickCount();
        if ((currentTick - lastUpdate) > pdMS_TO_TICKS(1000)) {
            lastUpdate = currentTick;

            // 更新状态栏文本
            // 例如显示系统信息、电池状态等
            uint32_t freeHeap = esp_get_free_heap_size();
            char statusText[64];
            snprintf(statusText, sizeof(statusText),
                     "Heap: %lu bytes", freeHeap);

            wEui_statusBar_setText(statusText);
        }

        // 处理按钮事件
        wEui_processButtonEvents(pdMS_TO_TICKS(100));

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
*/

