#ifndef WEUI_STATUSBAR_H
#define WEUI_STATUSBAR_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @file wEui_statusbar.h
 * @brief wEui Status Bar Component
 *
 * Manages the status bar display at the bottom of the screen.
 * The status bar is always enabled and reserves space at the bottom.
 *
 * @author Wuqiyang312
 * @version 1.0.0
 */

// ============================================================================
// Configuration Constants
// ============================================================================

#define WEUI_STATUS_BAR_LENGTH 64  // Maximum status bar text length
#define WEUI_STATUS_BAR_HEIGHT 14  // Status bar height (lineHeight + padding)

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief Status bar configuration structure
 */
typedef struct {
    bool showBorder;
    char text[WEUI_STATUS_BAR_LENGTH];
} wEui_StatusBar_t;

// Note: wEui_DisplayConfig_t must be defined before including this header

// ============================================================================
// Status Bar Functions
// ============================================================================

/**
 * @brief Initialize status bar system
 * @return 0 on success, negative on error
 */
int wEui_statusBar_init(void);

/**
 * @brief Deinitialize status bar system
 */
void wEui_statusBar_deinit(void);

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

/**
 * @brief Get the height reserved for the status bar
 * @return Status bar height in pixels
 */
uint8_t wEui_statusBar_getHeight(void);

/**
 * @brief Render the status bar
 * @param display Pointer to U8G2 display object
 * @param displayConfig Display configuration
 */
void wEui_statusBar_render(U8G2 *display, const wEui_DisplayConfig_t *displayConfig);

#endif // WEUI_STATUSBAR_H
