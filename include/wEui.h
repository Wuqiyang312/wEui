#ifndef WEUI_H
#define WEUI_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

/**
 * @file wEui.h
 * @brief wEui - Lightweight UI Library for Embedded Systems
 *
 * A simple and efficient UI library designed for embedded systems,
 * featuring list management, button handling, and display rendering.
 *
 * @author Wuqiyang312
 * @version 1.1.0
 */

// ============================================================================
// Configuration Constants
// ============================================================================

#define WEUI_MAX_ITEMS 20 // 列表支持的最大条目数
#define WEUI_VISIBLE_LINES 5 // 屏幕上可见的行数（近似）
#define WEUI_ITEM_NAME_LENGTH 32 // 条目名称的最大长度
#define WEUI_LINE_HEIGHT 12 // 每行的像素高度
#define WEUI_DEFAULT_FONT u8g2_font_6x10_tf // 默认使用的字体
#define WEUI_FIXED_CURSOR_POS 2  // 固定光标位置（0 基，位于可见行中间）
#define WEUI_SCROLL_MARGIN 1     // 上下保留的最小行距，防止光标贴边

// Scrollbar configuration
#define WEUI_SCROLLBAR_WIDTH 6   // 滚动条的像素宽度
#define WEUI_SCROLLBAR_MIN_HEIGHT 4  // 滚动条滑块高度的最小值

// Page management configuration
#define WEUI_MAX_PAGES 10        // 支持的最大页面堆栈深度
#define WEUI_PAGE_NAME_LENGTH 32 // 页面名称的最大长度

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief Page types supported by wEui
 */
typedef enum {
    WEUI_PAGE_TYPE_LIST = 0,     // 列表页面类型
    WEUI_PAGE_TYPE_CUSTOM        // 自定义页面类型
} wEui_PageType_t;

/**
 * @brief Display configuration structure
 */
typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t lineHeight;
    const uint8_t *font;
} wEui_DisplayConfig_t;

/**
 * @brief Custom page render callback function type
 * @param display U8G2 display pointer
 * @param displayConfig Display configuration
 * @param contentHeight Available content height (excluding status bar)
 */
typedef void (*wEui_CustomPageRender_t)(U8G2 *display, const wEui_DisplayConfig_t *displayConfig, uint8_t contentHeight);

/**
 * @brief Page structure
 */
typedef struct {
    char name[WEUI_PAGE_NAME_LENGTH];
    wEui_PageType_t type;
    union {
        struct {
            // 列表页面数据索引，用于关联独立的列表数据
            int8_t listDataIndex;
        } listPage;
        struct {
            wEui_CustomPageRender_t renderCallback;
        } customPage;
    } data;
    bool visible;  // 页面是否可见
} wEui_Page_t;

// Include statusbar header after DisplayConfig is defined
#include "wEui_statusbar.h"
#include "wEui_toast.h"

/**
 * @brief Button types supported by wEui
 */
typedef enum {
    WEUI_BUTTON_UP = 0,
    WEUI_BUTTON_DOWN,
    WEUI_BUTTON_OK,
    WEUI_BUTTON_BACK,
    WEUI_BUTTON_COUNT
} wEui_ButtonType_t;

/**
 * @brief List item callback function type
 * @param itemIndex Index of the selected item
 */
typedef void (*wEui_ItemCallback_t)(uint8_t itemIndex);

/**
 * @brief Button event callback function type
 * @param button The button that was pressed
 */
typedef void (*wEui_ButtonCallback_t)(wEui_ButtonType_t button);

/**
 * @brief List item structure
 */
typedef struct {
    char name[WEUI_ITEM_NAME_LENGTH];
    wEui_ItemCallback_t onClicked;
} wEui_ListItem_t;


/**
 * @brief Button pull mode configuration
 */
typedef enum {
    WEUI_BUTTON_PULL_DOWN = 0,      // 默认下拉检测 (按下时为HIGH)
    WEUI_BUTTON_PULL_UP = 1,        // 上拉检测 (按下时为LOW)
    WEUI_BUTTON_FLOATING_RISE = 2,  // 实验性：悬空输入检测电压上升 (LOW到HIGH)
    WEUI_BUTTON_FLOATING_FALL = 3,  // 实验性：悬空输入检测电压下降 (HIGH到LOW)
    WEUI_BUTTON_ADC_SHARED = 4      // ADC模式：一个GPIO通过不同电压范围检测多个按键
} wEui_ButtonPullMode_t;

/**
 * @brief ADC button range configuration for shared GPIO
 */
typedef struct {
    uint16_t minValue;    // 最小ADC值
    uint16_t maxValue;    // 最大ADC值
} wEui_AdcButtonRange_t;

/**
 * @brief Invalid/unconfigured pin marker
 * 未配置引脚标记 - 使用0xFF而不是0，因为GPIO0是有效的引脚
 */
#define WEUI_PIN_INVALID 0xFF

/**
 * @brief Single button configuration (supports both GPIO and ADC mode)
 * 单个按钮配置，支持GPIO模式和ADC模式
 */
typedef struct {
    uint8_t pin;                      // GPIO引脚号 (WEUI_PIN_INVALID表示未配置)
    wEui_ButtonPullMode_t pullMode;   // 拉模式/ADC模式
    wEui_AdcButtonRange_t adcRange;   // ADC模式时的电压范围
} wEui_SingleButtonConfig_t;

/**
 * @brief ADC group configuration for shared pins
 * ADC组配置（共享同一引脚的按钮组）
 */
typedef struct {
    uint8_t adcPin;           // ADC共享引脚
    uint16_t debounceMs;      // 防抖动延时（毫秒）
    uint16_t adcResolution;   // ADC分辨率 (例如: 4095 for 12-bit)
    uint8_t buttonMask;       // 哪些按钮使用此ADC引脚 (位掩码)
} wEui_AdcGroupConfig_t;

// 最大支持的ADC组数量（不同的ADC引脚）
#define WEUI_MAX_ADC_GROUPS 4

/**
 * @brief Button pin configuration structure
 * 按钮配置结构体 - 支持混合模式
 *
 * 使用示例:
 * 1. 所有按钮独立GPIO:
 *    buttons[WEUI_BUTTON_UP] = {.pin = 5, .pullMode = WEUI_BUTTON_PULL_UP};
 *    buttons[WEUI_BUTTON_DOWN] = {.pin = 6, .pullMode = WEUI_BUTTON_PULL_UP};
 *
 * 2. 部分按钮共享ADC:
 *    buttons[WEUI_BUTTON_UP] = {.pin = 34, .pullMode = WEUI_BUTTON_ADC_SHARED, .adcRange = {100, 500}};
 *    buttons[WEUI_BUTTON_DOWN] = {.pin = 34, .pullMode = WEUI_BUTTON_ADC_SHARED, .adcRange = {600, 1000}};
 *    buttons[WEUI_BUTTON_OK] = {.pin = 7, .pullMode = WEUI_BUTTON_PULL_UP};  // 独立GPIO
 *
 * 3. 多个ADC组:
 *    buttons[WEUI_BUTTON_UP] = {.pin = 34, .pullMode = WEUI_BUTTON_ADC_SHARED, .adcRange = {100, 500}};
 *    buttons[WEUI_BUTTON_DOWN] = {.pin = 34, .pullMode = WEUI_BUTTON_ADC_SHARED, .adcRange = {600, 1000}};
 *    buttons[WEUI_BUTTON_OK] = {.pin = 35, .pullMode = WEUI_BUTTON_ADC_SHARED, .adcRange = {200, 600}};
 *    buttons[WEUI_BUTTON_BACK] = {.pin = 35, .pullMode = WEUI_BUTTON_ADC_SHARED, .adcRange = {700, 1100}};
 */
typedef struct {
    wEui_SingleButtonConfig_t buttons[WEUI_BUTTON_COUNT];  // 每个按钮的独立配置
    uint16_t debounceMs;      // ADC防抖动延时（毫秒），默认50ms
    uint16_t adcResolution;   // ADC分辨率 (例如: 4095 for 12-bit)
} wEui_ButtonConfig_t;

/**
 * @brief wEui initialization configuration
 */
typedef struct {
    U8G2 *display;
    wEui_DisplayConfig_t displayConfig;
    wEui_ButtonConfig_t buttonConfig;
    QueueHandle_t buttonQueue;
    SemaphoreHandle_t listMutex;
} wEui_Config_t;

// ============================================================================
// Core Functions
// ============================================================================

/**
 * @brief Initialize the wEui library
 * @param config Configuration structure
 * @return 0 on success, negative on error
 */
int wEui_init(const wEui_Config_t *config);

/**
 * @brief Deinitialize the wEui library
 * @return 0 on success, negative on error
 */
int wEui_deinit(void);

/**
 * @brief Begin UI rendering (call once at startup)
 * @return 0 on success, negative on error
 */
int wEui_begin(void);

/**
 * @brief Render the current UI state to display buffer
 * @return 0 on success, negative on error
 */
int wEui_render(void);

/**
 * @brief Update the display buffer to screen
 * @return 0 on success, negative on error
 */
int wEui_update(void);

// ============================================================================
// Page Management Functions
// ============================================================================

/**
 * @brief Initialize page management system
 * @return 0 on success, negative on error
 */
int wEui_page_init(void);

/**
 * @brief Create a new list page
 * @param pageName Name of the page
 * @return Page ID on success, negative on error
 */
int wEui_page_createList(const char *pageName);

/**
 * @brief Create a new custom page with render callback
 * @param pageName Name of the page
 * @param renderCallback Custom render function
 * @return Page ID on success, negative on error
 */
int wEui_page_createCustom(const char *pageName, wEui_CustomPageRender_t renderCallback);

/**
 * @brief Push a page onto the stack (makes it visible)
 * @param pageId Page ID to push
 * @return 0 on success, negative on error
 */
int wEui_page_push(int pageId);

/**
 * @brief Pop the current page from stack
 * @return 0 on success, negative on error
 */
int wEui_page_pop(void);

/**
 * @brief Get current active page ID
 * @return Current page ID, negative if no page active
 */
int wEui_page_getCurrentId(void);

/**
 * @brief Get page type by page ID
 * @param pageId Page ID
 * @return Page type
 */
wEui_PageType_t wEui_page_getType(int pageId);

/**
 * @brief Get page name by page ID
 * @param pageId Page ID
 * @return Pointer to page name, NULL on error
 */
const char* wEui_page_getName(int pageId);

/**
 * @brief Get stack depth (number of pages in stack)
 * @return Stack depth
 */
uint8_t wEui_page_getStackDepth(void);

/**
 * @brief Switch current list context to specific page (for list pages only)
 * @param pageId Page ID
 * @return 0 on success, negative on error
 */
int wEui_page_switchListContext(int pageId);

// ============================================================================
// List Management Functions
// ============================================================================

/**
 * @brief Create a new list data storage
 * @return List data index on success, negative on failure
 */
int8_t wEui_list_createListData(void);

/**
 * @brief Switch to a specific list context
 * @param listIndex List data index to switch to
 */
void wEui_list_switchContext(int8_t listIndex);

/**
 * @brief Get current active list index
 * @return Current list index
 */
int8_t wEui_list_getCurrentListIndex(void);

/**
 * @brief Add an item to the list
 * @param itemName Name of the item
 * @param callback Callback function when item is selected
 * @return true on success, false on failure
 */
bool wEui_list_addItem(const char *itemName, wEui_ItemCallback_t callback);

/**
 * @brief Remove the last item from the list
 * @return true on success, false on failure
 */
bool wEui_list_removeLast(void);

/**
 * @brief Clear all items from the list
 */
void wEui_list_clear(void);

/**
 * @brief Get the currently selected item index
 * @return Selected item index
 */
uint8_t wEui_list_getSelectedIndex(void);

/**
 * @brief Set the selected item index
 * @param index Index to select
 */
void wEui_list_setSelectedIndex(uint8_t index);

/**
 * @brief Move selection up
 */
void wEui_list_moveUp(void);

/**
 * @brief Move selection down
 */
void wEui_list_moveDown(void);

/**
 * @brief Jump to first item
 */
void wEui_list_moveToFirst(void);

/**
 * @brief Jump to last item
 */
void wEui_list_moveToLast(void);

/**
 * @brief Move up by one page
 */
void wEui_list_pageUp(void);

/**
 * @brief Move down by one page
 */
void wEui_list_pageDown(void);

/**
 * @brief Execute the callback of the currently selected item
 */
void wEui_list_executeSelected(void);

/**
 * @brief Get the name of the currently selected item
 * @return Pointer to item name string
 */
const char* wEui_list_getSelectedItemName(void);

/**
 * @brief Get the total number of items in the list
 * @return Number of items
 */
uint8_t wEui_list_getItemCount(void);

/**
 * @brief Get the top visible item index
 * @return Top index
 */
uint8_t wEui_list_getTopIndex(void);

/**
 * @brief Get the current cursor position on screen (0-based)
 * @return Cursor position relative to top of screen
 */
uint8_t wEui_list_getCursorPosition(void);

/**
 * @brief Get actual visible lines count based on available display height
 * @return Number of lines that can actually be displayed
 */
uint8_t wEui_list_getActualVisibleLines(void);

/**
 * @brief Render the scrollbar for the list
 * @param display U8G2 display pointer
 * @param x X position of scrollbar
 * @param y Y position of scrollbar area
 * @param width Width of scrollbar
 * @param height Height of scrollbar area
 */
void wEui_list_renderScrollbar(U8G2 *display, uint8_t x, uint8_t y, uint8_t width, uint8_t height);

// ============================================================================
// Button Management Functions
// ============================================================================

/**
 * @brief Initialize button handling
 * @return 0 on success, negative on error
 */
int wEui_button_init(void);

/**
 * @brief Deinitialize button handling
 * @return 0 on success, negative on error
 */
int wEui_button_deinit(void);

/**
 * @brief Set button event callback
 * @param button Button type
 * @param callback Callback function
 * @return 0 on success, negative on error
 */
int wEui_button_setCallback(wEui_ButtonType_t button, wEui_ButtonCallback_t callback);

/**
 * @brief Scan button states (call periodically)
 * @return 0 on success, negative on error
 */
int wEui_button_scan(void);

/**
 * @brief Read current button state
 * @param button Button type
 * @return 1 if not pressed, 0 if pressed
 */
uint8_t wEui_button_read(wEui_ButtonType_t button);

/**
 * @brief Set button pin configuration
 * @param config Button pin configuration
 * @return 0 on success, negative on error
 */
int wEui_button_setPinConfig(const wEui_ButtonConfig_t *config);

/**
 * @brief Read ADC value and detect button press (scans all ADC groups)
 * @return Detected button type, WEUI_BUTTON_COUNT if no button detected
 */
wEui_ButtonType_t wEui_button_scanAdc(void);

/**
 * @brief Get current ADC reading for a specific pin (for debugging/calibration)
 * @param pin GPIO pin to read ADC value from
 * @return Current ADC value
 */
uint16_t wEui_button_readAdcValue(uint8_t pin);

/**
 * @brief Check if any button uses ADC mode
 * @return true if any ADC mode is enabled
 */
bool wEui_button_isAdcModeEnabled(void);

/**
 * @brief Get ADC group count
 * @return Number of unique ADC pins configured
 */
uint8_t wEui_button_getAdcGroupCount(void);

/**
 * @brief Initialize list management with mutex
 * @param mutex Semaphore handle for thread safety
 */
void wEui_list_init(SemaphoreHandle_t mutex);

/**
 * @brief Get item name by index
 * @param index Item index
 * @return Pointer to item name string
 */
const char* wEui_list_getItemName(uint8_t index);

/**
 * @brief Set button queue handle
 * @param queue Queue handle for button events
 */
void wEui_button_setButtonQueue(QueueHandle_t queue);

/**
 * @brief Check if button system is initialized
 * @return true if initialized, false otherwise
 */
bool wEui_button_isInitialized(void);

/**
 * @brief Get button name string
 * @param button Button type
 * @return Button name string
 */
const char* wEui_button_getName(wEui_ButtonType_t button);

// ============================================================================
// Event Processing Functions
// ============================================================================

/**
 * @brief Process button events from queue
 * @param timeout Timeout in ticks
 * @return 0 on success, negative on error or timeout
 */
int wEui_processButtonEvents(uint32_t timeout);

/**
 * @brief Set default button event handlers for list navigation
 */
void wEui_setDefaultButtonHandlers(void);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Get library version string
 * @return Version string
 */
const char* wEui_getVersion(void);

/**
 * @brief Get current display configuration
 * @return Pointer to display configuration
 */
const wEui_DisplayConfig_t* wEui_getDisplayConfig(void);

/**
 * @brief Set custom font for rendering
 * @param font Font to use
 */
void wEui_setFont(const uint8_t *font);

// ============================================================================
// Internal Page Rendering Functions (for library internal use)
// ============================================================================

/**
 * @brief Get current page render information
 * @param currentPageId Output current page ID
 * @param pageType Output page type
 * @return 0 on success, negative on error
 */
int wEui_page_getCurrentRenderInfo(int *currentPageId, wEui_PageType_t *pageType);

/**
 * @brief Render custom page
 * @param pageId Page ID
 * @param display U8G2 display pointer
 * @param displayConfig Display configuration
 * @param contentHeight Available content height
 * @return 0 on success, negative on error
 */
int wEui_page_renderCustom(int pageId, U8G2 *display, const wEui_DisplayConfig_t *displayConfig, uint8_t contentHeight);

// ============================================================================
// Status Bar Functions
// ============================================================================

/**
 * @brief Set status bar text
 * @param text Text to display in status bar
 */
void wEui_statusBar_setText(const char *text);

/**
 * @brief Get current status bar text
 * @return Pointer to status bar text
 */
const char* wEui_statusBar_getText(void);

/**
 * @brief Set status bar border visibility
 * @param showBorder Show border (true) or not (false)
 */
void wEui_statusBar_setShowBorder(bool showBorder);

/**
 * @brief Get status bar border visibility
 * @return true if border is shown, false otherwise
 */
bool wEui_statusBar_getShowBorder(void);

#endif // WEUI_H
