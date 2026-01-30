# 迁移到wEui库指南

本指南帮助您将现有的项目迁移到使用wEui库。

## 概览

wEui库将以下组件封装为统一的UI库：
- 列表管理 (`list_manager.h/cpp` -> `wEui_list.cpp`)
- 按钮硬件抽象层 (`button_hal.h/cpp` -> `wEui_button.cpp`)
- UI渲染逻辑 (原本分散在各个任务中 -> `wEui_core.cpp`)

## 迁移步骤

### 1. 替换包含的头文件

**之前:**
```cpp
#include "list_manager.h"
#include "button_hal.h"
```

**现在:**
```cpp
#include "wEui.h"
```

### 2. 初始化库

**之前:**
```cpp
// 分别初始化各个组件
listManagerInit(listMutex);
button_hal_init();
pinMode(BTN_UP_PIN, INPUT_PULLUP);
// ... 其他按钮引脚
```

**现在:**
```cpp
// 统一配置和初始化
wEui_Config_t config = {0};
config.display = &u8g2;
config.displayConfig.width = DISPLAY_WIDTH;
config.displayConfig.height = DISPLAY_HEIGHT;
config.displayConfig.lineHeight = LINE_HEIGHT;
config.displayConfig.font = u8g2_font_6x10_tf;
config.buttonConfig.upPin = BTN_UP_PIN;
config.buttonConfig.downPin = BTN_DOWN_PIN;
config.buttonConfig.okPin = BTN_OK_PIN;
config.buttonConfig.backPin = BTN_BACK_PIN;
config.buttonQueue = buttonQueue;
config.listMutex = listMutex;

wEui_init(&config);
wEui_list_init(listMutex);
```

### 3. 函数调用映射

| 旧函数 | 新函数 |
|--------|--------|
| `listAddItem()` | `wEui_list_addItem()` |
| `listMoveUp()` | `wEui_list_moveUp()` |
| `listMoveDown()` | `wEui_list_moveDown()` |
| `listExecuteSelectedCallback()` | `wEui_list_executeSelected()` |
| `listGetSelectedIndex()` | `wEui_list_getSelectedIndex()` |
| `listGetItemCount()` | `wEui_list_getItemCount()` |
| `button_hal_scan()` | `wEui_button_scan()` |

### 4. UI任务简化

**之前:**
```cpp
void vTaskUI(void *pvParameters) {
    wEui_begin();

    for (;;) {
        u8g2.clearBuffer();

        uint8_t itemCount = listGetItemCount();
        uint8_t selectedIndex = listGetSelectedIndex();
        uint8_t topIndex = listGetTopIndex();

        // 手动渲染逻辑...

        u8g2.sendBuffer();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
```

**现在:**
```cpp
void vTaskUI(void *pvParameters) {
    wEui_begin();

    for (;;) {
        wEui_render();
        wEui_update();
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
```

### 5. 按钮事件处理

**之前:**
```cpp
void vTaskGUIService(void *pvParameters) {
    for (;;) {
        const char* receivedBtn;
        if (xQueueReceive(buttonQueue, &receivedBtn, portMAX_DELAY) == pdPASS) {
            if (strcmp(receivedBtn, "UP") == 0) {
                listMoveUp();
            } else if (strcmp(receivedBtn, "DOWN") == 0) {
                listMoveDown();
            }
            // ... 其他按钮处理
        }
    }
}
```

**现在:**
```cpp
void vTaskGUIService(void *pvParameters) {
    for (;;) {
        wEui_processButtonEvents(portMAX_DELAY);
    }
}
```

### 6. 按钮任务简化

**之前:**
```cpp
void vTaskButtons(void *pvParameters) {
    button_hal_init();
    // 设置回调函数...
    
    for (;;) {
        button_hal_scan();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

**现在:**
```cpp
void vTaskButtons(void *pvParameters) {
    for (;;) {
        wEui_button_scan();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

## 优势

使用wEui库后，您将获得：

1. **代码简化**: 减少重复代码，统一接口
2. **更好的封装**: 隐藏实现细节，专注业务逻辑
3. **易于维护**: 集中管理UI相关代码
4. **线程安全**: 内置互斥锁保护
5. **可扩展性**: 模块化设计，便于扩展功能
6. **文档完整**: 完整的API文档和示例

## 注意事项

1. 确保所有旧的头文件引用都已替换
2. 检查回调函数参数是否匹配
3. 验证按钮引脚配置正确
4. 测试所有UI功能正常工作

## 示例项目

参考 `examples/basic_usage/basic_usage.ino` 获取完整的使用示例。
