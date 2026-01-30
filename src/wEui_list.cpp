#include "../include/wEui.h"
#include <string.h>

/**
 * @file wEui_list.cpp
 * @brief wEui List Management Implementation
 */

// ============================================================================
// Internal Variables
// ============================================================================

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
    // Fixed cursor position scrolling - keep cursor at WEUI_FIXED_CURSOR_POS when possible
    uint8_t targetCursorPos = WEUI_FIXED_CURSOR_POS;

    // If we have fewer items than visible lines, start from top
    if (g_listManager.itemCount <= WEUI_VISIBLE_LINES) {
        g_listManager.topIndex = 0;
        return;
    }

    // Calculate ideal top index to keep cursor at fixed position
    int16_t idealTopIndex = g_listManager.selectedIndex - targetCursorPos;

    // Clamp to valid range
    if (idealTopIndex < 0) {
        g_listManager.topIndex = 0;
    } else if (idealTopIndex > (int16_t)(g_listManager.itemCount - WEUI_VISIBLE_LINES)) {
        g_listManager.topIndex = g_listManager.itemCount - WEUI_VISIBLE_LINES;
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

    g_listManager.itemCount--;
    memset(&g_listManager.items[g_listManager.itemCount], 0, sizeof(wEui_ListItem_t));

    // Adjust selected index if necessary
    if (g_listManager.selectedIndex >= g_listManager.itemCount && g_listManager.itemCount > 0) {
        g_listManager.selectedIndex = g_listManager.itemCount - 1;
    }

    // Adjust top index
    if (g_listManager.topIndex > 0 && g_listManager.itemCount <= WEUI_VISIBLE_LINES) {
        g_listManager.topIndex = 0;
    } else if (g_listManager.topIndex + WEUI_VISIBLE_LINES > g_listManager.itemCount) {
        g_listManager.topIndex = (g_listManager.itemCount > WEUI_VISIBLE_LINES) ?
                                 g_listManager.itemCount - WEUI_VISIBLE_LINES : 0;
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
        int16_t newIndex = (int16_t)g_listManager.selectedIndex - WEUI_VISIBLE_LINES;
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
        uint8_t newIndex = g_listManager.selectedIndex + WEUI_VISIBLE_LINES;
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

