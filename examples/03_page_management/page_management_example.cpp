/**
 * @file page_management_example.cpp
 * @brief wEui库页面管理系统完整示例
 *
 * 这个示例演示wEui库的高级页面管理功能：
 * - 创建多级菜单页面
 * - 页面堆栈管理
 * - 自定义页面渲染
 * - 页面间导航
 * - 多页面列表上下文切换
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

// 页面ID变量
int g_mainMenuPageId = -1;
int g_settingsPageId = -1;
int g_wifiSettingsPageId = -1;
int g_displaySettingsPageId = -1;
int g_systemInfoPageId = -1;
int g_aboutPageId = -1;

// 设置变量
bool g_wifiEnabled = true;
uint8_t g_brightness = 80;
bool g_soundEnabled = true;
char g_deviceName[32] = "wEui Device";

// ============================================================================
// 自定义页面渲染函数
// ============================================================================

/**
 * @brief 关于页面的自定义渲染函数
 */
void aboutPageRender(U8G2 *display, const wEui_DisplayConfig_t *displayConfig, uint8_t contentHeight) {
    display->setDrawColor(1);
    display->setFont(displayConfig->font);

    // 绘制标题
    const char* title = "wEui v1.0.0";
    int titleWidth = display->getStrWidth(title);
    display->setCursor((displayConfig->width - titleWidth) / 2, 15);
    display->print(title);

    // 绘制描述信息
    display->setCursor(4, 30);
    display->print("轻量级嵌入式UI库");

    display->setCursor(4, 45);
    display->print("支持页面堆栈管理");

    display->setCursor(4, contentHeight - 25);
    display->print("功能特性:");

    display->setCursor(4, contentHeight - 15);
    display->print("✓ 多页面支持");

    // 显示返回提示
    display->setCursor(4, contentHeight - 5);
    display->print("按BACK键返回");
}

/**
 * @brief 系统信息页面渲染函数
 */
void systemInfoPageRender(U8G2 *display, const wEui_DisplayConfig_t *displayConfig, uint8_t contentHeight) {
    display->setDrawColor(1);
    display->setFont(displayConfig->font);

    // 标题
    display->setCursor(4, 10);
    display->print("系统信息");

    // 设备信息
    display->setCursor(4, 25);
    display->printf("设备: %s", g_deviceName);

    // 内存信息
    uint32_t freeHeap = esp_get_free_heap_size();
    uint32_t totalHeap = esp_get_heap_size();
    display->setCursor(4, 35);
    display->printf("内存: %lu/%lu", freeHeap, totalHeap);

    // 堆栈深度
    display->setCursor(4, 45);
    display->printf("页面堆栈深度: %d", wEui_page_getStackDepth());

    // WiFi状态
    display->setCursor(4, 55);
    display->printf("WiFi: %s", g_wifiEnabled ? "启用" : "禁用");

    // 返回提示
    display->setCursor(4, contentHeight - 5);
    display->print("按BACK键返回");
}

// ============================================================================
// 主菜单回调函数
// ============================================================================

void mainMenu_settings(uint8_t index) {
    Serial.println("进入设置菜单");

    // 推送设置页面
    wEui_page_push(g_settingsPageId);
    wEui_page_switchListContext(g_settingsPageId);

    // 清空并重新填充设置菜单
    wEui_list_clear();
    wEui_list_addItem("WiFi设置", settings_wifi);
    wEui_list_addItem("显示设置", settings_display);
    wEui_list_addItem("系统信息", settings_systemInfo);
    wEui_list_addItem("返回", settings_back);

    wEui_statusBar_setText("设置");
}

void mainMenu_about(uint8_t index) {
    Serial.println("显示关于页面");

    // 推送关于页面
    wEui_page_push(g_aboutPageId);
    wEui_statusBar_setText("关于");
}

void mainMenu_exit(uint8_t index) {
    Serial.println("退出应用程序");
    wEui_statusBar_setText("再见!");

    // 这里可以添加退出逻辑
    vTaskDelay(pdMS_TO_TICKS(1000));
    wEui_statusBar_setText("主菜单");
}

// ============================================================================
// 设置菜单回调函数
// ============================================================================

void settings_wifi(uint8_t index) {
    Serial.println("进入WiFi设置");

    // 推送WiFi设置页面
    wEui_page_push(g_wifiSettingsPageId);
    wEui_page_switchListContext(g_wifiSettingsPageId);

    // 填充WiFi设置菜单
    wEui_list_clear();
    wEui_list_addItem(g_wifiEnabled ? "禁用WiFi" : "启用WiFi", wifi_toggle);
    wEui_list_addItem("扫描网络", wifi_scan);
    wEui_list_addItem("连接状态", wifi_status);
    wEui_list_addItem("返回", wifi_back);

    wEui_statusBar_setText("WiFi设置");
}

void settings_display(uint8_t index) {
    Serial.println("进入显示设置");

    // 推送显示设置页面
    wEui_page_push(g_displaySettingsPageId);
    wEui_page_switchListContext(g_displaySettingsPageId);

    // 填充显示设置菜单
    wEui_list_clear();

    char brightnessItem[32];
    snprintf(brightnessItem, sizeof(brightnessItem), "亮度: %d%%", g_brightness);
    wEui_list_addItem(brightnessItem, display_brightness);

    wEui_list_addItem(g_soundEnabled ? "关闭声音" : "开启声音", display_sound);
    wEui_list_addItem("重置设置", display_reset);
    wEui_list_addItem("返回", display_back);

    wEui_statusBar_setText("显示设置");
}

void settings_systemInfo(uint8_t index) {
    Serial.println("显示系统信息");

    // 推送系统信息页面
    wEui_page_push(g_systemInfoPageId);
    wEui_statusBar_setText("系统信息");
}

void settings_back(uint8_t index) {
    Serial.println("返回主菜单");

    // 弹出当前页面，返回主菜单
    wEui_page_pop();

    // 恢复主菜单内容
    wEui_page_switchListContext(g_mainMenuPageId);
    wEui_list_clear();
    wEui_list_addItem("设置", mainMenu_settings);
    wEui_list_addItem("关于", mainMenu_about);
    wEui_list_addItem("退出", mainMenu_exit);

    wEui_statusBar_setText("主菜单");
}

// ============================================================================
// WiFi设置回调函数
// ============================================================================

void wifi_toggle(uint8_t index) {
    g_wifiEnabled = !g_wifiEnabled;
    Serial.printf("WiFi %s\n", g_wifiEnabled ? "启用" : "禁用");

    // 更新菜单项文本
    wEui_list_clear();
    wEui_list_addItem(g_wifiEnabled ? "禁用WiFi" : "启用WiFi", wifi_toggle);
    wEui_list_addItem("扫描网络", wifi_scan);
    wEui_list_addItem("连接状态", wifi_status);
    wEui_list_addItem("返回", wifi_back);

    char statusText[64];
    snprintf(statusText, sizeof(statusText), "WiFi: %s", g_wifiEnabled ? "启用" : "禁用");
    wEui_statusBar_setText(statusText);
}

void wifi_scan(uint8_t index) {
    Serial.println("扫描WiFi网络...");
    wEui_statusBar_setText("扫描中...");

    // 模拟扫描过程
    vTaskDelay(pdMS_TO_TICKS(2000));
    wEui_statusBar_setText("找到 3 个网络");

    vTaskDelay(pdMS_TO_TICKS(1500));
    wEui_statusBar_setText("WiFi设置");
}

void wifi_status(uint8_t index) {
    Serial.println("检查WiFi连接状态");

    if (g_wifiEnabled) {
        wEui_statusBar_setText("WiFi: 已连接");
    } else {
        wEui_statusBar_setText("WiFi: 未启用");
    }

    vTaskDelay(pdMS_TO_TICKS(1500));
    wEui_statusBar_setText("WiFi设置");
}

void wifi_back(uint8_t index) {
    Serial.println("返回设置菜单");

    // 弹出WiFi设置页面
    wEui_page_pop();

    // 恢复设置菜单
    wEui_page_switchListContext(g_settingsPageId);
    wEui_list_clear();
    wEui_list_addItem("WiFi设置", settings_wifi);
    wEui_list_addItem("显示设置", settings_display);
    wEui_list_addItem("系统信息", settings_systemInfo);
    wEui_list_addItem("返回", settings_back);

    wEui_statusBar_setText("设置");
}

// ============================================================================
// 显示设置回调函数
// ============================================================================

void display_brightness(uint8_t index) {
    Serial.println("调整亮度");

    // 循环调整亮度：20% -> 40% -> 60% -> 80% -> 100% -> 20%
    g_brightness += 20;
    if (g_brightness > 100) {
        g_brightness = 20;
    }

    Serial.printf("亮度设置为: %d%%\n", g_brightness);

    // 更新菜单项
    wEui_list_clear();

    char brightnessItem[32];
    snprintf(brightnessItem, sizeof(brightnessItem), "亮度: %d%%", g_brightness);
    wEui_list_addItem(brightnessItem, display_brightness);

    wEui_list_addItem(g_soundEnabled ? "关闭声音" : "开启声音", display_sound);
    wEui_list_addItem("重置设置", display_reset);
    wEui_list_addItem("返回", display_back);

    char statusText[64];
    snprintf(statusText, sizeof(statusText), "亮度: %d%%", g_brightness);
    wEui_statusBar_setText(statusText);

    vTaskDelay(pdMS_TO_TICKS(1000));
    wEui_statusBar_setText("显示设置");
}

void display_sound(uint8_t index) {
    g_soundEnabled = !g_soundEnabled;
    Serial.printf("声音 %s\n", g_soundEnabled ? "启用" : "禁用");

    // 更新菜单项
    wEui_list_clear();

    char brightnessItem[32];
    snprintf(brightnessItem, sizeof(brightnessItem), "亮度: %d%%", g_brightness);
    wEui_list_addItem(brightnessItem, display_brightness);

    wEui_list_addItem(g_soundEnabled ? "关闭声音" : "开启声音", display_sound);
    wEui_list_addItem("重置设置", display_reset);
    wEui_list_addItem("返回", display_back);

    char statusText[64];
    snprintf(statusText, sizeof(statusText), "声音: %s", g_soundEnabled ? "启用" : "禁用");
    wEui_statusBar_setText(statusText);

    vTaskDelay(pdMS_TO_TICKS(1000));
    wEui_statusBar_setText("显示设置");
}

void display_reset(uint8_t index) {
    Serial.println("重置显示设置");

    // 重置所有显示设置
    g_brightness = 80;
    g_soundEnabled = true;

    wEui_statusBar_setText("设置已重置");

    // 更新菜单项
    wEui_list_clear();

    char brightnessItem[32];
    snprintf(brightnessItem, sizeof(brightnessItem), "亮度: %d%%", g_brightness);
    wEui_list_addItem(brightnessItem, display_brightness);

    wEui_list_addItem(g_soundEnabled ? "关闭声音" : "开启声音", display_sound);
    wEui_list_addItem("重置设置", display_reset);
    wEui_list_addItem("返回", display_back);

    vTaskDelay(pdMS_TO_TICKS(1500));
    wEui_statusBar_setText("显示设置");
}

void display_back(uint8_t index) {
    Serial.println("返回设置菜单");

    // 弹出显示设置页面
    wEui_page_pop();

    // 恢复设置菜单
    wEui_page_switchListContext(g_settingsPageId);
    wEui_list_clear();
    wEui_list_addItem("WiFi设置", settings_wifi);
    wEui_list_addItem("显示设置", settings_display);
    wEui_list_addItem("系统信息", settings_systemInfo);
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
 * @brief GUI服务任务，包含自动返回处理
 */
void vTaskGUIService(void *pvParameters) {
    const char* buttonEvent;

    for (;;) {
        // 处理按钮事件，等待最多100ms
        if (xQueueReceive(buttonQueue, &buttonEvent, pdMS_TO_TICKS(100)) == pdPASS) {

            // 处理返回按钮逻辑
            if (strcmp(buttonEvent, "BACK") == 0) {
                uint8_t stackDepth = wEui_page_getStackDepth();

                if (stackDepth > 1) {
                    Serial.printf("自动返回，当前堆栈深度: %d\n", stackDepth);

                    int currentPageId = wEui_page_getCurrentId();
                    wEui_PageType_t pageType = wEui_page_getType(currentPageId);

                    // 弹出当前页面
                    wEui_page_pop();

                    // 根据返回到的页面类型处理
                    int newCurrentPageId = wEui_page_getCurrentId();
                    wEui_PageType_t newPageType = wEui_page_getType(newCurrentPageId);

                    if (newPageType == WEUI_PAGE_TYPE_LIST) {
                        // 返回到列表页面，恢复对应的菜单内容
                        if (newCurrentPageId == g_mainMenuPageId) {
                            wEui_page_switchListContext(g_mainMenuPageId);
                            wEui_list_clear();
                            wEui_list_addItem("设置", mainMenu_settings);
                            wEui_list_addItem("关于", mainMenu_about);
                            wEui_list_addItem("退出", mainMenu_exit);
                            wEui_statusBar_setText("主菜单");
                        }
                        else if (newCurrentPageId == g_settingsPageId) {
                            wEui_page_switchListContext(g_settingsPageId);
                            wEui_list_clear();
                            wEui_list_addItem("WiFi设置", settings_wifi);
                            wEui_list_addItem("显示设置", settings_display);
                            wEui_list_addItem("系统信息", settings_systemInfo);
                            wEui_list_addItem("返回", settings_back);
                            wEui_statusBar_setText("设置");
                        }
                        // 可以添加更多页面的恢复逻辑
                    }

                    const char* pageName = wEui_page_getName(newCurrentPageId);
                    Serial.printf("返回到页面: %s\n", pageName);
                } else {
                    Serial.println("已在根页面，无法返回");
                }
            } else {
                // 处理其他按钮事件
                wEui_processButtonEvents(0);  // 立即处理
            }
        }
    }
}

// ============================================================================
// 页面初始化
// ============================================================================

void initializePages() {
    Serial.println("初始化页面管理系统...");

    // 初始化页面管理系统
    if (wEui_page_init() != 0) {
        Serial.println("页面管理系统初始化失败!");
        return;
    }

    // 创建所有页面
    g_mainMenuPageId = wEui_page_createList("主菜单");
    g_settingsPageId = wEui_page_createList("设置");
    g_wifiSettingsPageId = wEui_page_createList("WiFi设置");
    g_displaySettingsPageId = wEui_page_createList("显示设置");
    g_systemInfoPageId = wEui_page_createCustom("系统信息", systemInfoPageRender);
    g_aboutPageId = wEui_page_createCustom("关于", aboutPageRender);

    Serial.printf("创建页面ID - 主菜单:%d, 设置:%d, WiFi:%d, 显示:%d, 系统信息:%d, 关于:%d\n",
                  g_mainMenuPageId, g_settingsPageId, g_wifiSettingsPageId,
                  g_displaySettingsPageId, g_systemInfoPageId, g_aboutPageId);

    // 设置主菜单为初始页面
    wEui_page_push(g_mainMenuPageId);
    wEui_page_switchListContext(g_mainMenuPageId);

    // 添加主菜单项
    wEui_list_addItem("设置", mainMenu_settings);
    wEui_list_addItem("关于", mainMenu_about);
    wEui_list_addItem("退出", mainMenu_exit);

    wEui_statusBar_setText("主菜单");

    Serial.println("页面管理系统初始化完成");
}

// ============================================================================
// 主程序
// ============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("wEui页面管理示例启动中...");

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

    // 初始化页面系统
    initializePages();

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

    Serial.println("wEui页面管理示例就绪!");
    Serial.println("导航说明:");
    Serial.println("  上/下键: 选择菜单项");
    Serial.println("  确定键: 进入选中项");
    Serial.println("  返回键: 返回上一级(自动处理)");
    Serial.println("页面功能:");
    Serial.println("  - 多级菜单导航");
    Serial.println("  - 自定义页面显示");
    Serial.println("  - 页面堆栈管理");
    Serial.println("  - 状态栏常驻显示");
}

void loop() {
    // FreeRTOS处理所有任务
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 定期输出状态信息
    static uint32_t lastPrint = 0;
    uint32_t currentTime = millis();
    if (currentTime - lastPrint > 10000) {  // 每10秒输出一次
        lastPrint = currentTime;

        int currentPageId = wEui_page_getCurrentId();
        const char* currentPageName = wEui_page_getName(currentPageId);
        uint8_t stackDepth = wEui_page_getStackDepth();

        Serial.printf("状态 - 当前页面: %s (ID:%d) | 堆栈深度: %d | WiFi: %s | 亮度: %d%%\n",
                     currentPageName, currentPageId, stackDepth,
                     g_wifiEnabled ? "启用" : "禁用", g_brightness);
    }
}
