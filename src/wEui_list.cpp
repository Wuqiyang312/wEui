#include "../include/wEui.h"
#include <string.h>

/**
 * @file wEui_list.cpp
 * @brief wEui List Management Implementation
 */

// ============================================================================
// Internal Variables
// ============================================================================

// 单个列表的数据结构
typedef struct {
    wEui_ListItem_t items[WEUI_MAX_ITEMS];
    uint8_t itemCount;
    uint8_t topIndex;
    uint8_t selectedIndex;
} wEui_ListData_t;

// 列表管理器结构（支持多个列表）
typedef struct {
    wEui_ListData_t lists[WEUI_MAX_PAGES];  // 每个页面一个列表数据
    int8_t currentListIndex;                 // 当前活动列表索引
    uint8_t listCount;                       // 已创建的列表数量
    SemaphoreHandle_t mutex;
} wEui_ListManager_t;

static wEui_ListManager_t g_listManager = {0};
static bool g_listInitialized = false;

// 获取当前活动列表数据的辅助宏
#define CURRENT_LIST() (&g_listManager.lists[g_listManager.currentListIndex >= 0 ? g_listManager.currentListIndex : 0])

// ============================================================================
// Internal Functions
// ============================================================================

static void wEui_list_adjustTopIndex(void) {
    wEui_ListData_t *list = CURRENT_LIST();

    // 获取当前可见行
    uint8_t actualVisibleLines = wEui_list_getActualVisibleLines();

    // 固定光标位置，尽量保持选中项在屏幕中间
    uint8_t targetCursorPos = WEUI_FIXED_CURSOR_POS;

    // 如果实际可见行数不足，就在中间显示选中项
    if (targetCursorPos >= actualVisibleLines) {
        targetCursorPos = actualVisibleLines / 2;  // Use middle position
    }

    // 如果条目数少于可见行数，从顶部开始
    if (list->itemCount <= actualVisibleLines) {
        list->topIndex = 0;
        return;
    }

    // 计算理想的顶部索引以保持光标在固定位置
    int16_t idealTopIndex = list->selectedIndex - targetCursorPos;

    // 限制在有效范围内
    if (idealTopIndex < 0) {
        list->topIndex = 0;
    } else if (idealTopIndex > (int16_t)(list->itemCount - actualVisibleLines)) {
        list->topIndex = list->itemCount - actualVisibleLines;
    } else {
        list->topIndex = idealTopIndex;
    }
}

// ============================================================================
// List Management Functions Implementation
// ============================================================================

void wEui_list_init(SemaphoreHandle_t mutex) {
    if (g_listInitialized) {
        return;
    }

    // 重置列表管理器并存储互斥量
    memset(&g_listManager, 0, sizeof(g_listManager));
    g_listManager.mutex = mutex;
    g_listManager.currentListIndex = 0;  // 默认使用第一个列表
    g_listManager.listCount = 0;
    g_listInitialized = true;
}

// 创建新的列表数据，返回列表索引
int8_t wEui_list_createListData(void) {
    if (!g_listInitialized || g_listManager.listCount >= WEUI_MAX_PAGES) {
        return -1;
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    int8_t listIndex = g_listManager.listCount;
    memset(&g_listManager.lists[listIndex], 0, sizeof(wEui_ListData_t));
    g_listManager.listCount++;

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }

    return listIndex;
}

// 切换当前活动列表
void wEui_list_switchContext(int8_t listIndex) {
    if (!g_listInitialized || listIndex < 0 || listIndex >= g_listManager.listCount) {
        return;
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    g_listManager.currentListIndex = listIndex;

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

// 获取当前活动列表索引
int8_t wEui_list_getCurrentListIndex(void) {
    return g_listManager.currentListIndex;
}

bool wEui_list_addItem(const char *itemName, wEui_ItemCallback_t callback) {
    wEui_ListData_t *list = CURRENT_LIST();

    if (!itemName || list->itemCount >= WEUI_MAX_ITEMS) {
        return false;
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    // 写入条目信息并递增计数
    strncpy(list->items[list->itemCount].name, itemName,
            WEUI_ITEM_NAME_LENGTH - 1);
    list->items[list->itemCount].name[WEUI_ITEM_NAME_LENGTH - 1] = '\0';
    list->items[list->itemCount].onClicked = callback;
    list->itemCount++;

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }

    return true;
}

bool wEui_list_removeLast(void) {
    wEui_ListData_t *list = CURRENT_LIST();

    if (list->itemCount == 0) {
        return false;
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    // 删除最后一个条目并处理索引回退
    list->itemCount--;
    memset(&list->items[list->itemCount], 0, sizeof(wEui_ListItem_t));

    // 调整选中索引
    if (list->selectedIndex >= list->itemCount && list->itemCount > 0) {
        list->selectedIndex = list->itemCount - 1;
    }

    // 调整顶部索引
    uint8_t actualVisibleLines = wEui_list_getActualVisibleLines();
    if (list->topIndex > 0 && list->itemCount <= actualVisibleLines) {
        list->topIndex = 0;
    } else if (list->topIndex + actualVisibleLines > list->itemCount) {
        list->topIndex = (list->itemCount > actualVisibleLines) ?
                                 list->itemCount - actualVisibleLines : 0;
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }

    return true;
}

void wEui_list_clear(void) {
    wEui_ListData_t *list = CURRENT_LIST();

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    // 清理所有条目并重置游标
    memset(list->items, 0, sizeof(list->items));
    list->itemCount = 0;
    list->topIndex = 0;
    list->selectedIndex = 0;

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

uint8_t wEui_list_getSelectedIndex(void) {
    uint8_t index = 0;
    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }
    index = CURRENT_LIST()->selectedIndex;
    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
    return index;
}

void wEui_list_setSelectedIndex(uint8_t index) {
    wEui_ListData_t *list = CURRENT_LIST();

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (index < list->itemCount) {
        list->selectedIndex = index;
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_moveUp(void) {
    wEui_ListData_t *list = CURRENT_LIST();

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (list->itemCount > 0) {
        if (list->selectedIndex > 0) {
            list->selectedIndex--;
        } else {
            // Wrap to bottom
            list->selectedIndex = list->itemCount - 1;
        }
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_moveDown(void) {
    wEui_ListData_t *list = CURRENT_LIST();

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (list->itemCount > 0) {
        if (list->selectedIndex < list->itemCount - 1) {
            list->selectedIndex++;
        } else {
            // Wrap to top
            list->selectedIndex = 0;
        }
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_moveToFirst(void) {
    wEui_ListData_t *list = CURRENT_LIST();

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (list->itemCount > 0) {
        list->selectedIndex = 0;
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_moveToLast(void) {
    wEui_ListData_t *list = CURRENT_LIST();

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (list->itemCount > 0) {
        list->selectedIndex = list->itemCount - 1;
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_pageUp(void) {
    wEui_ListData_t *list = CURRENT_LIST();

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (list->itemCount > 0) {
        uint8_t actualVisibleLines = wEui_list_getActualVisibleLines();
        int16_t newIndex = (int16_t)list->selectedIndex - actualVisibleLines;
        if (newIndex < 0) {
            newIndex = 0;
        }
        list->selectedIndex = newIndex;
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_pageDown(void) {
    wEui_ListData_t *list = CURRENT_LIST();

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, portMAX_DELAY);
    }

    if (list->itemCount > 0) {
        uint8_t actualVisibleLines = wEui_list_getActualVisibleLines();
        uint8_t newIndex = list->selectedIndex + actualVisibleLines;
        if (newIndex >= list->itemCount) {
            newIndex = list->itemCount - 1;
        }
        list->selectedIndex = newIndex;
        wEui_list_adjustTopIndex();
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
}

void wEui_list_executeSelected(void) {
    wEui_ListData_t *list = CURRENT_LIST();

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }

    uint8_t selectedIdx = list->selectedIndex;
    wEui_ItemCallback_t callback = nullptr;

    if (selectedIdx < list->itemCount) {
        callback = list->items[selectedIdx].onClicked;
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }

    // Execute callback outside of mutex to avoid deadlock
    if (callback != nullptr) {
        callback(selectedIdx);
    }
}

const char* wEui_list_getSelectedItemName(void) {
    static char tempName[WEUI_ITEM_NAME_LENGTH];
    wEui_ListData_t *list = CURRENT_LIST();

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }

    if (list->selectedIndex < list->itemCount) {
        strncpy(tempName, list->items[list->selectedIndex].name,
                WEUI_ITEM_NAME_LENGTH - 1);
        tempName[WEUI_ITEM_NAME_LENGTH - 1] = '\0';
    } else {
        tempName[0] = '\0';
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }

    return tempName;
}

const char* wEui_list_getItemName(uint8_t index) {
    static char tempName[WEUI_ITEM_NAME_LENGTH];
    wEui_ListData_t *list = CURRENT_LIST();

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }

    if (index < list->itemCount) {
        strncpy(tempName, list->items[index].name,
                WEUI_ITEM_NAME_LENGTH - 1);
        tempName[WEUI_ITEM_NAME_LENGTH - 1] = '\0';
    } else {
        tempName[0] = '\0';
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }

    return tempName;
}

uint8_t wEui_list_getItemCount(void) {
    uint8_t count = 0;
    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }
    count = CURRENT_LIST()->itemCount;
    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
    return count;
}

uint8_t wEui_list_getTopIndex(void) {
    uint8_t topIdx = 0;
    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }
    topIdx = CURRENT_LIST()->topIndex;
    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
    return topIdx;
}

uint8_t wEui_list_getCursorPosition(void) {
    uint8_t cursorPos = 0;
    wEui_ListData_t *list = CURRENT_LIST();

    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }

    if (list->itemCount == 0) {
        cursorPos = 0;
    } else {
        cursorPos = list->selectedIndex - list->topIndex;
    }

    if (g_listManager.mutex != nullptr) {
        xSemaphoreGive(g_listManager.mutex);
    }
    return cursorPos;
}

void wEui_list_renderScrollbar(U8G2 *display, uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    if (display == nullptr || height == 0) {
        return;
    }

    // 只有行数超过可见才绘制滑块
    uint8_t actualVisibleLines = wEui_list_getActualVisibleLines();
    wEui_ListData_t *list = CURRENT_LIST();

    // 获取列表状态
    if (g_listManager.mutex != nullptr) {
        xSemaphoreTake(g_listManager.mutex, pdMS_TO_TICKS(10));
    }

    uint8_t itemCount = list->itemCount;
    uint8_t topIndex = list->topIndex;

    if (g_listManager.mutex != nullptr) {
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

