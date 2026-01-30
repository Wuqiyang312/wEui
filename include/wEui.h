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
 * @version 1.0.0
 */

// ============================================================================
// Configuration Constants
// ============================================================================

#define WEUI_MAX_ITEMS 20
#define WEUI_VISIBLE_LINES 5
#define WEUI_ITEM_NAME_LENGTH 32
#define WEUI_LINE_HEIGHT 12
#define WEUI_DEFAULT_FONT u8g2_font_6x10_tf
#define WEUI_FIXED_CURSOR_POS 2  // Fixed cursor position (0-based, middle of visible lines)
#define WEUI_SCROLL_MARGIN 1     // Lines to keep above/below cursor when possible

// Scrollbar configuration
#define WEUI_SCROLLBAR_WIDTH 6   // Width of the scrollbar in pixels
#define WEUI_SCROLLBAR_MIN_HEIGHT 4  // Minimum height of scrollbar thumb

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief Display configuration structure
 */
typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t lineHeight;
    const uint8_t *font;
} wEui_DisplayConfig_t;

// Include statusbar header after DisplayConfig is defined
#include "wEui_statusbar.h"

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
 * @brief Button pin configuration structure
 */
typedef struct {
    uint8_t upPin;
    uint8_t downPin;
    uint8_t okPin;
    uint8_t backPin;
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
// List Management Functions
// ============================================================================

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
