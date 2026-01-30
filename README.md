# wEui - 嵌入式系统轻量级UI库

wEui 是为嵌入式系统设计的简单高效的UI库，提供列表管理、按钮处理和基于U8g2的显示渲染功能。

## 功能特性

- **列表管理**：支持滚动的动态列表
- **按钮处理**：按钮输入的硬件抽象层
- **显示渲染**：基于U8g2的显示渲染
- **线程安全**：FreeRTOS互斥锁支持
- **轻量级**：针对资源受限系统优化
- **模块化设计**：清晰的关注点分离

## 硬件要求

- ESP32 或兼容微控制器
- SSD1306 OLED显示屏（128x64）
- 4个按钮（上、下、确定、返回）
- I2C接口用于显示通信

## 安装方法

1. 将 `wEui` 文件夹复制到 PlatformIO 的 `lib` 目录
2. 在项目中引入库：`#include "wEui.h"`
3. 配置硬件引脚并初始化库

## 快速开始

```cpp
#include <Arduino.h>
#include <U8g2lib.h>
#include "wEui.h"

// 硬件设置
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 5, 4);
QueueHandle_t buttonQueue;  // FreeRTOS 队列句柄，用于按钮事件
SemaphoreHandle_t listMutex;  // FreeRTOS 互斥信号量，保护列表操作

void setup() {
    // 创建 FreeRTOS 对象
    buttonQueue = xQueueCreate(10, sizeof(const char*));
    listMutex = xSemaphoreCreateMutex();
    
    // 配置 wEui
    wEui_Config_t config = {0};
    config.display = &u8g2;  // 显示器指针
    config.displayConfig.width = 128;  // 显示宽度
    config.displayConfig.height = 64;  // 显示高度
    config.displayConfig.lineHeight = 12;  // 文本行高
    config.displayConfig.font = u8g2_font_6x10_tf;  // 字体
    config.buttonConfig.upPin = 3;  // 上按钮引脚
    config.buttonConfig.downPin = 2;  // 下按钮引脚
    config.buttonConfig.okPin = 1;  // 确定按钮引脚
    config.buttonConfig.backPin = 0;  // 返回按钮引脚
    config.buttonQueue = buttonQueue;  // 按钮队列
    config.listMutex = listMutex;  // 列表互斥锁
    
    // 初始化 wEui
    wEui_init(&config);
    wEui_list_init(listMutex);
    
    // 添加菜单项
    wEui_list_addItem("设置", onSettings);
    wEui_list_addItem("关于", onAbout);
    
    wEui_begin();
}
```

## API 参考

### 核心函数

#### `wEui_init(const wEui_Config_t *config)`
使用配置初始化 wEui 库。

#### `wEui_begin()`
启动UI渲染系统。

#### `wEui_render()`
将当前UI状态渲染到显示缓冲区。

#### `wEui_update()`
将缓冲区更新到屏幕显示。

### 列表管理

#### `wEui_list_addItem(const char *itemName, wEui_ItemCallback_t callback)`
向列表中添加项目并关联回调函数。

#### `wEui_list_moveUp()` / `wEui_list_moveDown()`
在列表项目间导航。

#### `wEui_list_executeSelected()`
执行当前选中项目的回调函数。

### 按钮处理

#### `wEui_button_scan()`
扫描按钮状态（应在任务中周期性调用）。

#### `wEui_processButtonEvents(uint32_t timeout)`
处理来自队列的按钮事件。

## 配置说明

### 显示配置
```cpp
wEui_DisplayConfig_t displayConfig = {
    .width = 128,  // 显示宽度像素数
    .height = 64,  // 显示高度像素数
    .lineHeight = 12,  // 每行文本高度
    .font = u8g2_font_6x10_tf  // 字体选择
};
```

### 按钮配置
```cpp
wEui_ButtonConfig_t buttonConfig = {
    .upPin = 3,  // 向上按钮GPIO引脚
    .downPin = 2,  // 向下按钮GPIO引脚
    .okPin = 1,  // 确定按钮GPIO引脚
    .backPin = 0  // 返回按钮GPIO引脚
};
```

## 架构说明

wEui 采用模块化架构设计：

- **wEui_core.cpp**: 库的主体初始化和渲染逻辑
- **wEui_list.cpp**: 列表管理和导航功能
- **wEui_button.cpp**: 按钮输入处理和硬件抽象
- **wEui.h**: 公共API接口

## 线程安全

所有列表操作都由 FreeRTOS 互斥锁保护。按钮事件通过 FreeRTOS 队列处理，确保线程安全通信。

## 示例

参见 `examples/basic_usage/` 目录获取完整的工作示例。

## 许可证

MIT 许可证 - 详见 LICENSE 文件。

## 贡献指南

欢迎贡献！提交拉取请求前请阅读贡献指南。
