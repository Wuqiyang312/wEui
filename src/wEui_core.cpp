#include "../include/wEui.h"
#include "../include/wEui_statusbar.h"
#include <string.h>

/**
 * @file wEui_core.cpp
 * @brief wEui Core Implementation
 */

// ============================================================================
// Internal Variables
// ============================================================================

// 模块级状态用于跟踪初始化、显示和按键资源
static bool g_wEui_initialized = false;
static U8G2 *g_display = nullptr;
static wEui_DisplayConfig_t g_displayConfig = {0};
static wEui_ButtonConfig_t g_buttonConfig = {0};
static QueueHandle_t g_buttonQueue = nullptr;
static SemaphoreHandle_t g_listMutex = nullptr;
// 当前根据显示高度计算出来的可见行数
static uint8_t g_actualVisibleLines = WEUI_VISIBLE_LINES;  // Actual visible lines based on display height

// Library version
static const char* WEUI_VERSION = "1.0.0";

// ============================================================================
// Internal Functions
// ============================================================================

/**
 * @brief 渲染列表页面内容
 * @param contentMaxHeight 内容区域最大高度
 */
static void wEui_render_listPage(uint8_t contentMaxHeight);

// ============================================================================
// Core Functions Implementation
// ============================================================================

int wEui_init(const wEui_Config_t *config) {
    if (config == nullptr) {
        return -1;
    }

    if (g_wEui_initialized) {
        return 0; // Already initialized
    }

    // 记住传入的显示/按键/列表互斥配置以供后续使用
    g_display = config->display;
    g_displayConfig = config->displayConfig;
    g_buttonConfig = config->buttonConfig;
    g_buttonQueue = config->buttonQueue;
    g_listMutex = config->listMutex;

    // 确保状态栏模块提前初始化
    if (wEui_statusBar_init() != 0) {
        Serial.println("wEui: Status bar initialization failed!");
        return -5;
    }

    // 初始化页面管理系统
    if (wEui_page_init() != 0) {
        Serial.println("wEui: Page management initialization failed!");
        return -6;
    }

    // 初始化Toast/Dialog系统
    if (wEui_toast_init() != 0) {
        Serial.println("wEui: Toast system initialization failed!");
        return -7;
    }

    // 初始化显示设备并开启 UTF-8 中文输出
    if (g_display != nullptr) {
        // Initialize display hardware
        if (!g_display->begin()) {
            Serial.println("wEui: Display initialization failed!");
            return -4;
        }

        // Enable UTF-8 mode for Chinese characters
        g_display->enableUTF8Print();

        // Set display properties
        g_display->setFont(g_displayConfig.font);
        g_display->setFontRefHeightExtendedText();
        g_display->setDrawColor(1);
        g_display->setFontPosTop();
        g_display->setFontDirection(0);
        g_display->clearBuffer();

        // Test display by sending buffer
        g_display->sendBuffer();

        Serial.println("wEui: Display initialized successfully");
        Serial.print("wEui: Font configured: ");
        Serial.println((g_displayConfig.font != nullptr) ? "OK" : "nullptr");
    } else {
        Serial.println("wEui: Warning - No display configured");
    }

    // 配置按键引脚与事件队列并启动按键模块
    if (wEui_button_setPinConfig(&g_buttonConfig) != 0) {
        return -2;
    }

    // Set button queue
    wEui_button_setButtonQueue(g_buttonQueue);

    // Initialize button system
    if (wEui_button_init() != 0) {
        return -3;
    }

    g_wEui_initialized = true;
    return 0;
}

int wEui_deinit(void) {
    if (!g_wEui_initialized) {
        return 0;
    }

    wEui_button_deinit();

    g_display = nullptr;
    g_buttonQueue = nullptr;
    g_listMutex = nullptr;

    // Clean up toast system
    wEui_toast_deinit();

    // Clean up status bar module
    wEui_statusBar_deinit();

    g_wEui_initialized = false;

    return 0;
}

int wEui_begin(void) {
    if (!g_wEui_initialized || g_display == nullptr) {
        return -1;
    }

    // Clear display and show initialization message
    g_display->clearBuffer();
    g_display->setDrawColor(1);
    g_display->setFont(g_displayConfig.font);

    // Draw startup screen using print for UTF-8 support
    g_display->setCursor(0, 0);
    g_display->print("wEui v1.0.0");
    g_display->setCursor(0, 16);
    g_display->print("就绪!");

    g_display->sendBuffer();

    Serial.println("wEui: Begin completed successfully");
    return 0;
}

/**
 * @brief 渲染列表页面内容
 * @param contentMaxHeight 内容区域最大高度
 */
static void wEui_render_listPage(uint8_t contentMaxHeight) {
    // 获取列表状态用于后续绘制
    uint8_t itemCount = wEui_list_getItemCount();
    uint8_t topIndex = wEui_list_getTopIndex();
    uint8_t cursorPos = wEui_list_getCursorPosition();

    if (itemCount == 0) {
        // 空列表时显示中文占位提示
        g_display->setDrawColor(1);
        g_display->setFont(g_displayConfig.font);
        const char* emptyMsg = "空";
        int msgWidth = g_display->getStrWidth(emptyMsg);
        g_display->setCursor((g_displayConfig.width - msgWidth) / 2,
                          (contentMaxHeight - g_displayConfig.lineHeight) / 2);
        g_display->print(emptyMsg);
    } else {
        // 绘制带滚动条的列表区域
        uint8_t scrollbarX = g_displayConfig.width - WEUI_SCROLLBAR_WIDTH;
        uint8_t contentAreaWidth = g_displayConfig.width - WEUI_SCROLLBAR_WIDTH - 2;

        g_display->setDrawColor(1);
        g_display->drawFrame(0, 0, contentAreaWidth, contentMaxHeight);

        wEui_list_renderScrollbar(g_display, scrollbarX, 0, WEUI_SCROLLBAR_WIDTH, contentMaxHeight);

        uint8_t contentX = 2;
        uint8_t contentY = 2;
        uint8_t contentWidth = contentAreaWidth - 4;

        g_display->setFont(g_displayConfig.font);

        uint8_t maxVisibleItems = g_actualVisibleLines;

        for (uint8_t i = 0; i < maxVisibleItems && (topIndex + i) < itemCount; i++) {
            uint8_t itemIndex = topIndex + i;
            uint8_t yPos = contentY + (i * g_displayConfig.lineHeight);

            if (yPos + g_displayConfig.lineHeight > contentMaxHeight - 2) {
                break;
            }

            bool isSelected = (i == cursorPos);

            if (isSelected) {
                // 选中项用背景和箭头高亮
                g_display->setDrawColor(1);
                g_display->drawBox(contentX, yPos - 1,
                                  contentWidth, g_displayConfig.lineHeight);

                g_display->setDrawColor(0);
                g_display->setCursor(contentX + 1, yPos);
                g_display->print(">");
            } else {
                g_display->setDrawColor(1);
            }

            const char* itemName = wEui_list_getItemName(itemIndex);
            if (itemName && strlen(itemName) > 0) {
                g_display->setCursor(contentX + 8, yPos);
                g_display->print(itemName);
            }

            g_display->setDrawColor(1);
        }
    }
}

int wEui_render(void) {
    if (!g_wEui_initialized || g_display == nullptr) {
        return -1;
    }

    g_display->clearBuffer();

    // 为状态栏预留底部高度空间
    uint8_t statusBarHeight = wEui_statusBar_getHeight();
    uint8_t contentMaxHeight = g_displayConfig.height - statusBarHeight;

    // 根据可用高度计算实际可见行数，确保不超过最大值
    uint8_t availableContentHeight = contentMaxHeight - 4;
    g_actualVisibleLines = availableContentHeight / g_displayConfig.lineHeight;

    if (g_actualVisibleLines == 0) {
        g_actualVisibleLines = 1;
    }
    if (g_actualVisibleLines > WEUI_VISIBLE_LINES) {
        g_actualVisibleLines = WEUI_VISIBLE_LINES;
    }

    // 获取当前页面信息
    int currentPageId = -1;
    wEui_PageType_t pageType = WEUI_PAGE_TYPE_LIST;

    if (wEui_page_getCurrentRenderInfo(&currentPageId, &pageType) == 0 && currentPageId >= 0) {
        // 渲染当前活动页面
        if (pageType == WEUI_PAGE_TYPE_LIST) {
            // 渲染列表页面（使用现有的列表渲染逻辑）
            wEui_render_listPage(contentMaxHeight);
        } else if (pageType == WEUI_PAGE_TYPE_CUSTOM) {
            // 渲染自定义页面
            wEui_page_renderCustom(currentPageId, g_display, &g_displayConfig, contentMaxHeight);
        }
    } else {
        // 没有活动页面时显示默认内容
        g_display->setDrawColor(1);
        g_display->setFont(g_displayConfig.font);
        const char* noPageMsg = "无页面";
        int msgWidth = g_display->getStrWidth(noPageMsg);
        g_display->setCursor((g_displayConfig.width - msgWidth) / 2,
                          (contentMaxHeight - g_displayConfig.lineHeight) / 2);
        g_display->print(noPageMsg);
    }

    // 状态栏始终在底部渲染
    wEui_statusBar_render(g_display, &g_displayConfig);

    // 更新并渲染Toast/Dialog（最后渲染，覆盖在最上层）
    wEui_toast_update();
    wEui_toast_render(g_display, &g_displayConfig);

    return 0;
}

int wEui_update(void) {
    if (!g_wEui_initialized || g_display == nullptr) {
        return -1;
    }

    g_display->sendBuffer();
    return 0;
}

// ============================================================================
// Event Processing Functions
// ============================================================================

int wEui_processButtonEvents(uint32_t timeout) {
    if (g_buttonQueue == nullptr) {
        return -1;
    }

    const char* receivedBtn;
    if (xQueueReceive(g_buttonQueue, &receivedBtn, timeout) == pdPASS) {

        // 优先处理对话框按键事件
        if (wEui_dialog_isActive()) {
            if (strcmp(receivedBtn, "UP") == 0) {
                wEui_dialog_handleButton(0);
            } else if (strcmp(receivedBtn, "DOWN") == 0) {
                wEui_dialog_handleButton(1);
            } else if (strcmp(receivedBtn, "OK") == 0) {
                wEui_dialog_handleButton(2);
            } else if (strcmp(receivedBtn, "BACK") == 0) {
                wEui_dialog_handleButton(3);
            }
            return 0;
        }

        // 如果Toast正在阻塞输入，只允许特定按键关闭
        if (wEui_toast_isBlockingInput()) {
            if (strcmp(receivedBtn, "OK") == 0 || strcmp(receivedBtn, "BACK") == 0) {
                // 仅非loading类型的toast可以通过按键关闭
                const wEui_ToastConfig_t* config = wEui_toast_getConfig();
                if (config != nullptr && config->type != WEUI_TOAST_LOADING) {
                    wEui_toast_hide();
                }
            }
            return 0;
        }

        // 如果有普通Toast显示，任意按键关闭
        if (wEui_toast_isActive()) {
            wEui_toast_hide();
            return 0;
        }

        // 检查是否为长按事件
        if (strstr(receivedBtn, "LONG_") == receivedBtn) {
            // Handle long press events
            if (strcmp(receivedBtn, "LONG_UP") == 0) {
                wEui_list_pageUp();
            } else if (strcmp(receivedBtn, "LONG_DOWN") == 0) {
                wEui_list_pageDown();
            } else if (strcmp(receivedBtn, "LONG_OK") == 0) {
                wEui_list_moveToFirst(); // Long OK goes to first item
            } else if (strcmp(receivedBtn, "LONG_BACK") == 0) {
                wEui_list_moveToLast(); // Long Back goes to last item
            }
        } else {
            // 处理普通按键事件
            if (strcmp(receivedBtn, "UP") == 0) {
                wEui_list_moveUp();
            } else if (strcmp(receivedBtn, "DOWN") == 0) {
                wEui_list_moveDown();
            } else if (strcmp(receivedBtn, "OK") == 0) {
                wEui_list_executeSelected();
            } else if (strcmp(receivedBtn, "BACK") == 0) {
                // Handle back button - can be customized
                Serial.println("Back button pressed");
            }
        }
        return 0;
    }

    return -1; // Timeout or error
}

void wEui_setDefaultButtonHandlers(void) {
    // 当前默认处理由 processButtonEvents 统一管理，留作未来扩展
    // This function is for future extensibility
}

// ============================================================================
// Utility Functions
// ============================================================================

const char* wEui_getVersion(void) {
    return WEUI_VERSION;
}

const wEui_DisplayConfig_t* wEui_getDisplayConfig(void) {
    return &g_displayConfig;
}

void wEui_setFont(const uint8_t *font) {
    if (g_display != nullptr && font != nullptr) {
        g_display->setFont(font);
        g_displayConfig.font = font;
    }
}

uint8_t wEui_list_getActualVisibleLines(void) {
    // 返回根据当前布局计算出来的实际可见行数
    return g_actualVisibleLines;
}

