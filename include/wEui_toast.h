#ifndef WEUI_TOAST_H
#define WEUI_TOAST_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @file wEui_toast.h
 * @brief wEui Toast/Dialog Component
 *
 * Provides toast notifications and dialog boxes for showing
 * detailed information and status messages.
 *
 * @author Wuqiyang312
 * @version 1.0.0
 */

// ============================================================================
// Configuration Constants
// ============================================================================

#define WEUI_TOAST_MAX_LINES 4           // 最多显示的行数
#define WEUI_TOAST_LINE_LENGTH 32        // 每行最大字符数
#define WEUI_TOAST_TITLE_LENGTH 24       // 标题最大长度
#define WEUI_TOAST_DEFAULT_DURATION 3000 // 默认显示时长（毫秒）
#define WEUI_TOAST_PADDING 4             // 内边距

// ============================================================================
// Type Definitions
// ============================================================================

/**
 * @brief Toast类型
 */
typedef enum {
    WEUI_TOAST_INFO = 0,     // 信息提示
    WEUI_TOAST_SUCCESS,      // 成功提示
    WEUI_TOAST_WARNING,      // 警告提示
    WEUI_TOAST_ERROR,        // 错误提示
    WEUI_TOAST_LOADING       // 加载提示
} wEui_ToastType_t;

/**
 * @brief Toast位置
 */
typedef enum {
    WEUI_TOAST_POS_CENTER = 0,  // 屏幕中央
    WEUI_TOAST_POS_TOP,         // 顶部
    WEUI_TOAST_POS_BOTTOM       // 底部
} wEui_ToastPosition_t;

/**
 * @brief Toast配置结构
 */
typedef struct {
    wEui_ToastType_t type;           // 提示类型
    wEui_ToastPosition_t position;   // 显示位置
    char title[WEUI_TOAST_TITLE_LENGTH];  // 标题
    char lines[WEUI_TOAST_MAX_LINES][WEUI_TOAST_LINE_LENGTH]; // 内容行
    uint8_t lineCount;               // 当前内容行数
    uint32_t duration;               // 显示时长（毫秒），0表示需要手动关闭
    bool showBorder;                 // 是否显示边框
    bool showIcon;                   // 是否显示图标
    bool blockInput;                 // 是否阻塞输入
    bool showProgress;               // 是否显示进度条
    uint8_t progress;                // 进度值(0-100)
} wEui_ToastConfig_t;

/**
 * @brief Toast状态
 */
typedef struct {
    bool active;                     // Toast是否激活
    wEui_ToastConfig_t config;       // 当前配置
    uint32_t startTime;              // 开始显示的时间
    uint8_t animFrame;               // 动画帧计数
} wEui_ToastState_t;

// ============================================================================
// Toast Functions
// ============================================================================

/**
 * @brief 初始化Toast系统
 * @return 0成功，负数表示错误
 */
int wEui_toast_init(void);

/**
 * @brief 反初始化Toast系统
 */
void wEui_toast_deinit(void);

/**
 * @brief 显示简单的信息提示
 * @param message 消息内容
 * @param duration 显示时长（毫秒），0表示使用默认值
 */
void wEui_toast_show(const char *message, uint32_t duration);

/**
 * @brief 显示带标题的信息提示
 * @param title 标题
 * @param message 消息内容
 * @param duration 显示时长（毫秒）
 */
void wEui_toast_showWithTitle(const char *title, const char *message, uint32_t duration);

/**
 * @brief 显示指定类型的提示
 * @param type 提示类型
 * @param title 标题
 * @param message 消息内容
 * @param duration 显示时长（毫秒）
 */
void wEui_toast_showTyped(wEui_ToastType_t type, const char *title, const char *message, uint32_t duration);

/**
 * @brief 显示多行信息
 * @param title 标题
 * @param lines 内容行数组
 * @param lineCount 行数
 * @param duration 显示时长（毫秒）
 */
void wEui_toast_showMultiLine(const char *title, const char **lines, uint8_t lineCount, uint32_t duration);

/**
 * @brief 显示成功提示
 * @param message 消息内容
 */
void wEui_toast_success(const char *message);

/**
 * @brief 显示错误提示
 * @param message 消息内容
 */
void wEui_toast_error(const char *message);

/**
 * @brief 显示警告提示
 * @param message 消息内容
 */
void wEui_toast_warning(const char *message);

/**
 * @brief 显示加载提示
 * @param message 消息内容
 */
void wEui_toast_loading(const char *message);

/**
 * @brief 更新加载进度
 * @param progress 进度值(0-100)
 * @param message 可选的更新消息（NULL保持原消息）
 */
void wEui_toast_updateProgress(uint8_t progress, const char *message);

/**
 * @brief 隐藏当前Toast
 */
void wEui_toast_hide(void);

/**
 * @brief 检查Toast是否正在显示
 * @return true表示正在显示，false表示未显示
 */
bool wEui_toast_isActive(void);

/**
 * @brief 更新Toast状态（需要定期调用）
 * @return 0表示正常，负数表示错误
 */
int wEui_toast_update(void);

/**
 * @brief 渲染Toast到显示缓冲区
 * @param display U8G2显示对象指针
 * @param displayConfig 显示配置
 * @return 0表示成功，负数表示错误
 */
int wEui_toast_render(U8G2 *display, const wEui_DisplayConfig_t *displayConfig);

/**
 * @brief 设置Toast是否阻塞按键输入
 * @param block true阻塞，false不阻塞
 */
void wEui_toast_setBlockInput(bool block);

/**
 * @brief 检查Toast是否阻塞输入
 * @return true表示阻塞，false表示不阻塞
 */
bool wEui_toast_isBlockingInput(void);

/**
 * @brief 获取当前Toast配置
 * @return 当前Toast配置的指针
 */
const wEui_ToastConfig_t* wEui_toast_getConfig(void);

// ============================================================================
// Dialog Functions (Modal Dialogs)
// ============================================================================

/**
 * @brief 对话框按钮类型
 */
typedef enum {
    WEUI_DIALOG_BTN_OK = 0,      // 仅确定按钮
    WEUI_DIALOG_BTN_OK_CANCEL,   // 确定和取消按钮
    WEUI_DIALOG_BTN_YES_NO       // 是和否按钮
} wEui_DialogButtonType_t;

/**
 * @brief 对话框回调函数类型
 * @param result 用户选择的结果（0=取消/否，1=确定/是）
 */
typedef void (*wEui_DialogCallback_t)(uint8_t result);

/**
 * @brief 显示确认对话框
 * @param title 标题
 * @param message 消息内容
 * @param buttonType 按钮类型
 * @param callback 回调函数
 */
void wEui_dialog_show(const char *title, const char *message,
                      wEui_DialogButtonType_t buttonType,
                      wEui_DialogCallback_t callback);

/**
 * @brief 检查对话框是否正在显示
 * @return true表示显示中，false表示未显示
 */
bool wEui_dialog_isActive(void);

/**
 * @brief 处理对话框按键事件
 * @param button 按键类型
 * @return 0表示处理成功，负数表示未处理
 */
int wEui_dialog_handleButton(uint8_t button);

/**
 * @brief 关闭对话框
 */
void wEui_dialog_close(void);

// ============================================================================
// Toast Scroll Functions
// ============================================================================

/**
 * @brief 手动滚动Toast内容
 * @param direction 滚动方向（正数向下，负数向上）
 */
void wEui_toast_scroll(int8_t direction);

/**
 * @brief 检查Toast是否可以滚动
 * @return true表示可以滚动，false表示不可滚动
 */
bool wEui_toast_canScroll(void);

#endif // WEUI_TOAST_H
