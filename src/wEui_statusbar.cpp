#include "../include/wEui.h"
#include <string.h>

static wEui_StatusBar_t g_statusBar = {0};
// 全局状态栏数据
static SemaphoreHandle_t g_statusBarMutex = nullptr;
// 保护状态栏读写的互斥量
static bool g_statusBar_initialized = false;
int wEui_statusBar_init(void) {
    if (g_statusBar_initialized) {
        return 0;
    }
    // 创建互斥量保证多线程安全
    g_statusBarMutex = xSemaphoreCreateMutex();
    if (g_statusBarMutex == nullptr) {
        return -1;
    }
    g_statusBar.showBorder = true;
    memset(g_statusBar.text, 0, WEUI_STATUS_BAR_LENGTH);
    g_statusBar.customRenderCallback = nullptr;
    g_statusBar_initialized = true;
    return 0;
}
void wEui_statusBar_deinit(void) {
    if (!g_statusBar_initialized) {
        return;
    }
    if (g_statusBarMutex != nullptr) {
        vSemaphoreDelete(g_statusBarMutex);
        g_statusBarMutex = nullptr;
    }
    memset(&g_statusBar, 0, sizeof(g_statusBar));
    g_statusBar_initialized = false;
}
void wEui_statusBar_setText(const char *text) {
    if (!g_statusBar_initialized || text == nullptr) {
        return;
    }
    if (g_statusBarMutex != nullptr) {
        xSemaphoreTake(g_statusBarMutex, portMAX_DELAY);
    }
    // 线程安全地更新状态栏文本
    strncpy(g_statusBar.text, text, WEUI_STATUS_BAR_LENGTH - 1);
    g_statusBar.text[WEUI_STATUS_BAR_LENGTH - 1] = '\0';
    if (g_statusBarMutex != nullptr) {
        xSemaphoreGive(g_statusBarMutex);
    }
}
const char* wEui_statusBar_getText(void) {
    static char tempText[WEUI_STATUS_BAR_LENGTH];
    if (!g_statusBar_initialized) {
        return "";
    }
    if (g_statusBarMutex != nullptr) {
        xSemaphoreTake(g_statusBarMutex, pdMS_TO_TICKS(10));
    }
    strncpy(tempText, g_statusBar.text, WEUI_STATUS_BAR_LENGTH - 1);
    tempText[WEUI_STATUS_BAR_LENGTH - 1] = '\0';
    if (g_statusBarMutex != nullptr) {
        xSemaphoreGive(g_statusBarMutex);
    }
    return tempText;
}
void wEui_statusBar_setShowBorder(bool showBorder) {
    if (!g_statusBar_initialized) {
        return;
    }
    if (g_statusBarMutex != nullptr) {
        xSemaphoreTake(g_statusBarMutex, portMAX_DELAY);
    }
    g_statusBar.showBorder = showBorder;
    if (g_statusBarMutex != nullptr) {
        xSemaphoreGive(g_statusBarMutex);
    }
}
bool wEui_statusBar_getShowBorder(void) {
    if (!g_statusBar_initialized) {
        return false;
    }
    bool showBorder = false;
    if (g_statusBarMutex != nullptr) {
        xSemaphoreTake(g_statusBarMutex, pdMS_TO_TICKS(10));
    }
    showBorder = g_statusBar.showBorder;
    if (g_statusBarMutex != nullptr) {
        xSemaphoreGive(g_statusBarMutex);
    }
    return showBorder;
}
uint8_t wEui_statusBar_getHeight(void) {
    return WEUI_STATUS_BAR_HEIGHT;
}
void wEui_statusBar_setCustomRender(wEui_StatusBarRenderCallback_t callback) {
    if (!g_statusBar_initialized) {
        return;
    }
    if (g_statusBarMutex != nullptr) {
        xSemaphoreTake(g_statusBarMutex, portMAX_DELAY);
    }
    g_statusBar.customRenderCallback = callback;
    if (g_statusBarMutex != nullptr) {
        xSemaphoreGive(g_statusBarMutex);
    }
}
void wEui_statusBar_render(U8G2 *display, const wEui_DisplayConfig_t *displayConfig) {
    if (!g_statusBar_initialized || display == nullptr || displayConfig == nullptr) {
        return;
    }
    uint8_t statusBarY = displayConfig->height - WEUI_STATUS_BAR_HEIGHT;
    display->setDrawColor(1);

    // Check if custom render callback is set
    wEui_StatusBarRenderCallback_t customCallback = nullptr;
    if (g_statusBarMutex != nullptr) {
        xSemaphoreTake(g_statusBarMutex, pdMS_TO_TICKS(10));
    }
    customCallback = g_statusBar.customRenderCallback;
    if (g_statusBarMutex != nullptr) {
        xSemaphoreGive(g_statusBarMutex);
    }

    // If custom callback is set, use it instead of default rendering
    if (customCallback != nullptr) {
        // 支持自定义状态栏渲染
        customCallback(display, displayConfig, statusBarY);
        return;
    }

    // Default rendering
    bool showBorder = false;
    if (g_statusBarMutex != nullptr) {
        xSemaphoreTake(g_statusBarMutex, pdMS_TO_TICKS(10));
    }
    showBorder = g_statusBar.showBorder;
    if (g_statusBarMutex != nullptr) {
        xSemaphoreGive(g_statusBarMutex);
    }
    if (showBorder) {
        display->drawHLine(0, statusBarY, displayConfig->width);
    }
    uint8_t textX = 2;
    uint8_t textY = statusBarY + 2;
    static char statusText[WEUI_STATUS_BAR_LENGTH];
    if (g_statusBarMutex != nullptr) {
        xSemaphoreTake(g_statusBarMutex, pdMS_TO_TICKS(10));
    }
    strncpy(statusText, g_statusBar.text, WEUI_STATUS_BAR_LENGTH - 1);
    statusText[WEUI_STATUS_BAR_LENGTH - 1] = '\0';
    if (g_statusBarMutex != nullptr) {
        xSemaphoreGive(g_statusBarMutex);
    }
    if (statusText[0] != '\0') {
        display->drawStr(textX, textY, statusText);
    }
}
