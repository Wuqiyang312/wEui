# wEui库示例说明

本目录包含了wEui库的完整示例代码，帮助您快速上手并了解各种功能的使用方法。

## 示例目录结构

```
examples/
├── 01_basic_example/          # 基础使用示例
├── 02_statusbar_example/      # 状态栏功能示例
├── 03_page_management/        # 页面管理系统示例
├── 04_complete_example/       # 完整应用示例
└── README.md                  # 本文件
```

## 示例说明

### 01_basic_example - 基础使用示例
展示wEui库的基本功能：
- 初始化wEui系统
- 创建简单的菜单列表
- 处理按钮事件
- UI渲染和显示

**适合人群**: 初次使用wEui的开发者

### 02_statusbar_example - 状态栏功能示例
演示状态栏的各种使用方法：
- 启用/禁用状态栏
- 动态更新状态信息
- 显示系统状态

**适合人群**: 需要状态显示功能的项目

### 03_page_management - 页面管理系统示例
展示高级页面管理功能：
- 创建多级菜单页面
- 页面堆栈管理
- 自定义页面渲染
- 页面间导航

**适合人群**: 需要复杂界面的项目

### 04_complete_example - 完整应用示例
综合演示wEui的所有功能：
- 完整的应用程序结构
- 实际使用场景
- 最佳实践示例

**适合人群**: 希望了解完整应用开发流程的开发者

## 使用建议

1. **初学者**: 从 `01_basic_example` 开始，理解基本概念
2. **进阶用户**: 学习 `02_statusbar_example` 和 `03_page_management`
3. **项目开发**: 参考 `04_complete_example` 了解最佳实践

## 硬件要求

所有示例都需要以下硬件：
- ESP32开发板
- SSD1306 OLED显示屏 (128x64)
- 4个按钮 (上、下、确定、返回)
- 连接线

## 编译说明

1. 使用PlatformIO IDE
2. 将示例代码复制到您的项目中
3. 确保wEui库已正确安装
4. 根据您的硬件调整引脚配置
5. 编译并上传到设备

## 引脚配置

默认引脚配置（可根据实际硬件修改）：
```cpp
#define I2C_SDA_PIN   4      // I2C数据引脚
#define I2C_SCL_PIN   5      // I2C时钟引脚
#define BTN_UP_PIN    3      // 上按钮
#define BTN_DOWN_PIN  2      // 下按钮
#define BTN_OK_PIN    1      // 确定按钮
#define BTN_BACK_PIN  0      // 返回按钮
```

## 获取帮助

如果遇到问题：
1. 检查硬件连接
2. 确认引脚配置
3. 查看串口输出信息
4. 参考库文档

## 贡献

欢迎提交改进建议和新的示例代码！
