#include "../include/wEui.h"

/**
 * @file wEui_button.cpp
 * @brief wEui Button Management Implementation
 */

// ============================================================================
// Internal Variables
// ============================================================================

// Button pin mapping
static uint8_t g_buttonPins[WEUI_BUTTON_COUNT] = {0};
// 按键编号到 GPIO 的映射表

// Button last states for edge detection
static uint8_t g_buttonLastStates[WEUI_BUTTON_COUNT] = {1, 1, 1, 1};
// 上一次的电平状态，用于抓取下降沿按下事件

// Button event callbacks
static wEui_ButtonCallback_t g_buttonCallbacks[WEUI_BUTTON_COUNT] = {nullptr};

// Button queue for event messaging
static QueueHandle_t g_buttonQueue = nullptr;
// 事件队列供默认处理器向上层传递按键事件

// Initialization flag
static bool g_buttonInitialized = false;

// ============================================================================
// Internal Functions
// ============================================================================

/**
 * @brief Read GPIO pin state
 * @param pin GPIO pin number
 * @return Pin state (0 or 1)
 */
static inline uint8_t wEui_button_readGpio(uint8_t pin) {
    // 直接读取对应的 GPIO 电平
    return digitalRead(pin);
}

/**
 * @brief Default button event handler
 * @param button Button that was pressed
 */
static void wEui_button_defaultHandler(wEui_ButtonType_t button) {
    const char* btnName = nullptr;

    // 将按键类型转换为字符串并打印
    switch (button) {
        case WEUI_BUTTON_UP:
            btnName = "UP";
            Serial.println("wEui: UP Pressed");
            break;
        case WEUI_BUTTON_DOWN:
            btnName = "DOWN";
            Serial.println("wEui: DOWN Pressed");
            break;
        case WEUI_BUTTON_OK:
            btnName = "OK";
            Serial.println("wEui: OK Pressed");
            break;
        case WEUI_BUTTON_BACK:
            btnName = "BACK";
            Serial.println("wEui: BACK Pressed");
            break;
        default:
            return;
    }

    // 将事件推送到队列供 wEui_core 处理
    if (btnName != nullptr && g_buttonQueue != nullptr) {
        xQueueSend(g_buttonQueue, &btnName, 0);
    }
}

// ============================================================================
// Button Management Functions Implementation
// ============================================================================

int wEui_button_init(void) {
    if (g_buttonInitialized) {
        return 0; // Already initialized
    }

    // Note: Pin configuration should be set via wEui_button_setPinConfig
    // before calling this function

    // 配置 GPIO 输入并默认标记为未按下
    for (int i = 0; i < WEUI_BUTTON_COUNT; i++) {
        if (g_buttonPins[i] != 0) { // 0 means pin not configured
            pinMode(g_buttonPins[i], INPUT_PULLUP);
            g_buttonLastStates[i] = 1; // Initialize as not pressed (high)
        }
    }

    // Set default callbacks
    for (int i = 0; i < WEUI_BUTTON_COUNT; i++) {
        g_buttonCallbacks[i] = wEui_button_defaultHandler;
    }

    g_buttonInitialized = true;
    return 0;
}

int wEui_button_deinit(void) {
    if (!g_buttonInitialized) {
        return 0;
    }

    // Clear callbacks
    for (int i = 0; i < WEUI_BUTTON_COUNT; i++) {
        g_buttonCallbacks[i] = nullptr;
    }

    g_buttonInitialized = false;
    return 0;
}

int wEui_button_setPinConfig(const wEui_ButtonConfig_t *config) {
    if (config == nullptr) {
        return -1;
    }

    g_buttonPins[WEUI_BUTTON_UP] = config->upPin;
    g_buttonPins[WEUI_BUTTON_DOWN] = config->downPin;
    g_buttonPins[WEUI_BUTTON_OK] = config->okPin;
    g_buttonPins[WEUI_BUTTON_BACK] = config->backPin;

    return 0;
}

int wEui_button_setCallback(wEui_ButtonType_t button, wEui_ButtonCallback_t callback) {
    if (button >= WEUI_BUTTON_COUNT) {
        return -1; // Invalid button
    }

    g_buttonCallbacks[button] = callback;
    return 0;
}

int wEui_button_scan(void) {
    if (!g_buttonInitialized) {
        return -1;
    }

    // 轮询所有已配置按键，检测按下边沿
    for (int i = 0; i < WEUI_BUTTON_COUNT; i++) {
        if (g_buttonPins[i] == 0) {
            continue; // Pin not configured
        }

        uint8_t currentState = wEui_button_readGpio(g_buttonPins[i]);

        // Detect falling edge (button press): HIGH to LOW transition
        if (g_buttonLastStates[i] == 1 && currentState == 0) {
            // Button pressed event
            if (g_buttonCallbacks[i] != nullptr) {
                g_buttonCallbacks[i]((wEui_ButtonType_t)i);
            }
        }

        g_buttonLastStates[i] = currentState;
    }

    return 0;
}

uint8_t wEui_button_read(wEui_ButtonType_t button) {
    if (button >= WEUI_BUTTON_COUNT || g_buttonPins[button] == 0) {
        return 1; // Invalid button or not configured, return not pressed
    }

    return wEui_button_readGpio(g_buttonPins[button]);
}

// ============================================================================
// Utility Functions
// ============================================================================

void wEui_button_setButtonQueue(QueueHandle_t queue) {
    // 保存事件队列指针供默认处理器使用
    g_buttonQueue = queue;
}

bool wEui_button_isInitialized(void) {
    return g_buttonInitialized;
}

const char* wEui_button_getName(wEui_ButtonType_t button) {
    switch (button) {
        case WEUI_BUTTON_UP:    return "UP";
        case WEUI_BUTTON_DOWN:  return "DOWN";
        case WEUI_BUTTON_OK:    return "OK";
        case WEUI_BUTTON_BACK:  return "BACK";
        default:                return "UNKNOWN";
    }
}
