#include "../include/wEui.h"

/**
 * @file wEui_button.cpp
 * @brief wEui Button Management Implementation
 *
 * 支持多种运行模式：
 * - 独立GPIO模式：每个按钮使用独立的GPIO引脚
 * - ADC共享模式：多个按钮共享同一ADC引脚，通过不同电压范围区分
 * - 混合模式：部分按钮使用GPIO，部分使用ADC
 * - 多ADC组模式：支持多个不同的ADC引脚，每个引脚可以有多个按钮
 */

// ============================================================================
// Internal Variables
// ============================================================================

// 每个按钮的独立配置
static wEui_SingleButtonConfig_t g_buttonConfigs[WEUI_BUTTON_COUNT] = {0};

// ADC全局配置
static uint16_t g_adcDebounceMs = 50;
static uint16_t g_adcResolution = 4095;

// ADC组管理 - 支持多个不同的ADC引脚
typedef struct {
    uint8_t pin;                          // ADC引脚
    uint8_t buttonMask;                   // 使用此引脚的按钮位掩码
    wEui_ButtonType_t lastDetectedButton; // 上次检测到的按钮
    uint32_t lastPressTime;               // 上次按下时间
    bool valid;                           // 是否有效
} wEui_AdcGroup_t;

static wEui_AdcGroup_t g_adcGroups[WEUI_MAX_ADC_GROUPS] = {0};
static uint8_t g_adcGroupCount = 0;

// Button last states for GPIO edge detection
static uint8_t g_buttonLastStates[WEUI_BUTTON_COUNT] = {0, 0, 0, 0};

// Button event callbacks
static wEui_ButtonCallback_t g_buttonCallbacks[WEUI_BUTTON_COUNT] = {nullptr};

// Button queue for event messaging
static QueueHandle_t g_buttonQueue = nullptr;

// Initialization flag
static bool g_buttonInitialized = false;

// ADC mode enabled flag
static bool g_adcModeEnabled = false;

// ============================================================================
// Internal Functions
// ============================================================================

/**
 * @brief Read GPIO pin state
 */
static inline uint8_t wEui_button_readGpio(uint8_t pin) {
    return digitalRead(pin);
}

/**
 * @brief Default button event handler
 */
static void wEui_button_defaultHandler(wEui_ButtonType_t button) {
    const char* btnName = nullptr;

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

    if (btnName != nullptr && g_buttonQueue != nullptr) {
        xQueueSend(g_buttonQueue, &btnName, 0);
    }
}

/**
 * @brief 查找或创建ADC组
 */
static int8_t wEui_button_findOrCreateAdcGroup(uint8_t pin) {
    // 查找现有组
    for (uint8_t i = 0; i < g_adcGroupCount; i++) {
        if (g_adcGroups[i].valid && g_adcGroups[i].pin == pin) {
            return i;
        }
    }

    // 创建新组
    if (g_adcGroupCount < WEUI_MAX_ADC_GROUPS) {
        uint8_t idx = g_adcGroupCount++;
        g_adcGroups[idx].pin = pin;
        g_adcGroups[idx].buttonMask = 0;
        g_adcGroups[idx].lastDetectedButton = WEUI_BUTTON_COUNT;
        g_adcGroups[idx].lastPressTime = 0;
        g_adcGroups[idx].valid = true;
        return idx;
    }

    return -1;
}

/**
 * @brief 检查引脚配置冲突
 */
static void wEui_button_checkPinConflicts(void) {
    for (int i = 0; i < WEUI_BUTTON_COUNT; i++) {
        if (g_buttonConfigs[i].pin == WEUI_PIN_INVALID) continue;

        for (int j = i + 1; j < WEUI_BUTTON_COUNT; j++) {
            if (g_buttonConfigs[j].pin == WEUI_PIN_INVALID) continue;

            // 相同引脚
            if (g_buttonConfigs[i].pin == g_buttonConfigs[j].pin) {
                bool iIsAdc = (g_buttonConfigs[i].pullMode == WEUI_BUTTON_ADC_SHARED);
                bool jIsAdc = (g_buttonConfigs[j].pullMode == WEUI_BUTTON_ADC_SHARED);

                if (iIsAdc != jIsAdc) {
                    // 模式不一致 - 输出警告
                    Serial.printf("wEui WARNING: Pin %d conflict! Button %s uses %s mode, "
                                  "but Button %s uses %s mode on same pin!\n",
                                  g_buttonConfigs[i].pin,
                                  wEui_button_getName((wEui_ButtonType_t)i),
                                  iIsAdc ? "ADC" : "GPIO",
                                  wEui_button_getName((wEui_ButtonType_t)j),
                                  jIsAdc ? "ADC" : "GPIO");
                } else if (!iIsAdc && !jIsAdc) {
                    // 两个都是GPIO模式但共享引脚 - 输出警告
                    Serial.printf("wEui WARNING: Pin %d used by multiple GPIO buttons! "
                                  "Button %s and Button %s share the same GPIO pin. "
                                  "Consider using ADC_SHARED mode.\n",
                                  g_buttonConfigs[i].pin,
                                  wEui_button_getName((wEui_ButtonType_t)i),
                                  wEui_button_getName((wEui_ButtonType_t)j));
                }
            }
        }
    }
}

/**
 * @brief 检查ADC值是否在范围内
 */
static bool wEui_button_isInAdcRange(uint16_t adcValue, const wEui_AdcButtonRange_t *range) {
    if (range->minValue == 0 && range->maxValue == 0) {
        return false;
    }
    return (adcValue >= range->minValue && adcValue <= range->maxValue);
}

/**
 * @brief 在指定ADC组中检测按钮
 */
static wEui_ButtonType_t wEui_button_detectInGroup(uint8_t groupIdx, uint16_t adcValue) {
    if (groupIdx >= g_adcGroupCount || !g_adcGroups[groupIdx].valid) {
        return WEUI_BUTTON_COUNT;
    }

    uint8_t mask = g_adcGroups[groupIdx].buttonMask;

    for (int i = 0; i < WEUI_BUTTON_COUNT; i++) {
        if (mask & (1 << i)) {
            if (wEui_button_isInAdcRange(adcValue, &g_buttonConfigs[i].adcRange)) {
                return (wEui_ButtonType_t)i;
            }
        }
    }

    return WEUI_BUTTON_COUNT;
}

// ============================================================================
// Button Management Functions Implementation
// ============================================================================

int wEui_button_init(void) {
    if (g_buttonInitialized) {
        return 0;
    }

    // 配置GPIO和ADC模式
    for (int i = 0; i < WEUI_BUTTON_COUNT; i++) {
        uint8_t pin = g_buttonConfigs[i].pin;
        wEui_ButtonPullMode_t mode = g_buttonConfigs[i].pullMode;

        if (pin == WEUI_PIN_INVALID) continue;

        switch (mode) {
            case WEUI_BUTTON_PULL_UP:
                pinMode(pin, INPUT_PULLUP);
                g_buttonLastStates[i] = 1;
                break;

            case WEUI_BUTTON_PULL_DOWN:
                pinMode(pin, INPUT_PULLDOWN);
                g_buttonLastStates[i] = 0;
                break;

            case WEUI_BUTTON_FLOATING_RISE:
            case WEUI_BUTTON_FLOATING_FALL:
                pinMode(pin, INPUT);
                g_buttonLastStates[i] = digitalRead(pin);
                break;

            case WEUI_BUTTON_ADC_SHARED:
                g_buttonLastStates[i] = 0;
                break;
        }
    }

    // 设置ADC衰减
    if (g_adcModeEnabled) {
        analogSetAttenuation(ADC_11db);
    }

    // 设置默认回调
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

    // 复制按钮配置
    for (int i = 0; i < WEUI_BUTTON_COUNT; i++) {
        g_buttonConfigs[i] = config->buttons[i];
    }

    // 复制ADC全局配置
    g_adcDebounceMs = config->debounceMs > 0 ? config->debounceMs : 50;
    g_adcResolution = config->adcResolution > 0 ? config->adcResolution : 4095;

    // 重置ADC组
    g_adcGroupCount = 0;
    g_adcModeEnabled = false;
    for (int i = 0; i < WEUI_MAX_ADC_GROUPS; i++) {
        g_adcGroups[i].valid = false;
        g_adcGroups[i].buttonMask = 0;
    }

    // 检查引脚冲突
    wEui_button_checkPinConflicts();

    // 构建ADC组
    for (int i = 0; i < WEUI_BUTTON_COUNT; i++) {
        if (g_buttonConfigs[i].pullMode == WEUI_BUTTON_ADC_SHARED) {
            g_adcModeEnabled = true;

            int8_t groupIdx = wEui_button_findOrCreateAdcGroup(g_buttonConfigs[i].pin);
            if (groupIdx >= 0) {
                g_adcGroups[groupIdx].buttonMask |= (1 << i);
            }
        }
    }

    // 打印ADC组配置信息
    if (g_adcModeEnabled) {
        Serial.printf("wEui: ADC mode enabled with %d group(s)\n", g_adcGroupCount);
        for (uint8_t i = 0; i < g_adcGroupCount; i++) {
            if (g_adcGroups[i].valid) {
                Serial.printf("  Group %d: Pin %d, Buttons: ", i, g_adcGroups[i].pin);
                for (int j = 0; j < WEUI_BUTTON_COUNT; j++) {
                    if (g_adcGroups[i].buttonMask & (1 << j)) {
                        Serial.printf("%s ", wEui_button_getName((wEui_ButtonType_t)j));
                    }
                }
                Serial.println();
            }
        }
    }

    return 0;
}

int wEui_button_setCallback(wEui_ButtonType_t button, wEui_ButtonCallback_t callback) {
    if (button >= WEUI_BUTTON_COUNT) {
        return -1;
    }

    g_buttonCallbacks[button] = callback;
    return 0;
}

int wEui_button_scan(void) {
    if (!g_buttonInitialized) {
        return -1;
    }

    // 扫描GPIO模式的按钮
    for (int i = 0; i < WEUI_BUTTON_COUNT; i++) {
        uint8_t pin = g_buttonConfigs[i].pin;
        wEui_ButtonPullMode_t mode = g_buttonConfigs[i].pullMode;

        if (mode == WEUI_BUTTON_ADC_SHARED) {
            continue;
        }

        uint8_t currentState = wEui_button_readGpio(pin);
        bool buttonPressed = false;

        switch (mode) {
            case WEUI_BUTTON_PULL_UP:
                if (g_buttonLastStates[i] == 1 && currentState == 0) {
                    buttonPressed = true;
                }
                break;

            case WEUI_BUTTON_PULL_DOWN:
                if (g_buttonLastStates[i] == 0 && currentState == 1) {
                    buttonPressed = true;
                }
                break;

            case WEUI_BUTTON_FLOATING_RISE:
                if (g_buttonLastStates[i] == 0 && currentState == 1) {
                    buttonPressed = true;
                }
                break;

            case WEUI_BUTTON_FLOATING_FALL:
                if (g_buttonLastStates[i] == 1 && currentState == 0) {
                    buttonPressed = true;
                }
                break;

            default:
                break;
        }

        if (buttonPressed && g_buttonCallbacks[i] != nullptr) {
            g_buttonCallbacks[i]((wEui_ButtonType_t)i);
        }

        g_buttonLastStates[i] = currentState;
    }

    return 0;
}

wEui_ButtonType_t wEui_button_scanAdc(void) {
    if (!g_adcModeEnabled) {
        return WEUI_BUTTON_COUNT;
    }

    wEui_ButtonType_t result = WEUI_BUTTON_COUNT;
    uint32_t currentTime = millis();

    // 扫描所有ADC组
    for (uint8_t g = 0; g < g_adcGroupCount; g++) {
        if (!g_adcGroups[g].valid) continue;

        uint16_t adcValue = analogRead(g_adcGroups[g].pin);
        wEui_ButtonType_t detectedButton = wEui_button_detectInGroup(g, adcValue);

        // 边沿检测和防抖
        if (detectedButton != g_adcGroups[g].lastDetectedButton) {
            if (detectedButton != WEUI_BUTTON_COUNT) {
                if (currentTime - g_adcGroups[g].lastPressTime >= g_adcDebounceMs) {
                    result = detectedButton;
                    g_adcGroups[g].lastPressTime = currentTime;

                    if (g_buttonCallbacks[detectedButton] != nullptr) {
                        g_buttonCallbacks[detectedButton](detectedButton);
                    }
                }
            }
            g_adcGroups[g].lastDetectedButton = detectedButton;
        }
    }

    return result;
}

uint8_t wEui_button_read(wEui_ButtonType_t button) {
    if (button >= WEUI_BUTTON_COUNT || g_buttonConfigs[button].pin == WEUI_PIN_INVALID) {
        return 1;
    }

    if (g_buttonConfigs[button].pullMode == WEUI_BUTTON_ADC_SHARED) {
        return 1;
    }

    return wEui_button_readGpio(g_buttonConfigs[button].pin);
}

// ============================================================================
// Utility Functions
// ============================================================================

void wEui_button_setButtonQueue(QueueHandle_t queue) {
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

bool wEui_button_isAdcModeEnabled(void) {
    return g_adcModeEnabled;
}

uint8_t wEui_button_getAdcGroupCount(void) {
    return g_adcGroupCount;
}

uint16_t wEui_button_readAdcValue(uint8_t pin) {
    if (pin == WEUI_PIN_INVALID) {
        return 0;
    }
    return analogRead(pin);
}
