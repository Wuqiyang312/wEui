/**
 * @file complete_example.cpp
 * @brief wEui库完整应用示例
 *
 * 这是一个综合展示wEui库所有功能的完整应用示例：
 * - 多级页面管理
 * - 动态状态栏
 * - 系统监控
 * - 设置管理
 * - 实际使用场景演示
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

// 传感器模拟引脚（实际应用中替换为真实传感器）
#define TEMP_SENSOR_PIN A0
#define LIGHT_SENSOR_PIN A1

// ============================================================================
// 全局变量和结构体
// ============================================================================

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, I2C_SCL_PIN, I2C_SDA_PIN);
QueueHandle_t buttonQueue;
SemaphoreHandle_t listMutex;

// 页面ID
int g_mainMenuPageId = -1;
int g_sensorsPageId = -1;
int g_settingsPageId = -1;
int g_monitorPageId = -1;
int g_aboutPageId = -1;
int g_wifiSettingsPageId = -1;
int g_systemPageId = -1;

// 系统状态
typedef struct {
    float temperature;
    uint16_t lightLevel;
    uint32_t uptime;
    uint32_t freeHeap;
    bool wifiEnabled;
    bool bluetoothEnabled;
    uint8_t batteryLevel;
    bool isCharging;
    uint8_t brightness;
    bool soundEnabled;
    char deviceName[32];
    bool autoUpdate;
} SystemState_t;

SystemState_t g_systemState = {
    .temperature = 25.0f,
    .lightLevel = 512,
    .uptime = 0,
    .freeHeap = 0,
    .wifiEnabled = true,
    .bluetoothEnabled = false,
    .batteryLevel = 85,
    .isCharging = false,
    .brightness = 80,
    .soundEnabled = true,
    .autoUpdate = true
};

// 初始化设备名称
void initSystemState() {
    strcpy(g_systemState.deviceName, "wEui Demo Device");
}

// ============================================================================
// 传感器数据模拟
// ============================================================================

void updateSensorData() {
    // 模拟温度传感器 (20-30度)
    static float tempOffset = 0;
    tempOffset += (random(-10, 11) / 100.0f);
    if (tempOffset > 3.0f) tempOffset = 3.0f;
    if (tempOffset < -3.0f) tempOffset = -3.0f;
    g_systemState.temperature = 25.0f + tempOffset;

    // 模拟光照传感器 (0-1023)
    g_systemState.lightLevel = random(100, 900);

    // 更新系统信息
    g_systemState.uptime = millis() / 1000;
    g_systemState.freeHeap = esp_get_free_heap_size();

    // 模拟电池电量变化
    if (g_systemState.isCharging && g_systemState.batteryLevel < 100) {
        if (random(0, 100) < 10) g_systemState.batteryLevel++;
    } else if (!g_systemState.isCharging && g_systemState.batteryLevel > 0) {
        if (random(0, 1000) < 5) g_systemState.batteryLevel--;
    }
}

// ============================================================================
// 自定义页面渲染函数
// ============================================================================

/**
 * @brief 传感器监控页面渲染
 */
void sensorsPageRender(U8G2 *display, const wEui_DisplayConfig_t *displayConfig, uint8_t contentHeight) {
    display->setDrawColor(1);
    display->setFont(displayConfig->font);

    // 标题
    display->setCursor(4, 12);
    display->print("传感器数据");

    // 分割线
    display->drawHLine(4, 15, displayConfig->width - 8);

    // 温度
    display->setCursor(4, 28);
    display->printf("温度: %.1f°C", g_systemState.temperature);

    // 温度条形图
    int tempBarWidth = (int)((g_systemState.temperature - 20.0f) / 10.0f * 60);
    if (tempBarWidth < 0) tempBarWidth = 0;
    if (tempBarWidth > 60) tempBarWidth = 60;
    display->drawFrame(64, 22, 62, 8);
    display->drawBox(65, 23, tempBarWidth, 6);

    // 光照
    display->setCursor(4, 40);
    display->printf("光照: %d", g_systemState.lightLevel);

    // 光照条形图
    int lightBarWidth = g_systemState.lightLevel * 60 / 1023;
    display->drawFrame(64, 34, 62, 8);
    display->drawBox(65, 35, lightBarWidth, 6);

    // 运行时间
    uint32_t hours = g_systemState.uptime / 3600;
    uint32_t minutes = (g_systemState.uptime % 3600) / 60;
    display->setCursor(4, 52);
    display->printf("运行: %luh %lum", hours, minutes);

    // 返回提示
    display->setCursor(4, contentHeight - 5);
    display->print("按BACK键返回");
}

/**
 * @brief 系统监控页面渲染
 */
void monitorPageRender(U8G2 *display, const wEui_DisplayConfig_t *displayConfig, uint8_t contentHeight) {
    display->setDrawColor(1);
    display->setFont(displayConfig->font);

    // 标题
    display->setCursor(4, 12);
    display->print("系统监控");

    display->drawHLine(4, 15, displayConfig->width - 8);

    // 内存使用
    uint32_t totalHeap = esp_get_heap_size();
    uint32_t freePercent = (g_systemState.freeHeap * 100) / totalHeap;
    display->setCursor(4, 28);
    display->printf("内存: %lu%%", freePercent);

    // 内存条形图
    int memBarWidth = freePercent * 60 / 100;
    display->drawFrame(64, 22, 62, 8);
    display->drawBox(65, 23, memBarWidth, 6);

    // 电池状态
    display->setCursor(4, 40);
    display->printf("电池: %d%%%s", g_systemState.batteryLevel,
                   g_systemState.isCharging ? " ⚡" : "");

    // 电池条形图
    int battBarWidth = g_systemState.batteryLevel * 60 / 100;
    display->drawFrame(64, 34, 62, 8);
    display->drawBox(65, 35, battBarWidth, 6);

    // 连接状态
    display->setCursor(4, 52);
    display->printf("WiFi:%s BT:%s",
                   g_systemState.wifiEnabled ? "✓" : "✗",
                   g_systemState.bluetoothEnabled ? "✓" : "✗");

    // 页面堆栈信息
    display->setCursor(4, contentHeight - 15);
    display->printf("页面堆栈: %d", wEui_page_getStackDepth());

    display->setCursor(4, contentHeight - 5);
    display->print("按BACK键返回");
}

/**
 * @brief 关于页面渲染
 */
void aboutPageRender(U8G2 *display, const wEui_DisplayConfig_t *displayConfig, uint8_t contentHeight) {
    display->setDrawColor(1);
    display->setFont(displayConfig->font);

    // 标题
    const char* title = "wEui Complete Demo";
    int titleWidth = display->getStrWidth(title);
    display->setCursor((displayConfig->width - titleWidth) / 2, 12);
    display->print(title);

    display->drawHLine(4, 15, displayConfig->width - 8);

    // 版本信息
    display->setCursor(4, 28);
    display->print("版本: v1.0.0");

    display->setCursor(4, 38);
    display->print("构建: 2024.01.31");

    // 设备信息
    display->setCursor(4, 48);
    display->printf("设备: %s", g_systemState.deviceName);

    // 功能特性
    display->setCursor(4, contentHeight - 25);
    display->print("特性: 多页面+状态栏");

    display->setCursor(4, contentHeight - 15);
    display->print("作者: wEui Team");

    display->setCursor(4, contentHeight - 5);
    display->print("按BACK键返回");
}

// ============================================================================
// 主菜单回调函数
// ============================================================================

void mainMenu_sensors(uint8_t index) {
    Serial.println("查看传感器数据");
    wEui_page_push(g_sensorsPageId);
    wEui_statusBar_setText("传感器");
}

void mainMenu_monitor(uint8_t index) {
    Serial.println("查看系统监控");
    wEui_page_push(g_monitorPageId);
    wEui_statusBar_setText("系统监控");
}

void mainMenu_settings(uint8_t index) {
    Serial.println("进入设置");

    wEui_page_push(g_settingsPageId);
    wEui_page_switchListContext(g_settingsPageId);

    wEui_list_clear();
    wEui_list_addItem("WiFi设置", settings_wifi);
    wEui_list_addItem("显示设置", settings_display);
    wEui_list_addItem("系统设置", settings_system);
    wEui_list_addItem("返回", settings_back);

    wEui_statusBar_setText("设置");
}

void mainMenu_about(uint8_t index) {
    Serial.println("显示关于信息");
    wEui_page_push(g_aboutPageId);
    wEui_statusBar_setText("关于");
}

// ============================================================================
// 设置菜单回调函数
// ============================================================================

void settings_wifi(uint8_t index) {
    Serial.println("WiFi设置");

    wEui_page_push(g_wifiSettingsPageId);
    wEui_page_switchListContext(g_wifiSettingsPageId);

    wEui_list_clear();
    wEui_list_addItem(g_systemState.wifiEnabled ? "禁用WiFi" : "启用WiFi", wifi_toggle);
    wEui_list_addItem(g_systemState.bluetoothEnabled ? "禁用蓝牙" : "启用蓝牙", bluetooth_toggle);
    wEui_list_addItem("扫描网络", wifi_scan);
    wEui_list_addItem("返回", wifi_back);

    wEui_statusBar_setText("无线设置");
}

void settings_display(uint8_t index) {
    Serial.println("显示设置");

    char brightnessItem[32];
    snprintf(brightnessItem, sizeof(brightnessItem), "亮度: %d%%", g_systemState.brightness);

    // 这里可以创建一个临时的显示设置列表
    wEui_statusBar_setText("亮度调整中...");

    // 调整亮度
    g_systemState.brightness += 20;
    if (g_systemState.brightness > 100) {
        g_systemState.brightness = 20;
    }

    char statusText[64];
    snprintf(statusText, sizeof(statusText), "亮度: %d%%", g_systemState.brightness);
    wEui_statusBar_setText(statusText);

    Serial.printf("亮度调整为: %d%%\n", g_systemState.brightness);

    vTaskDelay(pdMS_TO_TICKS(1500));
    wEui_statusBar_setText("设置");
}

void settings_system(uint8_t index) {
    Serial.println("系统设置");

    wEui_page_push(g_systemPageId);
    wEui_page_switchListContext(g_systemPageId);

    wEui_list_clear();
    wEui_list_addItem(g_systemState.soundEnabled ? "关闭声音" : "开启声音", system_sound);
    wEui_list_addItem(g_systemState.autoUpdate ? "禁用自动更新" : "启用自动更新", system_autoUpdate);
    wEui_list_addItem("重启系统", system_reboot);
    wEui_list_addItem("恢复出厂设置", system_factory);
    wEui_list_addItem("返回", system_back);

    wEui_statusBar_setText("系统设置");
}

void settings_back(uint8_t index) {
    Serial.println("返回主菜单");

    wEui_page_pop();
    wEui_page_switchListContext(g_mainMenuPageId);

    wEui_list_clear();
    wEui_list_addItem("传感器", mainMenu_sensors);
    wEui_list_addItem("监控", mainMenu_monitor);
    wEui_list_addItem("设置", mainMenu_settings);
    wEui_list_addItem("关于", mainMenu_about);

    wEui_statusBar_setText("主菜单");
}

// ============================================================================
// WiFi设置回调函数
// ============================================================================

void wifi_toggle(uint8_t index) {
    g_systemState.wifiEnabled = !g_systemState.wifiEnabled;
    Serial.printf("WiFi %s\n", g_systemState.wifiEnabled ? "启用" : "禁用");

    // 更新菜单
    wEui_list_clear();
    wEui_list_addItem(g_systemState.wifiEnabled ? "禁用WiFi" : "启用WiFi", wifi_toggle);
    wEui_list_addItem(g_systemState.bluetoothEnabled ? "禁用蓝牙" : "启用蓝牙", bluetooth_toggle);
    wEui_list_addItem("扫描网络", wifi_scan);
    wEui_list_addItem("返回", wifi_back);

    char statusText[64];
    snprintf(statusText, sizeof(statusText), "WiFi: %s", g_systemState.wifiEnabled ? "启用" : "禁用");
    wEui_statusBar_setText(statusText);

    vTaskDelay(pdMS_TO_TICKS(1000));
    wEui_statusBar_setText("无线设置");
}

void bluetooth_toggle(uint8_t index) {
    g_systemState.bluetoothEnabled = !g_systemState.bluetoothEnabled;
    Serial.printf("蓝牙 %s\n", g_systemState.bluetoothEnabled ? "启用" : "禁用");

    // 更新菜单
    wEui_list_clear();
    wEui_list_addItem(g_systemState.wifiEnabled ? "禁用WiFi" : "启用WiFi", wifi_toggle);
    wEui_list_addItem(g_systemState.bluetoothEnabled ? "禁用蓝牙" : "启用蓝牙", bluetooth_toggle);
    wEui_list_addItem("扫描网络", wifi_scan);
    wEui_list_addItem("返回", wifi_back);

    char statusText[64];
    snprintf(statusText, sizeof(statusText), "蓝牙: %s", g_systemState.bluetoothEnabled ? "启用" : "禁用");
    wEui_statusBar_setText(statusText);

    vTaskDelay(pdMS_TO_TICKS(1000));
    wEui_statusBar_setText("无线设置");
}

void wifi_scan(uint8_t index) {
    Serial.println("扫描WiFi网络...");

    wEui_statusBar_setText("扫描中...");
    vTaskDelay(pdMS_TO_TICKS(1000));

    wEui_statusBar_setText("找到 5 个网络");
    vTaskDelay(pdMS_TO_TICKS(1500));

    wEui_statusBar_setText("无线设置");
}

void wifi_back(uint8_t index) {
    Serial.println("返回设置菜单");

    wEui_page_pop();
    wEui_page_switchListContext(g_settingsPageId);

    wEui_list_clear();
    wEui_list_addItem("WiFi设置", settings_wifi);
    wEui_list_addItem("显示设置", settings_display);
    wEui_list_addItem("系统设置", settings_system);
    wEui_list_addItem("返回", settings_back);

    wEui_statusBar_setText("设置");
}

// ============================================================================
// 系统设置回调函数
// ============================================================================

void system_sound(uint8_t index) {
    g_systemState.soundEnabled = !g_systemState.soundEnabled;
    Serial.printf("声音 %s\n", g_systemState.soundEnabled ? "启用" : "禁用");

    // 更新菜单
    wEui_list_clear();
    wEui_list_addItem(g_systemState.soundEnabled ? "关闭声音" : "开启声音", system_sound);
    wEui_list_addItem(g_systemState.autoUpdate ? "禁用自动更新" : "启用自动更新", system_autoUpdate);
    wEui_list_addItem("重启系统", system_reboot);
    wEui_list_addItem("恢复出厂设置", system_factory);
    wEui_list_addItem("返回", system_back);

    char statusText[64];
    snprintf(statusText, sizeof(statusText), "声音: %s", g_systemState.soundEnabled ? "开启" : "关闭");
    wEui_statusBar_setText(statusText);

    vTaskDelay(pdMS_TO_TICKS(1000));
    wEui_statusBar_setText("系统设置");
}

void system_autoUpdate(uint8_t index) {
    g_systemState.autoUpdate = !g_systemState.autoUpdate;
    Serial.printf("自动更新 %s\n", g_systemState.autoUpdate ? "启用" : "禁用");

    // 更新菜单
    wEui_list_clear();
    wEui_list_addItem(g_systemState.soundEnabled ? "关闭声音" : "开启声音", system_sound);
    wEui_list_addItem(g_systemState.autoUpdate ? "禁用自动更新" : "启用自动更新", system_autoUpdate);
    wEui_list_addItem("重启系统", system_reboot);
    wEui_list_addItem("恢复出厂设置", system_factory);
    wEui_list_addItem("返回", system_back);

    char statusText[64];
    snprintf(statusText, sizeof(statusText), "自动更新: %s", g_systemState.autoUpdate ? "启用" : "禁用");
    wEui_statusBar_setText(statusText);

    vTaskDelay(pdMS_TO_TICKS(1000));
    wEui_statusBar_setText("系统设置");
}

void system_reboot(uint8_t index) {
    Serial.println("系统重启...");

    wEui_statusBar_setText("重启中...");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 模拟重启
    wEui_statusBar_setText("重启完成");
    vTaskDelay(pdMS_TO_TICKS(1000));
    wEui_statusBar_setText("系统设置");
}

void system_factory(uint8_t index) {
    Serial.println("恢复出厂设置...");

    wEui_statusBar_setText("恢复中...");
    vTaskDelay(pdMS_TO_TICKS(1500));

    // 重置系统状态
    g_systemState.wifiEnabled = true;
    g_systemState.bluetoothEnabled = false;
    g_systemState.brightness = 80;
    g_systemState.soundEnabled = true;
    g_systemState.autoUpdate = true;
    strcpy(g_systemState.deviceName, "wEui Demo Device");

    wEui_statusBar_setText("恢复完成");
    vTaskDelay(pdMS_TO_TICKS(1000));
    wEui_statusBar_setText("系统设置");
}

void system_back(uint8_t index) {
    Serial.println("返回设置菜单");

    wEui_page_pop();
    wEui_page_switchListContext(g_settingsPageId);

    wEui_list_clear();
    wEui_list_addItem("WiFi设置", settings_wifi);
    wEui_list_addItem("显示设置", settings_display);
    wEui_list_addItem("系统设置", settings_system);
    wEui_list_addItem("返回", settings_back);

    wEui_statusBar_setText("设置");
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
    const char* buttonEvent;

    for (;;) {
        if (xQueueReceive(buttonQueue, &buttonEvent, pdMS_TO_TICKS(100)) == pdPASS) {
            if (strcmp(buttonEvent, "BACK") == 0) {
                // 自动处理返回逻辑
                if (wEui_page_getStackDepth() > 1) {
                    wEui_page_pop();

                    // 根据返回的页面恢复相应内容
                    int currentPageId = wEui_page_getCurrentId();
                    if (currentPageId == g_mainMenuPageId) {
                        wEui_page_switchListContext(g_mainMenuPageId);
                        wEui_list_clear();
                        wEui_list_addItem("传感器", mainMenu_sensors);
                        wEui_list_addItem("监控", mainMenu_monitor);
                        wEui_list_addItem("设置", mainMenu_settings);
                        wEui_list_addItem("关于", mainMenu_about);
                        wEui_statusBar_setText("主菜单");
                    }
                }
            } else {
                wEui_processButtonEvents(0);
            }
        }
    }
}

/**
 * @brief 传感器数据更新任务
 */
void vTaskSensorUpdate(void *pvParameters) {
    for (;;) {
        updateSensorData();
        vTaskDelay(pdMS_TO_TICKS(2000));  // 每2秒更新一次传感器数据
    }
}

/**
 * @brief 动态状态栏更新任务
 */
void vTaskStatusBarUpdate(void *pvParameters) {
    TickType_t lastUpdate = xTaskGetTickCount();
    uint32_t updateCounter = 0;

    for (;;) {
        TickType_t currentTick = xTaskGetTickCount();

        // 每5秒更新一次状态栏
        if ((currentTick - lastUpdate) > pdMS_TO_TICKS(5000)) {
            lastUpdate = currentTick;
            updateCounter++;

            int currentPageId = wEui_page_getCurrentId();

            // 根据当前页面显示不同的状态信息
            if (currentPageId == g_mainMenuPageId) {
                char statusText[64];
                if (updateCounter % 3 == 0) {
                    snprintf(statusText, sizeof(statusText), "电量: %d%%", g_systemState.batteryLevel);
                } else if (updateCounter % 3 == 1) {
                    snprintf(statusText, sizeof(statusText), "温度: %.1f°C", g_systemState.temperature);
                } else {
                    snprintf(statusText, sizeof(statusText), "内存: %luK", g_systemState.freeHeap / 1024);
                }
                wEui_statusBar_setText(statusText);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief 系统监控任务
 */
void vTaskSystemMonitor(void *pvParameters) {
    for (;;) {
        // 检查系统状态
        if (g_systemState.batteryLevel < 20) {
            Serial.println("警告: 电池电量低!");
        }

        if (g_systemState.freeHeap < 50000) {  // 少于50KB
            Serial.println("警告: 内存不足!");
        }

        if (g_systemState.temperature > 35.0f) {
            Serial.println("警告: 温度过高!");
        }

        vTaskDelay(pdMS_TO_TICKS(10000));  // 每10秒检查一次
    }
}

// ============================================================================
// 页面初始化
// ============================================================================

void initializePages() {
    Serial.println("初始化完整应用页面系统...");

    if (wEui_page_init() != 0) {
        Serial.println("页面系统初始化失败!");
        return;
    }

    // 创建所有页面
    g_mainMenuPageId = wEui_page_createList("主菜单");
    g_sensorsPageId = wEui_page_createCustom("传感器", sensorsPageRender);
    g_monitorPageId = wEui_page_createCustom("监控", monitorPageRender);
    g_settingsPageId = wEui_page_createList("设置");
    g_aboutPageId = wEui_page_createCustom("关于", aboutPageRender);
    g_wifiSettingsPageId = wEui_page_createList("WiFi设置");
    g_systemPageId = wEui_page_createList("系统设置");

    Serial.printf("页面创建完成 - 主菜单:%d, 传感器:%d, 监控:%d, 设置:%d, 关于:%d\n",
                  g_mainMenuPageId, g_sensorsPageId, g_monitorPageId, g_settingsPageId, g_aboutPageId);

    // 设置主菜单
    wEui_page_push(g_mainMenuPageId);
    wEui_page_switchListContext(g_mainMenuPageId);

    wEui_list_addItem("传感器", mainMenu_sensors);
    wEui_list_addItem("监控", mainMenu_monitor);
    wEui_list_addItem("设置", mainMenu_settings);
    wEui_list_addItem("关于", mainMenu_about);

    wEui_statusBar_setText("wEui Complete Demo");

    Serial.println("完整应用页面系统初始化完成");
}

// ============================================================================
// 主程序
// ============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("=========================================");
    Serial.println("      wEui完整应用示例启动中...");
    Serial.println("=========================================");

    // 初始化系统状态
    initSystemState();

    // 初始化I2C
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // 创建FreeRTOS对象
    buttonQueue = xQueueCreate(20, sizeof(const char*));
    listMutex = xSemaphoreCreateMutex();

    if (buttonQueue == NULL || listMutex == NULL) {
        Serial.println("错误: 创建FreeRTOS对象失败!");
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
        Serial.println("错误: 初始化wEui失败!");
        return;
    }

    // 初始化列表和状态栏
    wEui_list_init(listMutex);
    wEui_statusBar_setEnabled(true);
    wEui_statusBar_setShowBorder(true);

    // 初始化页面系统
    initializePages();

    // 启动UI
    wEui_begin();

    // 创建所有FreeRTOS任务
    BaseType_t result;

    result = xTaskCreate(vTaskUI, "UI_Task", 2048, NULL, 3, NULL);
    if (result != pdPASS) {
        Serial.println("错误: 创建UI任务失败!");
        return;
    }

    result = xTaskCreate(vTaskButtons, "Button_Task", 1024, NULL, 4, NULL);
    if (result != pdPASS) {
        Serial.println("错误: 创建按钮任务失败!");
        return;
    }

    result = xTaskCreate(vTaskGUIService, "GUI_Service", 2048, NULL, 2, NULL);
    if (result != pdPASS) {
        Serial.println("错误: 创建GUI服务任务失败!");
        return;
    }

    result = xTaskCreate(vTaskSensorUpdate, "Sensor_Task", 1024, NULL, 1, NULL);
    if (result != pdPASS) {
        Serial.println("错误: 创建传感器任务失败!");
        return;
    }

    result = xTaskCreate(vTaskStatusBarUpdate, "StatusBar_Task", 1024, NULL, 1, NULL);
    if (result != pdPASS) {
        Serial.println("错误: 创建状态栏任务失败!");
        return;
    }

    result = xTaskCreate(vTaskSystemMonitor, "Monitor_Task", 1024, NULL, 1, NULL);
    if (result != pdPASS) {
        Serial.println("错误: 创建监控任务失败!");
        return;
    }

    Serial.println("=========================================");
    Serial.println("      wEui完整应用示例就绪!");
    Serial.println("=========================================");
    Serial.println("应用功能:");
    Serial.println("  ✓ 实时传感器数据显示");
    Serial.println("  ✓ 系统状态监控");
    Serial.println("  ✓ 多级设置菜单");
    Serial.println("  ✓ 动态状态栏");
    Serial.println("  ✓ 电池和连接状态");
    Serial.println("  ✓ 自动页面导航");
    Serial.println("");
    Serial.println("操作说明:");
    Serial.println("  上/下键: 菜单导航");
    Serial.println("  确定键: 选择/执行");
    Serial.println("  返回键: 自动返回上级");
    Serial.println("=========================================");
}

void loop() {
    // FreeRTOS处理所有任务
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 定期输出完整系统状态
    static uint32_t lastStatusPrint = 0;
    uint32_t currentTime = millis();
    if (currentTime - lastStatusPrint > 30000) {  // 每30秒输出一次
        lastStatusPrint = currentTime;

        Serial.println("\n========== 系统状态报告 ==========");
        Serial.printf("运行时间: %lu秒\n", g_systemState.uptime);
        Serial.printf("当前页面: %s\n", wEui_page_getName(wEui_page_getCurrentId()));
        Serial.printf("页面堆栈深度: %d\n", wEui_page_getStackDepth());
        Serial.printf("温度: %.1f°C\n", g_systemState.temperature);
        Serial.printf("光照: %d\n", g_systemState.lightLevel);
        Serial.printf("电池: %d%%%s\n", g_systemState.batteryLevel,
                     g_systemState.isCharging ? " (充电中)" : "");
        Serial.printf("可用内存: %lu bytes\n", g_systemState.freeHeap);
        Serial.printf("WiFi: %s | 蓝牙: %s\n",
                     g_systemState.wifiEnabled ? "启用" : "禁用",
                     g_systemState.bluetoothEnabled ? "启用" : "禁用");
        Serial.printf("亮度: %d%% | 声音: %s\n",
                     g_systemState.brightness,
                     g_systemState.soundEnabled ? "启用" : "禁用");
        Serial.println("================================\n");
    }
}
