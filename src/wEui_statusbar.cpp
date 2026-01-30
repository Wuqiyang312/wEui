#include "../include/wEui.h"
#include <string.h>

static wEui_StatusBar_t g_statusBar = {0};
static SemaphoreHandle_t g_statusBarMutex = NULL;
static bool g_statusBar_initialized = false;
int wEui_statusBar_init(void) {
    if (g_statusBar_initialized) {
        return 0;
    }
    g_statusBarMutex = xSemaphoreCreateMutex();
    if (g_statusBarMutex == NULL) {
        return -1;
    }
    g_statusBar.showBorder = true;
    memset(g_statusBar.text, 0, WEUI_STATUS_BAR_LENGTH);
    g_statusBar_initialized = true;
    return 0;
}
void wEui_statusBar_deinit(void) {
    if (!g_statusBar_initialized) {
        return;
    }
    if (g_statusBarMutex != NULL) {
        vSemaphoreDelete(g_statusBarMutex);
        g_statusBarMutex = NULL;
    }
    memset(&g_statusBar, 0, sizeof(g_statusBar));
    g_statusBar_initialized = false;
}
void wEui_statusBar_setText(const char *text) {
    if (!g_statusBar_initialized || text == NULL) {
        return;
    }
    if (g_statusBarMutex != NULL) {
        xSemaphoreTake(g_statusBarMutex, portMAX_DELAY);
    }
    strncpy(g_statusBar.text, text, WEUI_STATUS_BAR_LENGTH - 1);
    g_statusBar.text[WEUI_STATUS_BAR_LENGTH - 1] = '\0';
    if (g_statusBarMutex != NULL) {
        xSemaphoreGive(g_statusBarMutex);
    }
}
const char* wEui_statusBar_getText(void) {
    static char tempText[WEUI_STATUS_BAR_LENGTH];
    if (!g_statusBar_initialized) {
        return "";
    }
    if (g_statusBarMutex != NULL) {
        xSemaphoreTake(g_statusBarMutex, pdMS_TO_TICKS(10));
    }
    strncpy(tempText, g_statusBar.text, WEUI_STATUS_BAR_LENGTH - 1);
    tempText[WEUI_STATUS_BAR_LENGTH - 1] = '\0';
    if (g_statusBarMutex != NULL) {
        xSemaphoreGive(g_statusBarMutex);
    }
    return tempText;
}
void wEui_statusBar_setShowBorder(bool showBorder) {
    if (!g_statusBar_initialized) {
        return;
    }
    if (g_statusBarMutex != NULL) {
        xSemaphoreTake(g_statusBarMutex, portMAX_DELAY);
    }
    g_statusBar.showBorder = showBorder;
    if (g_statusBarMutex != NULL) {
        xSemaphoreGive(g_statusBarMutex);
    }
}
bool wEui_statusBar_getShowBorder(void) {
    if (!g_statusBar_initialized) {
        return false;
    }
    bool showBorder = false;
    if (g_statusBarMutex != NULL) {
        xSemaphoreTake(g_statusBarMutex, pdMS_TO_TICKS(10));
    }
    showBorder = g_statusBar.showBorder;
    if (g_statusBarMutex != NULL) {
        xSemaphoreGive(g_statusBarMutex);
    }
    return showBorder;
}
uint8_t wEui_statusBar_getHeight(void) {
    return WEUI_STATUS_BAR_HEIGHT;
}
void wEui_statusBar_render(U8G2 *display, const wEui_DisplayConfig_t *displayConfig) {
    if (!g_statusBar_initialized || display == NULL || displayConfig == NULL) {
        return;
    }
    uint8_t statusBarY = displayConfig->height - WEUI_STATUS_BAR_HEIGHT;
    display->setDrawColor(1);
    bool showBorder = false;
    if (g_statusBarMutex != NULL) {
        xSemaphoreTake(g_statusBarMutex, pdMS_TO_TICKS(10));
    }
    showBorder = g_statusBar.showBorder;
    if (g_statusBarMutex != NULL) {
        xSemaphoreGive(g_statusBarMutex);
    }
    if (showBorder) {
        display->drawHLine(0, statusBarY, displayConfig->width);
    }
    uint8_t textX = 2;
    uint8_t textY = statusBarY + 2;
    static char statusText[WEUI_STATUS_BAR_LENGTH];
    if (g_statusBarMutex != NULL) {
        xSemaphoreTake(g_statusBarMutex, pdMS_TO_TICKS(10));
    }
    strncpy(statusText, g_statusBar.text, WEUI_STATUS_BAR_LENGTH - 1);
    statusText[WEUI_STATUS_BAR_LENGTH - 1] = '\0';
    if (g_statusBarMutex != NULL) {
        xSemaphoreGive(g_statusBarMutex);
    }
    if (statusText[0] != '\0') {
        display->drawStr(textX, textY, statusText);
    }
}
