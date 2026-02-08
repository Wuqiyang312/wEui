#include "../include/wEui.h"
#include <string.h>

/**
 * @file wEui_page.cpp
 * @brief wEui Page Management Implementation
 */

// ============================================================================
// Internal Variables
// ============================================================================

// 页面管理器结构
typedef struct {
    wEui_Page_t pages[WEUI_MAX_PAGES];     // 页面数组
    int pageStack[WEUI_MAX_PAGES];         // 页面堆栈（存储页面ID）
    uint8_t pageCount;                     // 当前页面数量
    uint8_t stackDepth;                    // 当前堆栈深度
    int currentPageId;                     // 当前活动页面ID
    SemaphoreHandle_t mutex;               // 保护页面数据的互斥量
} wEui_PageManager_t;

static wEui_PageManager_t g_pageManager = {0};
static bool g_pageInitialized = false;

// ============================================================================
// Internal Functions
// ============================================================================

static bool wEui_page_isValidId(int pageId) {
    return (pageId >= 0 && pageId < g_pageManager.pageCount);
}

// ============================================================================
// Page Management Functions Implementation
// ============================================================================

int wEui_page_init(void) {
    if (g_pageInitialized) {
        return 0;
    }

    // 创建互斥量保护页面数据
    g_pageManager.mutex = xSemaphoreCreateMutex();
    if (g_pageManager.mutex == nullptr) {
        return -1;
    }

    // 初始化页面管理器
    memset(g_pageManager.pages, 0, sizeof(g_pageManager.pages));
    memset(g_pageManager.pageStack, -1, sizeof(g_pageManager.pageStack));
    g_pageManager.pageCount = 0;
    g_pageManager.stackDepth = 0;
    g_pageManager.currentPageId = -1;

    g_pageInitialized = true;
    return 0;
}

int wEui_page_createList(const char *pageName) {
    if (!g_pageInitialized || pageName == nullptr || g_pageManager.pageCount >= WEUI_MAX_PAGES) {
        return -1;
    }

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreTake(g_pageManager.mutex, portMAX_DELAY);
    }

    // 创建新的列表页面
    int pageId = g_pageManager.pageCount;
    wEui_Page_t *page = &g_pageManager.pages[pageId];

    strncpy(page->name, pageName, WEUI_PAGE_NAME_LENGTH - 1);
    page->name[WEUI_PAGE_NAME_LENGTH - 1] = '\0';
    page->type = WEUI_PAGE_TYPE_LIST;
    page->visible = false;

    // 创建关联的列表数据
    int8_t listIndex = wEui_list_createListData();
    page->data.listPage.listDataIndex = listIndex;

    g_pageManager.pageCount++;

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreGive(g_pageManager.mutex);
    }

    return pageId;
}

int wEui_page_createCustom(const char *pageName, wEui_CustomPageRender_t renderCallback) {
    if (!g_pageInitialized || pageName == nullptr || renderCallback == nullptr ||
        g_pageManager.pageCount >= WEUI_MAX_PAGES) {
        return -1;
    }

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreTake(g_pageManager.mutex, portMAX_DELAY);
    }

    // 创建新的自定义页面
    int pageId = g_pageManager.pageCount;
    wEui_Page_t *page = &g_pageManager.pages[pageId];

    strncpy(page->name, pageName, WEUI_PAGE_NAME_LENGTH - 1);
    page->name[WEUI_PAGE_NAME_LENGTH - 1] = '\0';
    page->type = WEUI_PAGE_TYPE_CUSTOM;
    page->data.customPage.renderCallback = renderCallback;
    page->visible = false;

    g_pageManager.pageCount++;

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreGive(g_pageManager.mutex);
    }

    return pageId;
}

int wEui_page_push(int pageId) {
    if (!g_pageInitialized || !wEui_page_isValidId(pageId) ||
        g_pageManager.stackDepth >= WEUI_MAX_PAGES) {
        return -1;
    }

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreTake(g_pageManager.mutex, portMAX_DELAY);
    }

    // 隐藏当前页面
    if (g_pageManager.currentPageId >= 0) {
        g_pageManager.pages[g_pageManager.currentPageId].visible = false;
    }

    // 将页面推入堆栈
    g_pageManager.pageStack[g_pageManager.stackDepth] = pageId;
    g_pageManager.stackDepth++;
    g_pageManager.currentPageId = pageId;
    g_pageManager.pages[pageId].visible = true;

    // 如果是列表页面，自动切换列表上下文
    wEui_PageType_t pageType = g_pageManager.pages[pageId].type;
    int8_t listDataIndex = g_pageManager.pages[pageId].data.listPage.listDataIndex;

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreGive(g_pageManager.mutex);
    }

    // 切换列表上下文（在释放互斥量后执行，避免死锁）
    if (pageType == WEUI_PAGE_TYPE_LIST && listDataIndex >= 0) {
        wEui_list_switchContext(listDataIndex);
    }

    return 0;
}

int wEui_page_pop(void) {
    if (!g_pageInitialized || g_pageManager.stackDepth == 0) {
        return -1;
    }

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreTake(g_pageManager.mutex, portMAX_DELAY);
    }

    // 隐藏当前页面
    if (g_pageManager.currentPageId >= 0) {
        g_pageManager.pages[g_pageManager.currentPageId].visible = false;
    }

    // 从堆栈弹出页面
    g_pageManager.stackDepth--;
    g_pageManager.pageStack[g_pageManager.stackDepth] = -1;

    // 设置新的当前页面
    wEui_PageType_t pageType = WEUI_PAGE_TYPE_LIST;
    int8_t listDataIndex = -1;

    if (g_pageManager.stackDepth > 0) {
        g_pageManager.currentPageId = g_pageManager.pageStack[g_pageManager.stackDepth - 1];
        g_pageManager.pages[g_pageManager.currentPageId].visible = true;

        // 获取新当前页面的列表上下文
        pageType = g_pageManager.pages[g_pageManager.currentPageId].type;
        if (pageType == WEUI_PAGE_TYPE_LIST) {
            listDataIndex = g_pageManager.pages[g_pageManager.currentPageId].data.listPage.listDataIndex;
        }
    } else {
        g_pageManager.currentPageId = -1;
    }

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreGive(g_pageManager.mutex);
    }

    // 切换列表上下文（在释放互斥量后执行，避免死锁）
    if (pageType == WEUI_PAGE_TYPE_LIST && listDataIndex >= 0) {
        wEui_list_switchContext(listDataIndex);
    }

    return 0;
}

int wEui_page_getCurrentId(void) {
    if (!g_pageInitialized) {
        return -1;
    }

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreTake(g_pageManager.mutex, pdMS_TO_TICKS(10));
    }
    int currentId = g_pageManager.currentPageId;
    if (g_pageManager.mutex != nullptr) {
        xSemaphoreGive(g_pageManager.mutex);
    }
    return currentId;
}

wEui_PageType_t wEui_page_getType(int pageId) {
    if (!g_pageInitialized || !wEui_page_isValidId(pageId)) {
        return WEUI_PAGE_TYPE_LIST; // Default fallback
    }

    wEui_PageType_t type;
    if (g_pageManager.mutex != nullptr) {
        xSemaphoreTake(g_pageManager.mutex, pdMS_TO_TICKS(10));
    }
    type = g_pageManager.pages[pageId].type;
    if (g_pageManager.mutex != nullptr) {
        xSemaphoreGive(g_pageManager.mutex);
    }
    return type;
}

const char* wEui_page_getName(int pageId) {
    static char tempName[WEUI_PAGE_NAME_LENGTH];

    if (!g_pageInitialized || !wEui_page_isValidId(pageId)) {
        return "";
    }

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreTake(g_pageManager.mutex, pdMS_TO_TICKS(10));
    }

    strncpy(tempName, g_pageManager.pages[pageId].name, WEUI_PAGE_NAME_LENGTH - 1);
    tempName[WEUI_PAGE_NAME_LENGTH - 1] = '\0';

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreGive(g_pageManager.mutex);
    }

    return tempName;
}

uint8_t wEui_page_getStackDepth(void) {
    if (!g_pageInitialized) {
        return 0;
    }

    uint8_t depth = 0;
    if (g_pageManager.mutex != nullptr) {
        xSemaphoreTake(g_pageManager.mutex, pdMS_TO_TICKS(10));
    }
    depth = g_pageManager.stackDepth;
    if (g_pageManager.mutex != nullptr) {
        xSemaphoreGive(g_pageManager.mutex);
    }
    return depth;
}

int wEui_page_switchListContext(int pageId) {
    if (!g_pageInitialized || !wEui_page_isValidId(pageId)) {
        return -1;
    }

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreTake(g_pageManager.mutex, pdMS_TO_TICKS(10));
    }

    wEui_PageType_t pageType = g_pageManager.pages[pageId].type;
    int8_t listDataIndex = g_pageManager.pages[pageId].data.listPage.listDataIndex;

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreGive(g_pageManager.mutex);
    }

    // 只有列表页面才能切换列表上下文
    if (pageType != WEUI_PAGE_TYPE_LIST) {
        return -2;
    }

    // 切换到该页面关联的列表数据
    wEui_list_switchContext(listDataIndex);
    return 0;
}

// ============================================================================
// Internal Page Rendering Functions
// ============================================================================

/**
 * @brief 渲染自定义页面
 * @param pageId 页面ID
 * @param display U8G2显示对象
 * @param displayConfig 显示配置
 * @param contentHeight 内容区域高度
 * @return 0成功，负数失败
 */
int wEui_page_renderCustom(int pageId, U8G2 *display, const wEui_DisplayConfig_t *displayConfig, uint8_t contentHeight) {
    if (!g_pageInitialized || !wEui_page_isValidId(pageId) || display == nullptr || displayConfig == nullptr) {
        return -1;
    }

    wEui_CustomPageRender_t renderCallback = nullptr;

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreTake(g_pageManager.mutex, pdMS_TO_TICKS(10));
    }

    if (g_pageManager.pages[pageId].type == WEUI_PAGE_TYPE_CUSTOM) {
        renderCallback = g_pageManager.pages[pageId].data.customPage.renderCallback;
    }

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreGive(g_pageManager.mutex);
    }

    // 执行自定义渲染回调
    if (renderCallback != nullptr) {
        renderCallback(display, displayConfig, contentHeight);
        return 0;
    }

    return -2;
}

/**
 * @brief 获取当前页面的渲染信息
 * @param currentPageId 输出当前页面ID
 * @param pageType 输出页面类型
 * @return 0成功，负数失败
 */
int wEui_page_getCurrentRenderInfo(int *currentPageId, wEui_PageType_t *pageType) {
    if (!g_pageInitialized || currentPageId == nullptr || pageType == nullptr) {
        return -1;
    }

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreTake(g_pageManager.mutex, pdMS_TO_TICKS(10));
    }

    *currentPageId = g_pageManager.currentPageId;

    if (g_pageManager.currentPageId >= 0 && wEui_page_isValidId(g_pageManager.currentPageId)) {
        *pageType = g_pageManager.pages[g_pageManager.currentPageId].type;
    } else {
        *pageType = WEUI_PAGE_TYPE_LIST; // Default fallback
    }

    if (g_pageManager.mutex != nullptr) {
        xSemaphoreGive(g_pageManager.mutex);
    }

    return 0;
}
