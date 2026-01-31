#include "../include/wEui.h"
#include <string.h>

/**
 * @file wEui_list.cpp
 * @brief wEui List Management Implementation
 */

// ============================================================================
// Internal Variables
// ============================================================================

// 列表缓冲与状态数据
typedef struct {
    wEui_ListItem_t items[WEUI_MAX_ITEMS];
    uint8_t itemCount;
    uint8_t topIndex;
    uint8_t selectedIndex;
    SemaphoreHandle_t mutex;
} wEui_ListManager_t;

static wEui_ListManager_t g_listManager = {0};
static bool g_listInitialized = false;

// ============================================================================
// Internal Functions
// ============================================================================

static void wEui_list_adjustTopIndex(void) {
    // 获取当前可见行
    uint8_t actualVisibleLines = wEui_list_getActualVisibleLines();

    // 固定光标位置，尽量保持选中项在屏幕中间
    uint8_t targetCursorPos = WEUI_FIXED_CURSOR_POS;

    // 如果实际可见行数不足，就在中间显示选中项
    if (targetCursorPos >= actualVisibleLines) {
        targetCursorPos = actualVisibleLines / 2;  // Use middle position
    }

    // 如果条目数少于可见行数，从顶部开始
    if (g_listManager.itemCount <= actualVisibleLines) {
        g_listManager.topIndex = 0;
        return;
    }

    // 计算理想的顶部索引以保持光标在固定位置
    int16_t idealTopIndex = g_listManager.selectedIndex - targetCursorPos;

    // 限制在有效范围内
    if (idealTopIndex < 0) {
        g_listManager.topIndex = 0;
    } else if (idealTopIndex > (int16_t)(g_listManager.itemCount - actualVisibleLines)) {
        g_listManager.topIndex = g_listManager.itemCount - actualVisibleLines;
    } else {
        g_listManager.topIndex = idealTopIndex;
    }
}

// ============================================================================
// List Management Functions Implementation
// ============================================================================

void wEui_list_init(SemaphoreHandle_t mutex) {
    if (g_listInitialized) {
        return;
    }

    // 重置列表状态并存储互斥量
    memset(&g_listManager, 0, sizeof(g_listManager));
    g_listManager.mutex = mutex;
    g_listInitialized = true;
}

bool wEui_list_addItem(const char *itemName, wEui_ItemCallback_t callback) {
    if (!itemName || g_listManager.itemCount >= WEUI_MAX_ITEMS) {
        return false;
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    // 写入条目信息并递增计数
    strncpy(g_listManager.items[g_listManager.itemCount].name, itemName,
            WEUI_ITEM_NAME_LENGTH - 1);
    g_listManager.items[g_listManager.itemCount].name[WEUI_ITEM_NAME_LENGTH - 1] = '\0';
    g_listManager.items[g_listManager.itemCount].onClicked = callback;
    g_listManager.itemCount++;

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }

    return true;
}

bool wEui_list_removeLast(void) {
    if (g_listManager.itemCount == 0) {
        return false;
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    // 删除最后一个条目并处理索引回退
    g_listManager.itemCount--;
    memset(&g_listManager.items[g_listManager.itemCount], 0, sizeof(wEui_ListItem_t));

    // 调整选中索引
    if (g_listManager.selectedIndex >= g_listManager.itemCount && g_listManager.itemCount > 0) {
        g_listManager.selectedIndex = g_listManager.itemCount - 1;
    }

    // 调整顶部索引
    uint8_t actualVisibleLines = wEui_list_getActualVisibleLines();
    if (g_listManager.topIndex > 0 && g_listManager.itemCount <= actualVisibleLines) {
        g_listManager.topIndex = 0;
    } else if (g_listManager.topIndex + actualVisibleLines > g_listManager.itemCount) {
        g_listManager.topIndex = (g_listManager.itemCount > actualVisibleLines) ?
                                 g_listManager.itemCount - actualVisibleLines : 0;
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }

    return true;
}

void wEui_list_clear(void) {
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    // 清理所有条目并重置游标
    memset(g_listManager.items, 0, sizeof(g_listManager.items));
    g_listManager.itemCount = 0;
    g_listManager.topIndex = 0;
    g_listManager.selectedIndex = 0;

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

uint8_t wEui_list_getSelectedIndex(void) {
    uint8_t index = 0;
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }
    index = g_listManager.selectedIndex;
    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }
    return index;
}

void wEui_list_setSelectedIndex(uint8_t index) {
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (index < g_listManager.itemCount) {
        g_listManager.selectedIndex = index;
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_moveUp(void) {
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (g_listManager.itemCount > 0) {
        if (g_listManager.selectedIndex > 0) {
            g_listManager.selectedIndex--;
        } else {
            // Wrap to bottom
            g_listManager.selectedIndex = g_listManager.itemCount - 1;
        }
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_moveDown(void) {
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (g_listManager.itemCount > 0) {
        if (g_listManager.selectedIndex < g_listManager.itemCount - 1) {
            g_listManager.selectedIndex++;
        } else {
            // Wrap to top
            g_listManager.selectedIndex = 0;
        }
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_moveToFirst(void) {
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (g_listManager.itemCount > 0) {
        g_listManager.selectedIndex = 0;
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_moveToLast(void) {
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (g_listManager.itemCount > 0) {
        g_listManager.selectedIndex = g_listManager.itemCount - 1;
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_pageUp(void) {
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (g_listManager.itemCount > 0) {
        uint8_t actualVisibleLines = wEui_list_getActualVisibleLines();
        int16_t newIndex = (int16_t)g_listManager.selectedIndex - actualVisibleLines;
        if (newIndex < 0) {
            newIndex = 0;
        }
        g_listManager.selectedIndex = newIndex;
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_pageDown(void) {
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (g_listManager.itemCount > 0) {
        uint8_t actualVisibleLines = wEui_list_getActualVisibleLines();
        uint8_t newIndex = g_listManager.selectedIndex + actualVisibleLines;
        if (newIndex >= g_listManager.itemCount) {
            newIndex = g_listManager.itemCount - 1;
        }
        g_listManager.selectedIndex = newIndex;
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_executeSelected(void) {
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }

    uint8_t selectedIdx = g_listManager.selectedIndex;
    wEui_ItemCallback_t callback = NULL;

    if (selectedIdx < g_listManager.itemCount) {
        callback = g_listManager.items[selectedIdx].onClicked;
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }

    // Execute callback outside of mutex to avoid deadlock
    if (callback != NULL) {
        callback(selectedIdx);
    }
}

const char* wEui_list_getSelectedItemName(void) {
    static char tempName[WEUI_ITEM_NAME_LENGTH];

    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }

    if (g_listManager.selectedIndex < g_listManager.itemCount) {
        strncpy(tempName, g_listManager.items[g_listManager.selectedIndex].name,
                WEUI_ITEM_NAME_LENGTH - 1);
        tempName[WEUI_ITEM_NAME_LENGTH - 1] = '\0';
    } else {
        tempName[0] = '\0';
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }

    return tempName;
}

const char* wEui_list_getItemName(uint8_t index) {
    static char tempName[WEUI_ITEM_NAME_LENGTH];

    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }

    if (index < g_listManager.itemCount) {
        strncpy(tempName, g_listManager.items[index].name,
                WEUI_ITEM_NAME_LENGTH - 1);
        tempName[WEUI_ITEM_NAME_LENGTH - 1] = '\0';
    } else {
        tempName[0] = '\0';
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }

    return tempName;
}

uint8_t wEui_list_getItemCount(void) {
    uint8_t count = 0;
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }
    count = g_listManager.itemCount;
    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }
    return count;
}

uint8_t wEui_list_getTopIndex(void) {
    uint8_t topIdx = 0;
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }
    topIdx = g_listManager.topIndex;
    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }
    return topIdx;
}

uint8_t wEui_list_getCursorPosition(void) {
    uint8_t cursorPos = 0;
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }

    if (g_listManager.itemCount == 0) {
        cursorPos = 0;
    } else {
        cursorPos = g_listManager.selectedIndex - g_listManager.topIndex;
    }

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }
    return cursorPos;
}

void wEui_list_renderScrollbar(U8G2 *display, uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    if (display == NULL || height == 0) {
        return;
    }

    // 只有行数超过可见才绘制滑块
    uint8_t actualVisibleLines = wEui_list_getActualVisibleLines();

    // 获取列表状态
    if (g_listManager.mutex != NULL) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }

    uint8_t itemCount = g_listManager.itemCount;
    uint8_t topIndex = g_listManager.topIndex;

    if (g_listManager.mutex != NULL) {
        xSemaphoreGive(g_listManager.mutex);
    }

    // 只有在条目数超过可见行数时才显示滚动条
    if (itemCount <= actualVisibleLines) {
        // 绘制空白轨道作为视觉提示
        display->setDrawColor(1);
        display->drawFrame(x, y, width, height);
        return;
    }

    // 绘制轨道与滑块
    display->setDrawColor(1);
    display->drawFrame(x, y, width, height);

    // 计算滑块的大小和位置
    // 滑块高度与可见条目数/总条目数成比例
    uint8_t trackHeight = height - 2;  // Account for frame
    uint8_t thumbHeight = (trackHeight * actualVisibleLines) / itemCount;

    // 确保滑块有最小高度
    if (thumbHeight < WEUI_SCROLLBAR_MIN_HEIGHT) {
        thumbHeight = WEUI_SCROLLBAR_MIN_HEIGHT;
    }

    // 根据顶部索引计算滑块位置
    // 最大滚动范围：itemCount - actualVisibleLines
    uint8_t maxTopIndex = itemCount - actualVisibleLines;
    uint8_t availableTrack = trackHeight - thumbHeight;
    uint8_t thumbY = y + 1;  // Start after frame

    if (maxTopIndex > 0) {
        thumbY += (availableTrack * topIndex) / maxTopIndex;
    }

    // 绘制滑块（填充的矩形）
    display->setDrawColor(1);
    display->drawBox(x + 1, thumbY, width - 2, thumbHeight);
}

