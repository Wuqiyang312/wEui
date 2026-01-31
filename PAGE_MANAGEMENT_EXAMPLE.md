/**
 * @file wEui_page_example.md
 * @brief wEui页面管理使用示例
 */

# wEui页面管理使用示例

## 基本用法

### 1. 创建页面

```cpp
// 创建列表页面
int mainMenuId = wEui_page_createList("主菜单");
int settingsId = wEui_page_createList("设置");

// 创建自定义页面
int aboutId = wEui_page_createCustom("关于", aboutPageRender);
```

### 2. 页面堆栈操作

```cpp
// 推送页面到堆栈（显示页面）
wEui_page_push(mainMenuId);

// 进入子页面
wEui_page_push(settingsId);

// 返回上一页面
wEui_page_pop();
```

### 3. 自定义页面渲染

```cpp
void aboutPageRender(U8G2 *display, const wEui_DisplayConfig_t *displayConfig, uint8_t contentHeight) {
    display->setDrawColor(1);
    display->setFont(displayConfig->font);
    
    // 居中显示标题
    const char* title = "wEui v1.0.0";
    int titleWidth = display->getStrWidth(title);
    display->setCursor((displayConfig->width - titleWidth) / 2, 10);
    display->print(title);
    
    // 显示描述文本
    display->setCursor(4, 30);
    display->print("嵌入式UI库");
    
    display->setCursor(4, 45);
    display->print("作者: Wuqiyang312");
}
```

### 4. 完整示例

```cpp
#include "wEui.h"

// 自定义页面渲染函数
void settingsPageRender(U8G2 *display, const wEui_DisplayConfig_t *config, uint8_t height);
void aboutPageRender(U8G2 *display, const wEui_DisplayConfig_t *config, uint8_t height);

// 菜单项回调函数
void mainMenu_settings(uint8_t index);
void mainMenu_about(uint8_t index);
void settings_back(uint8_t index);

// 页面ID全局变量
int g_mainMenuId = -1;
int g_settingsListId = -1;
int g_aboutId = -1;

void setup() {
    // ... wEui初始化代码 ...
    
    // 创建页面
    g_mainMenuId = wEui_page_createList("主菜单");
    g_settingsListId = wEui_page_createList("设置菜单");  
    g_aboutId = wEui_page_createCustom("关于", aboutPageRender);
    
    // 设置主菜单页面为当前列表上下文并添加菜单项
    wEui_page_push(g_mainMenuId);
    wEui_page_switchListContext(g_mainMenuId);
    wEui_list_addItem("设置", mainMenu_settings);
    wEui_list_addItem("关于", mainMenu_about);
    
    // 设置状态栏显示当前页面
    wEui_statusBar_setText("主菜单");
}

void loop() {
    // 处理按键事件
    wEui_processButtonEvents(pdMS_TO_TICKS(50));
    
    // 渲染显示
    wEui_render();
    wEui_update();
    
    vTaskDelay(pdMS_TO_TICKS(50));
}

// 主菜单回调函数
void mainMenu_settings(uint8_t index) {
    // 清理当前列表内容
    wEui_list_clear();
    
    // 推送设置页面
    wEui_page_push(g_settingsListId);
    wEui_page_switchListContext(g_settingsListId);
    
    // 添加设置菜单项
    wEui_list_addItem("WiFi设置", nullptr);
    wEui_list_addItem("显示设置", nullptr);
    wEui_list_addItem("返回", settings_back);
    
    wEui_statusBar_setText("设置");
}

void mainMenu_about(uint8_t index) {
    // 推送关于页面
    wEui_page_push(g_aboutId);
    wEui_statusBar_setText("关于");
}

void settings_back(uint8_t index) {
    // 弹出当前页面，返回上一页面
    wEui_page_pop();
    
    // 恢复主菜单内容
    wEui_list_clear();
    wEui_page_switchListContext(g_mainMenuId);
    wEui_list_addItem("设置", mainMenu_settings);
    wEui_list_addItem("关于", mainMenu_about);
    
    wEui_statusBar_setText("主菜单");
}

// 自定义页面渲染函数
void aboutPageRender(U8G2 *display, const wEui_DisplayConfig_t *config, uint8_t height) {
    display->setDrawColor(1);
    display->setFont(config->font);
    
    // 居中显示标题
    const char* title = "wEui v1.0.0";
    int titleWidth = display->getStrWidth(title);
    display->setCursor((config->width - titleWidth) / 2, 15);
    display->print(title);
    
    // 显示其他信息
    display->setCursor(4, 30);
    display->print("轻量级嵌入式UI库");
    
    display->setCursor(4, 45);
    display->print("支持页面堆栈管理");
    
    // 显示返回提示
    display->setCursor(4, height - 15);
    display->print("按BACK键返回");
}
```

## 特性说明

### 页面堆栈
- 支持最多10层页面堆栈
- 自动管理页面可见性
- 只有栈顶页面会被渲染

### 状态栏常驻
- 状态栏始终显示在屏幕底部
- 不受页面切换影响
- 可用于显示当前页面名称或其他状态信息

### 页面类型
- **列表页面**: 使用内置的列表管理系统
- **自定义页面**: 使用自定义渲染回调函数

### 多页面列表支持
- 每个列表页面可以有独立的菜单内容
- 通过`wEui_page_switchListContext`切换列表上下文
- 支持动态添加/删除菜单项

## API总览

### 页面管理
- `wEui_page_init()` - 初始化页面管理系统
- `wEui_page_createList()` - 创建列表页面
- `wEui_page_createCustom()` - 创建自定义页面
- `wEui_page_push()` - 推送页面到堆栈
- `wEui_page_pop()` - 从堆栈弹出页面
- `wEui_page_getCurrentId()` - 获取当前页面ID
- `wEui_page_getType()` - 获取页面类型
- `wEui_page_getName()` - 获取页面名称
- `wEui_page_getStackDepth()` - 获取堆栈深度
- `wEui_page_switchListContext()` - 切换列表上下文

这个页面管理系统让您能够创建复杂的多级菜单界面，同时保持代码的简洁和可维护性。
