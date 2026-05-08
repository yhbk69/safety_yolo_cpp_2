/**
 * @file gui_logger.cpp
 * @brief GUI日志输出工具实现(带颜色高亮)
 *
 * 注意: 所有中文字符串必须使用 QString::fromUtf8() 包裹,
 * 或者直接使用 UTF-8 编码的字符串字面量(配合 /utf-8 编译选项)。
 */

#include "gui_logger.hpp"

void GuiLogger::log(QTextEdit* logWidget, const QString& category, const QString& message) {
    if (!logWidget) return;

    QString timestamp = currentTimestamp();

    // 根据不同类别设置颜色
    QString color;
    if (category == CATEGORY_ALERT || category == CATEGORY_ERROR) {
        color = "#e74c3c";  // 红色 - 告警/错误
    } else if (category == CATEGORY_DETECT) {
        color = "#27ae60";  // 绿色 - 检测
    } else if (category == CATEGORY_RECORD) {
        color = "#e67e22";  // 橙色 - 录像
    } else if (category == CATEGORY_WEBSOCKET || category == CATEGORY_MJPEG) {
        color = "#3498db";  // 蓝色 - 网络相关
    } else if (category == CATEGORY_CONFIG) {
        color = "#9b59b6";  // 紫色 - 配置
    } else if (category == CATEGORY_SYSTEM) {
        color = "#34495e";  // 深灰 - 系统
    } else if (category == CATEGORY_MODEL) {
        color = "#5cb85c";  // 绿色 - 模型
    } else {
        color = "#7f8c8d";  // 灰色 - 其他
    }

    QString formatted = QString("<span style='color:%1; font-size:22px;'>[%2][%3] %4</span>")
        .arg(color, timestamp, category, message);

    logWidget->append(formatted);
}

QString GuiLogger::currentTimestamp() {
    return QDateTime::currentDateTime().toString("hh:mm:ss");
}
