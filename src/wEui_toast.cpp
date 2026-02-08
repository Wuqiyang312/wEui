#include "../include/wEui.h"
#include "../include/wEui_toast.h"
#include <string.h>

/**
 * @file wEui_toast.cpp
 * @brief wEui Toast/Dialog Implementation
 */

// ============================================================================
// Internal Variables
// ============================================================================

static wEui_ToastState_t g_toastState = {0};
static bool g_toastInitialized = false;
static SemaphoreHandle_t g_toastMutex = nullptr;

// 对话框状态
static struct {
    bool active;
    char title[WEUI_TOAST_TITLE_LENGTH];
    char message[WEUI_TOAST_LINE_LENGTH * 2];
    wEui_DialogButtonType_t buttonType;
    wEui_DialogCallback_t callback;
    uint8_t selectedButton;  // 0=左按钮，1=右按钮
} g_dialogState = {0};

// Toast滚动状态
static struct {
    int8_t scrollOffset;      // 当前滚动偏移
    int8_t maxScrollOffset;   // 最大滚动偏移
    uint32_t lastScrollTime;  // 上次自动滚动时间
    bool autoScroll;          // 是否自动滚动
} g_scrollState = {0, 0, 0, true};

// ============================================================================
// Internal Functions
// ============================================================================

/**
 * @brief 绘制Toast图标
 */
static void wEui_toast_drawIcon(U8G2 *display, uint8_t x, uint8_t y, wEui_ToastType_t type, uint8_t animFrame) {
    switch (type) {
        case WEUI_TOAST_SUCCESS:
            // 绘制对勾图标
            display->drawLine(x + 2, y + 5, x + 4, y + 7);
            display->drawLine(x + 4, y + 7, x + 8, y + 3);
            break;

        case WEUI_TOAST_ERROR:
            // 绘制X图标
            display->drawLine(x + 2, y + 2, x + 8, y + 8);
            display->drawLine(x + 8, y + 2, x + 2, y + 8);
            break;

        case WEUI_TOAST_WARNING:
            // 绘制三角形警告图标
            display->drawTriangle(x + 5, y + 1, x + 1, y + 9, x + 9, y + 9);
            display->setCursor(x + 4, y + 3);
            display->print("!");
            break;

        case WEUI_TOAST_LOADING:
            // 绘制旋转加载图标
            {
                uint8_t cx = x + 5;
                uint8_t cy = y + 5;
                uint8_t r = 4;
                // 绘制旋转的点
                for (int i = 0; i < 4; i++) {
                    int angle = (animFrame * 45 + i * 90) % 360;
                    int px = cx + (r * cos(angle * PI / 180));
                    int py = cy + (r * sin(angle * PI / 180));
                    if (i == 0) {
                        display->drawDisc(px, py, 2);
                    } else {
                        display->drawPixel(px, py);
                    }
                }
            }
            break;

        case WEUI_TOAST_INFO:
        default:
            // 绘制信息图标(i)
            display->drawCircle(x + 5, y + 5, 4);
            display->setCursor(x + 4, y + 2);
            display->print("i");
            break;
    }
}

/**
 * @brief 计算字符串显示宽度并截断
 */
static void wEui_toast_truncateText(U8G2 *display, const char *text, char *output, uint8_t maxWidth) {
    if (text == nullptr || output == nullptr) return;

    int textWidth = display->getStrWidth(text);
    if (textWidth <= maxWidth) {
        strcpy(output, text);
        return;
    }

    // 需要截断
    char temp[WEUI_TOAST_LINE_LENGTH];
    strncpy(temp, text, WEUI_TOAST_LINE_LENGTH - 4);
    temp[WEUI_TOAST_LINE_LENGTH - 4] = '\0';

    while (display->getStrWidth(temp) > maxWidth - display->getStrWidth("...") && strlen(temp) > 0) {
        temp[strlen(temp) - 1] = '\0';
    }

    strcpy(output, temp);
    strcat(output, "...");
}

// ============================================================================
// Toast Functions Implementation
// ============================================================================

int wEui_toast_init(void) {
    if (g_toastInitialized) {
        return 0;
    }

    g_toastMutex = xSemaphoreCreateMutex();
    if (g_toastMutex == nullptr) {
        return -1;
    }

    memset(&g_toastState, 0, sizeof(g_toastState));
    memset(&g_dialogState, 0, sizeof(g_dialogState));

    g_toastInitialized = true;
    return 0;
}

void wEui_toast_deinit(void) {
    if (!g_toastInitialized) return;

    if (g_toastMutex != nullptr) {
        vSemaphoreDelete(g_toastMutex);
        g_toastMutex = nullptr;
    }

    g_toastInitialized = false;
}

void wEui_toast_show(const char *message, uint32_t duration) {
    wEui_toast_showTyped(WEUI_TOAST_INFO, nullptr, message, duration);
}

void wEui_toast_showWithTitle(const char *title, const char *message, uint32_t duration) {
    wEui_toast_showTyped(WEUI_TOAST_INFO, title, message, duration);
}

void wEui_toast_showTyped(wEui_ToastType_t type, const char *title, const char *message, uint32_t duration) {
    if (!g_toastInitialized || message == nullptr) return;

    if (g_toastMutex != nullptr) {
        xSemaphoreTake(g_toastMutex, portMAX_DELAY);
    }

    memset(&g_toastState.config, 0, sizeof(g_toastState.config));

    g_toastState.config.type = type;
    g_toastState.config.position = WEUI_TOAST_POS_CENTER;
    g_toastState.config.showBorder = true;
    g_toastState.config.showIcon = true;
    g_toastState.config.blockInput = (type == WEUI_TOAST_LOADING);
    g_toastState.config.showProgress = false;
    g_toastState.config.duration = (duration > 0) ? duration : WEUI_TOAST_DEFAULT_DURATION;

    // 设置标题
    if (title != nullptr) {
        strncpy(g_toastState.config.title, title, WEUI_TOAST_TITLE_LENGTH - 1);
        g_toastState.config.title[WEUI_TOAST_TITLE_LENGTH - 1] = '\0';
    } else {
        g_toastState.config.title[0] = '\0';
    }

    // 设置消息内容（单行）
    strncpy(g_toastState.config.lines[0], message, WEUI_TOAST_LINE_LENGTH - 1);
    g_toastState.config.lines[0][WEUI_TOAST_LINE_LENGTH - 1] = '\0';
    g_toastState.config.lineCount = 1;

    g_toastState.active = true;
    g_toastState.startTime = millis();
    g_toastState.animFrame = 0;

    if (g_toastMutex != nullptr) {
        xSemaphoreGive(g_toastMutex);
    }
}

void wEui_toast_showMultiLine(const char *title, const char **lines, uint8_t lineCount, uint32_t duration) {
    if (!g_toastInitialized || lines == nullptr || lineCount == 0) return;

    if (g_toastMutex != nullptr) {
        xSemaphoreTake(g_toastMutex, portMAX_DELAY);
    }

    memset(&g_toastState.config, 0, sizeof(g_toastState.config));

    g_toastState.config.type = WEUI_TOAST_INFO;
    g_toastState.config.position = WEUI_TOAST_POS_CENTER;
    g_toastState.config.showBorder = true;
    g_toastState.config.showIcon = false;  // 多行不显示图标
    g_toastState.config.blockInput = false;
    g_toastState.config.duration = (duration > 0) ? duration : WEUI_TOAST_DEFAULT_DURATION;

    // 设置标题
    if (title != nullptr) {
        strncpy(g_toastState.config.title, title, WEUI_TOAST_TITLE_LENGTH - 1);
        g_toastState.config.title[WEUI_TOAST_TITLE_LENGTH - 1] = '\0';
    } else {
        g_toastState.config.title[0] = '\0';
    }

    // 设置多行消息
    uint8_t actualLines = (lineCount > WEUI_TOAST_MAX_LINES) ? WEUI_TOAST_MAX_LINES : lineCount;
    for (uint8_t i = 0; i < actualLines; i++) {
        if (lines[i] != nullptr) {
            strncpy(g_toastState.config.lines[i], lines[i], WEUI_TOAST_LINE_LENGTH - 1);
            g_toastState.config.lines[i][WEUI_TOAST_LINE_LENGTH - 1] = '\0';
        }
    }
    g_toastState.config.lineCount = actualLines;

    g_toastState.active = true;
    g_toastState.startTime = millis();
    g_toastState.animFrame = 0;

    // 重置滚动状态
    g_scrollState.scrollOffset = 0;
    g_scrollState.maxScrollOffset = 0;
    g_scrollState.lastScrollTime = millis();
    g_scrollState.autoScroll = true;

    if (g_toastMutex != nullptr) {
        xSemaphoreGive(g_toastMutex);
    }
}

void wEui_toast_success(const char *message) {
    wEui_toast_showTyped(WEUI_TOAST_SUCCESS, "成功", message, WEUI_TOAST_DEFAULT_DURATION);
}

void wEui_toast_error(const char *message) {
    wEui_toast_showTyped(WEUI_TOAST_ERROR, "错误", message, WEUI_TOAST_DEFAULT_DURATION);
}

void wEui_toast_warning(const char *message) {
    wEui_toast_showTyped(WEUI_TOAST_WARNING, "警告", message, WEUI_TOAST_DEFAULT_DURATION);
}

void wEui_toast_loading(const char *message) {
    if (!g_toastInitialized) return;

    if (g_toastMutex != nullptr) {
        xSemaphoreTake(g_toastMutex, portMAX_DELAY);
    }

    memset(&g_toastState.config, 0, sizeof(g_toastState.config));

    g_toastState.config.type = WEUI_TOAST_LOADING;
    g_toastState.config.position = WEUI_TOAST_POS_CENTER;
    g_toastState.config.showBorder = true;
    g_toastState.config.showIcon = true;
    g_toastState.config.blockInput = true;  // 加载时阻塞输入
    g_toastState.config.duration = 0;  // 需要手动关闭
    g_toastState.config.showProgress = true;
    g_toastState.config.progress = 0;

    strncpy(g_toastState.config.title, "请稍候", WEUI_TOAST_TITLE_LENGTH - 1);

    if (message != nullptr) {
        strncpy(g_toastState.config.lines[0], message, WEUI_TOAST_LINE_LENGTH - 1);
        g_toastState.config.lines[0][WEUI_TOAST_LINE_LENGTH - 1] = '\0';
    } else {
        strcpy(g_toastState.config.lines[0], "加载中...");
    }
    g_toastState.config.lineCount = 1;

    g_toastState.active = true;
    g_toastState.startTime = millis();
    g_toastState.animFrame = 0;

    if (g_toastMutex != nullptr) {
        xSemaphoreGive(g_toastMutex);
    }
}

void wEui_toast_updateProgress(uint8_t progress, const char *message) {
    if (!g_toastInitialized || !g_toastState.active) return;

    if (g_toastMutex != nullptr) {
        xSemaphoreTake(g_toastMutex, portMAX_DELAY);
    }

    g_toastState.config.progress = (progress > 100) ? 100 : progress;

    if (message != nullptr) {
        strncpy(g_toastState.config.lines[0], message, WEUI_TOAST_LINE_LENGTH - 1);
        g_toastState.config.lines[0][WEUI_TOAST_LINE_LENGTH - 1] = '\0';
    }

    if (g_toastMutex != nullptr) {
        xSemaphoreGive(g_toastMutex);
    }
}

void wEui_toast_hide(void) {
    if (!g_toastInitialized) return;

    if (g_toastMutex != nullptr) {
        xSemaphoreTake(g_toastMutex, portMAX_DELAY);
    }

    g_toastState.active = false;

    if (g_toastMutex != nullptr) {
        xSemaphoreGive(g_toastMutex);
    }
}

bool wEui_toast_isActive(void) {
    return g_toastState.active;
}

int wEui_toast_update(void) {
    if (!g_toastInitialized || !g_toastState.active) {
        return 0;
    }

    if (g_toastMutex != nullptr) {
        xSemaphoreTake(g_toastMutex, portMAX_DELAY);
    }

    // 更新动画帧
    g_toastState.animFrame++;

    // 检查是否超时
    if (g_toastState.config.duration > 0) {
        uint32_t elapsed = millis() - g_toastState.startTime;
        if (elapsed >= g_toastState.config.duration) {
            g_toastState.active = false;
        }
    }

    if (g_toastMutex != nullptr) {
        xSemaphoreGive(g_toastMutex);
    }

    return 0;
}

int wEui_toast_render(U8G2 *display, const wEui_DisplayConfig_t *displayConfig) {
    if (!g_toastInitialized || display == nullptr || displayConfig == nullptr) {
        return -1;
    }

    if (!g_toastState.active && !g_dialogState.active) {
        return 0;
    }

    // 优先渲染对话框
    if (g_dialogState.active) {
        // 计算对话框尺寸
        uint8_t dialogWidth = displayConfig->width - 16;
        uint8_t dialogHeight = 48;
        uint8_t dialogX = (displayConfig->width - dialogWidth) / 2;
        uint8_t dialogY = (displayConfig->height - dialogHeight) / 2;

        // 绘制半透明背景效果（用棋盘格模拟）
        for (uint8_t y = 0; y < displayConfig->height; y += 2) {
            for (uint8_t x = (y / 2) % 2; x < displayConfig->width; x += 2) {
                display->setDrawColor(0);
                display->drawPixel(x, y);
            }
        }

        // 绘制对话框背景
        display->setDrawColor(0);
        display->drawBox(dialogX, dialogY, dialogWidth, dialogHeight);
        display->setDrawColor(1);
        display->drawFrame(dialogX, dialogY, dialogWidth, dialogHeight);

        // 绘制标题
        if (strlen(g_dialogState.title) > 0) {
            display->setCursor(dialogX + 4, dialogY + 2);
            display->print(g_dialogState.title);
            display->drawHLine(dialogX + 1, dialogY + displayConfig->lineHeight, dialogWidth - 2);
        }

        // 绘制消息
        uint8_t msgY = dialogY + displayConfig->lineHeight + 4;
        display->setCursor(dialogX + 4, msgY);
        display->print(g_dialogState.message);

        // 绘制按钮
        uint8_t btnY = dialogY + dialogHeight - displayConfig->lineHeight - 4;
        uint8_t btnWidth = 36;
        uint8_t btnSpacing = 8;

        const char *btn1Text = nullptr;
        const char *btn2Text = nullptr;

        switch (g_dialogState.buttonType) {
            case WEUI_DIALOG_BTN_OK:
                btn1Text = "确定";
                break;
            case WEUI_DIALOG_BTN_OK_CANCEL:
                btn1Text = "取消";
                btn2Text = "确定";
                break;
            case WEUI_DIALOG_BTN_YES_NO:
                btn1Text = "否";
                btn2Text = "是";
                break;
        }

        if (g_dialogState.buttonType == WEUI_DIALOG_BTN_OK) {
            // 单按钮居中
            uint8_t btnX = dialogX + (dialogWidth - btnWidth) / 2;
            if (g_dialogState.selectedButton == 0) {
                display->drawBox(btnX, btnY, btnWidth, displayConfig->lineHeight);
                display->setDrawColor(0);
            } else {
                display->drawFrame(btnX, btnY, btnWidth, displayConfig->lineHeight);
            }
            display->setCursor(btnX + 8, btnY + 1);
            display->print(btn1Text);
            display->setDrawColor(1);
        } else {
            // 双按钮
            uint8_t totalWidth = btnWidth * 2 + btnSpacing;
            uint8_t startX = dialogX + (dialogWidth - totalWidth) / 2;

            // 按钮1（左）
            if (g_dialogState.selectedButton == 0) {
                display->drawBox(startX, btnY, btnWidth, displayConfig->lineHeight);
                display->setDrawColor(0);
            } else {
                display->drawFrame(startX, btnY, btnWidth, displayConfig->lineHeight);
            }
            display->setCursor(startX + 8, btnY + 1);
            display->print(btn1Text);
            display->setDrawColor(1);

            // 按钮2（右）
            uint8_t btn2X = startX + btnWidth + btnSpacing;
            if (g_dialogState.selectedButton == 1) {
                display->drawBox(btn2X, btnY, btnWidth, displayConfig->lineHeight);
                display->setDrawColor(0);
            } else {
                display->drawFrame(btn2X, btnY, btnWidth, displayConfig->lineHeight);
            }
            display->setCursor(btn2X + 8, btnY + 1);
            display->print(btn2Text);
            display->setDrawColor(1);
        }

        return 0;
    }

    // 渲染Toast
    if (g_toastState.active) {
        // 计算Toast尺寸
        uint8_t hasTitle = (strlen(g_toastState.config.title) > 0) ? 1 : 0;
        uint8_t lineCount = g_toastState.config.lineCount;
        uint8_t hasProgress = g_toastState.config.showProgress ? 1 : 0;

        // 计算内容所需高度
        uint8_t contentHeight = WEUI_TOAST_PADDING * 2 +
                              (hasTitle ? displayConfig->lineHeight + 2 : 0) +
                              (lineCount * displayConfig->lineHeight) +
                              (hasProgress ? 8 : 0);

        // 限制最大高度为屏幕高度的80%
        uint8_t maxToastHeight = displayConfig->height * 80 / 100;
        uint8_t toastHeight = (contentHeight > maxToastHeight) ? maxToastHeight : contentHeight;

        // 计算可见行数
        uint8_t headerHeight = WEUI_TOAST_PADDING + (hasTitle ? displayConfig->lineHeight + 2 : 0);
        uint8_t availableHeight = toastHeight - headerHeight - WEUI_TOAST_PADDING - (hasProgress ? 8 : 0);
        uint8_t visibleLines = availableHeight / displayConfig->lineHeight;
        if (visibleLines < 1) visibleLines = 1;
        if (visibleLines > lineCount) visibleLines = lineCount;

        // 计算最大滚动偏移
        g_scrollState.maxScrollOffset = (lineCount > visibleLines) ? (lineCount - visibleLines) : 0;

        // 自动滚动逻辑
        if (g_scrollState.autoScroll && g_scrollState.maxScrollOffset > 0) {
            uint32_t now = millis();
            if (now - g_scrollState.lastScrollTime > 1500) {  // 每1.5秒滚动一行
                g_scrollState.scrollOffset++;
                if (g_scrollState.scrollOffset > g_scrollState.maxScrollOffset) {
                    g_scrollState.scrollOffset = 0;  // 循环滚动
                }
                g_scrollState.lastScrollTime = now;
            }
        }

        // 确保scrollOffset在有效范围内
        if (g_scrollState.scrollOffset > g_scrollState.maxScrollOffset) {
            g_scrollState.scrollOffset = g_scrollState.maxScrollOffset;
        }
        if (g_scrollState.scrollOffset < 0) {
            g_scrollState.scrollOffset = 0;
        }

        uint8_t toastWidth = displayConfig->width - 16;
        uint8_t toastX = (displayConfig->width - toastWidth) / 2;
        uint8_t toastY;

        // 根据位置计算Y坐标
        switch (g_toastState.config.position) {
            case WEUI_TOAST_POS_TOP:
                toastY = 4;
                break;
            case WEUI_TOAST_POS_BOTTOM:
                toastY = displayConfig->height - toastHeight - 16;  // 留空间给状态栏
                break;
            case WEUI_TOAST_POS_CENTER:
            default:
                toastY = (displayConfig->height - toastHeight) / 2;
                break;
        }

        // 绘制半透明背景效果
        for (uint8_t y = 0; y < displayConfig->height; y += 2) {
            for (uint8_t x = (y / 2) % 2; x < displayConfig->width; x += 2) {
                display->setDrawColor(0);
                display->drawPixel(x, y);
            }
        }

        // 绘制Toast背景
        display->setDrawColor(0);
        display->drawBox(toastX, toastY, toastWidth, toastHeight);
        display->setDrawColor(1);

        if (g_toastState.config.showBorder) {
            display->drawFrame(toastX, toastY, toastWidth, toastHeight);
        }

        uint8_t contentX = toastX + WEUI_TOAST_PADDING;
        uint8_t contentY = toastY + WEUI_TOAST_PADDING;
        uint8_t contentWidth = toastWidth - WEUI_TOAST_PADDING * 2;

        // 绘制图标
        uint8_t textStartX = contentX;
        if (g_toastState.config.showIcon) {
            wEui_toast_drawIcon(display, contentX, contentY,
                               g_toastState.config.type, g_toastState.animFrame);
            textStartX = contentX + 14;
            contentWidth -= 14;
        }

        // 绘制标题
        if (hasTitle) {
            display->setCursor(textStartX, contentY);
            display->print(g_toastState.config.title);
            contentY += displayConfig->lineHeight + 2;

            // 绘制分割线
            display->drawHLine(toastX + 2, contentY - 1, toastWidth - 4);
        }

        // 绘制可见的内容行（带滚动）
        for (uint8_t i = 0; i < visibleLines && (i + g_scrollState.scrollOffset) < lineCount; i++) {
            uint8_t lineIndex = i + g_scrollState.scrollOffset;
            display->setCursor(textStartX, contentY + i * displayConfig->lineHeight);
            display->print(g_toastState.config.lines[lineIndex]);
        }

        // 如果有滚动，显示滚动指示器
        if (g_scrollState.maxScrollOffset > 0) {
            uint8_t indicatorX = toastX + toastWidth - 4;
            uint8_t indicatorY = contentY;
            uint8_t indicatorHeight = visibleLines * displayConfig->lineHeight;

            // 绘制滚动条背景
            display->drawVLine(indicatorX, indicatorY, indicatorHeight);

            // 绘制滚动条位置
            uint8_t thumbHeight = indicatorHeight / (g_scrollState.maxScrollOffset + 1);
            if (thumbHeight < 3) thumbHeight = 3;
            uint8_t thumbY = indicatorY;
            if (g_scrollState.maxScrollOffset > 0) {
                thumbY = indicatorY + (indicatorHeight - thumbHeight) * g_scrollState.scrollOffset / g_scrollState.maxScrollOffset;
            }
            display->drawBox(indicatorX - 1, thumbY, 3, thumbHeight);
        }

        // 绘制进度条
        if (hasProgress) {
            uint8_t progressY = toastY + toastHeight - 10;
            uint8_t progressWidth = toastWidth - 8;
            uint8_t progressX = toastX + 4;

            display->drawFrame(progressX, progressY, progressWidth, 6);
            uint8_t filledWidth = (progressWidth - 2) * g_toastState.config.progress / 100;
            if (filledWidth > 0) {
                display->drawBox(progressX + 1, progressY + 1, filledWidth, 4);
            }
        }
    }

    return 0;
}

void wEui_toast_setBlockInput(bool block) {
    if (g_toastMutex != nullptr) {
        xSemaphoreTake(g_toastMutex, portMAX_DELAY);
    }
    g_toastState.config.blockInput = block;
    if (g_toastMutex != nullptr) {
        xSemaphoreGive(g_toastMutex);
    }
}

bool wEui_toast_isBlockingInput(void) {
    if (g_toastState.active && g_toastState.config.blockInput) {
        return true;
    }
    if (g_dialogState.active) {
        return true;
    }
    return false;
}

const wEui_ToastConfig_t* wEui_toast_getConfig(void) {
    return &g_toastState.config;
}

// ============================================================================
// Dialog Functions Implementation
// ============================================================================

void wEui_dialog_show(const char *title, const char *message,
                      wEui_DialogButtonType_t buttonType,
                      wEui_DialogCallback_t callback) {
    if (!g_toastInitialized || message == nullptr) return;

    if (g_toastMutex != nullptr) {
        xSemaphoreTake(g_toastMutex, portMAX_DELAY);
    }

    // 关闭当前的toast
    g_toastState.active = false;

    // 设置对话框
    if (title != nullptr) {
        strncpy(g_dialogState.title, title, WEUI_TOAST_TITLE_LENGTH - 1);
        g_dialogState.title[WEUI_TOAST_TITLE_LENGTH - 1] = '\0';
    } else {
        g_dialogState.title[0] = '\0';
    }

    strncpy(g_dialogState.message, message, sizeof(g_dialogState.message) - 1);
    g_dialogState.message[sizeof(g_dialogState.message) - 1] = '\0';

    g_dialogState.buttonType = buttonType;
    g_dialogState.callback = callback;
    g_dialogState.selectedButton = (buttonType == WEUI_DIALOG_BTN_OK) ? 0 : 1;  // 默认选中确定/是
    g_dialogState.active = true;

    if (g_toastMutex != nullptr) {
        xSemaphoreGive(g_toastMutex);
    }
}

bool wEui_dialog_isActive(void) {
    return g_dialogState.active;
}

int wEui_dialog_handleButton(uint8_t button) {
    if (!g_dialogState.active) {
        return -1;
    }

    // button: 0=UP, 1=DOWN, 2=OK, 3=BACK
    switch (button) {
        case 0:  // UP - 不处理
        case 1:  // DOWN - 不处理
            break;

        case 2:  // OK - 确认选择
            {
                uint8_t result = g_dialogState.selectedButton;
                wEui_DialogCallback_t callback = g_dialogState.callback;
                wEui_dialog_close();
                if (callback != nullptr) {
                    callback(result);
                }
            }
            return 0;

        case 3:  // BACK - 取消/切换按钮
            if (g_dialogState.buttonType != WEUI_DIALOG_BTN_OK) {
                // 切换按钮选择
                g_dialogState.selectedButton = 1 - g_dialogState.selectedButton;
            } else {
                // 只有确定按钮时，BACK等同于确定
                wEui_DialogCallback_t callback = g_dialogState.callback;
                wEui_dialog_close();
                if (callback != nullptr) {
                    callback(0);
                }
            }
            return 0;
    }

    return -1;
}

void wEui_dialog_close(void) {
    if (g_toastMutex != nullptr) {
        xSemaphoreTake(g_toastMutex, portMAX_DELAY);
    }

    g_dialogState.active = false;
    g_dialogState.callback = nullptr;

    if (g_toastMutex != nullptr) {
        xSemaphoreGive(g_toastMutex);
    }
}

// ============================================================================
// Toast Scroll Functions
// ============================================================================

void wEui_toast_scroll(int8_t direction) {
    if (!g_toastState.active || g_scrollState.maxScrollOffset == 0) return;

    g_scrollState.autoScroll = false;  // 手动操作后停止自动滚动
    g_scrollState.scrollOffset += direction;

    if (g_scrollState.scrollOffset < 0) {
        g_scrollState.scrollOffset = 0;
    }
    if (g_scrollState.scrollOffset > g_scrollState.maxScrollOffset) {
        g_scrollState.scrollOffset = g_scrollState.maxScrollOffset;
    }
}

bool wEui_toast_canScroll(void) {
    return g_toastState.active && g_scrollState.maxScrollOffset > 0;
}

