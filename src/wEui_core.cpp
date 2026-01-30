#include "../include/wEui.h"
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

    // Initialize display with proper error checking
    if (g_display != NULL) {
        // Initialize display hardware
        if (!g_display->begin()) {
            Serial.println("wEui: Display initialization failed!");
            return -4;
        }

        // Set display properties
        g_display->setFont(g_displayConfig.font);
        g_display->setFontRefHeightExtendedText();
        g_display->setDrawColor(1);
        g_display->setFontPosTop();
        g_display->clearBuffer();

        // Test display by sending buffer
        g_display->sendBuffer();

        Serial.println("wEui: Display initialized successfully");
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

    // Draw startup screen
    g_display->drawStr(0, 0, "wEui v1.0.0");
    g_display->drawStr(0, 12, "Initializing...");
    g_display->drawStr(0, 36, "Display: OK");
    g_display->drawStr(0, 48, "Ready!");

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
    uint8_t selectedIndex = wEui_list_getSelectedIndex();
    uint8_t topIndex = wEui_list_getTopIndex();
    uint8_t cursorPos = wEui_list_getCursorPosition();

    if (itemCount == 0) {
        // Show empty list message
        g_display->setDrawColor(1);
        g_display->drawStr((g_displayConfig.width - 60) / 2,
                          (g_displayConfig.height - g_displayConfig.lineHeight) / 2,
                          "No items");
        return 0;
    }

    // Draw list border/frame
    g_display->setDrawColor(1);
    g_display->drawFrame(0, 0, g_displayConfig.width - 8, g_displayConfig.height);

    // Calculate content area
    uint8_t contentX = 2;
    uint8_t contentY = 2;
    uint8_t contentWidth = g_displayConfig.width - 12;

    // Render list items with enhanced visuals
    for (uint8_t i = 0; i < WEUI_VISIBLE_LINES && (topIndex + i) < itemCount; i++) {
        uint8_t itemIndex = topIndex + i;
        uint8_t yPos = contentY + (i * g_displayConfig.lineHeight);

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
            g_display->drawStr(contentX + 1, yPos, ">");
        } else {
            // Normal item - ensure normal drawing color
            g_display->setDrawColor(1);
            g_display->drawStr(contentX + 1, yPos, " ");
        }

        // Draw item name with proper offset
        const char* itemName = wEui_list_getItemName(itemIndex);
        if (itemName && strlen(itemName) > 0) {
            g_display->drawStr(contentX + 8, yPos, itemName);
        }

        // Reset draw color for next iteration
        g_display->setDrawColor(1);
    }

    // Draw enhanced scrollbar if needed
    if (itemCount > WEUI_VISIBLE_LINES) {
        uint8_t scrollBarX = g_displayConfig.width - 6;
        uint8_t scrollBarY = 2;
        uint8_t scrollBarMaxHeight = g_displayConfig.height - 4;

        // Calculate scrollbar dimensions
        uint8_t scrollBarHeight = (WEUI_VISIBLE_LINES * scrollBarMaxHeight) / itemCount;
        if (scrollBarHeight < 4) scrollBarHeight = 4; // Minimum height

        uint8_t scrollBarPos = scrollBarY +
                              (topIndex * (scrollBarMaxHeight - scrollBarHeight)) /
                              (itemCount - WEUI_VISIBLE_LINES);

        // Draw scrollbar background
        g_display->setDrawColor(1);
        g_display->drawFrame(scrollBarX, scrollBarY, 3, scrollBarMaxHeight);

        // Draw scrollbar thumb
        g_display->drawBox(scrollBarX + 1, scrollBarPos, 1, scrollBarHeight);
    }

    // Draw list status info (item counter)
    if (itemCount > 0) {
        char statusStr[16];
        snprintf(statusStr, sizeof(statusStr), "%d/%d", selectedIndex + 1, itemCount);

        // Position status at bottom right
        uint8_t statusX = g_displayConfig.width -
                         (strlen(statusStr) * 6) - 2; // Approximate character width
        uint8_t statusY = g_displayConfig.height - g_displayConfig.lineHeight + 2;

        g_display->setDrawColor(1);
        g_display->drawStr(statusX, statusY, statusStr);
    }

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
