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

static bool g_wEui_initialized = false;
static U8G2 *g_display = NULL;
static wEui_DisplayConfig_t g_displayConfig = {0};
static wEui_ButtonConfig_t g_buttonConfig = {0};
static QueueHandle_t g_buttonQueue = NULL;
static SemaphoreHandle_t g_listMutex = NULL;
static uint8_t g_actualVisibleLines = WEUI_VISIBLE_LINES;  // Actual visible lines based on display height

// Library version
static const char* WEUI_VERSION = "1.0.0";

// ============================================================================
// Core Functions Implementation
// ============================================================================

int wEui_init(const wEui_Config_t *config) {
    if (config == NULL) {
        return -1;
    }

    if (g_wEui_initialized) {
        return 0; // Already initialized
    }

    // Store configuration
    g_display = config->display;
    g_displayConfig = config->displayConfig;
    g_buttonConfig = config->buttonConfig;
    g_buttonQueue = config->buttonQueue;
    g_listMutex = config->listMutex;

    // Initialize status bar module
    if (wEui_statusBar_init() != 0) {
        Serial.println("wEui: Status bar initialization failed!");
        return -5;
    }

    // Initialize display with proper error checking
    if (g_display != NULL) {
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
        Serial.println((g_displayConfig.font != NULL) ? "OK" : "NULL");
    } else {
        Serial.println("wEui: Warning - No display configured");
    }

    // Set button pin configuration
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

    g_display = NULL;
    g_buttonQueue = NULL;
    g_listMutex = NULL;

    // Clean up status bar module
    wEui_statusBar_deinit();

    g_wEui_initialized = false;

    return 0;
}

int wEui_begin(void) {
    if (!g_wEui_initialized || g_display == NULL) {
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

int wEui_render(void) {
    if (!g_wEui_initialized || g_display == NULL) {
        return -1;
    }

    g_display->clearBuffer();

    // Get list state
    uint8_t itemCount = wEui_list_getItemCount();
    uint8_t topIndex = wEui_list_getTopIndex();
    uint8_t cursorPos = wEui_list_getCursorPosition();

    // Status bar is always enabled, reserve space for it
    uint8_t statusBarHeight = wEui_statusBar_getHeight();
    uint8_t contentMaxHeight = g_displayConfig.height - statusBarHeight;

    // Calculate actual visible lines based on content height
    // Account for border (4 pixels total: 2 top + 2 bottom)
    uint8_t availableContentHeight = contentMaxHeight - 4;
    g_actualVisibleLines = availableContentHeight / g_displayConfig.lineHeight;

    // Ensure at least 1 line is visible and not more than WEUI_VISIBLE_LINES
    if (g_actualVisibleLines == 0) {
        g_actualVisibleLines = 1;
    }
    if (g_actualVisibleLines > WEUI_VISIBLE_LINES) {
        g_actualVisibleLines = WEUI_VISIBLE_LINES;
    }

    if (itemCount == 0) {
        // Show empty list message in Chinese
        g_display->setDrawColor(1);
        g_display->setFont(g_displayConfig.font);  // Ensure font is set
        const char* emptyMsg = "空";
        int msgWidth = g_display->getStrWidth(emptyMsg);
        g_display->setCursor((g_displayConfig.width - msgWidth) / 2,
                          (contentMaxHeight - g_displayConfig.lineHeight) / 2);
        g_display->print(emptyMsg);
    } else {
        // Calculate scrollbar area
        uint8_t scrollbarX = g_displayConfig.width - WEUI_SCROLLBAR_WIDTH;
        uint8_t contentAreaWidth = g_displayConfig.width - WEUI_SCROLLBAR_WIDTH - 2;

        // Draw list border/frame (excluding scrollbar area)
        g_display->setDrawColor(1);
        g_display->drawFrame(0, 0, contentAreaWidth, contentMaxHeight);

        // Draw scrollbar
        wEui_list_renderScrollbar(g_display, scrollbarX, 0, WEUI_SCROLLBAR_WIDTH, contentMaxHeight);

        // Calculate content area
        uint8_t contentX = 2;
        uint8_t contentY = 2;
        uint8_t contentWidth = contentAreaWidth - 4;

        // Ensure font is set for text rendering
        g_display->setFont(g_displayConfig.font);

        // Use actual visible lines calculated above
        uint8_t maxVisibleItems = g_actualVisibleLines;

        // Render list items with enhanced visuals
        for (uint8_t i = 0; i < maxVisibleItems && (topIndex + i) < itemCount; i++) {
            uint8_t itemIndex = topIndex + i;
            uint8_t yPos = contentY + (i * g_displayConfig.lineHeight);

            // Ensure we don't draw beyond content area
            if (yPos + g_displayConfig.lineHeight > contentMaxHeight - 2) {
                break;
            }

            // Check if this is the selected item
            bool isSelected = (i == cursorPos);

            if (isSelected) {
                // Draw selection background box
                g_display->setDrawColor(1);
                g_display->drawBox(contentX, yPos - 1,
                                  contentWidth, g_displayConfig.lineHeight);

                // Switch to inverse color for selected item text
                g_display->setDrawColor(0);

                // Draw selection arrow/indicator
                g_display->setCursor(contentX + 1, yPos);
                g_display->print(">");
            } else {
                // Normal item - ensure normal drawing color
                g_display->setDrawColor(1);
            }

            // Draw item name with proper offset
            const char* itemName = wEui_list_getItemName(itemIndex);
            if (itemName && strlen(itemName) > 0) {
                g_display->setCursor(contentX + 8, yPos);
                g_display->print(itemName);
            }

            // Reset draw color for next iteration
            g_display->setDrawColor(1);
        }
    }

    // Always render status bar at bottom
    wEui_statusBar_render(g_display, &g_displayConfig);

    return 0;
}

int wEui_update(void) {
    if (!g_wEui_initialized || g_display == NULL) {
        return -1;
    }

    g_display->sendBuffer();
    return 0;
}

// ============================================================================
// Event Processing Functions
// ============================================================================

int wEui_processButtonEvents(uint32_t timeout) {
    if (g_buttonQueue == NULL) {
        return -1;
    }

    const char* receivedBtn;
    if (xQueueReceive(g_buttonQueue, &receivedBtn, timeout) == pdPASS) {

        // Check for long press events (if implemented in button handler)
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
            // Handle normal button presses
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
    // Default handlers are implemented in processButtonEvents
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
    if (g_display != NULL && font != NULL) {
        g_display->setFont(font);
        g_displayConfig.font = font;
    }
}

uint8_t wEui_list_getActualVisibleLines(void) {
    return g_actualVisibleLines;
}

