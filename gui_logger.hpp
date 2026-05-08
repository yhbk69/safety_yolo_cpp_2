/**
 * @file gui_logger.hpp
 * @brief GUI日志输出工具(带颜色高亮)
 *
 * 独立于 MainWindow 的日志工具类,负责在 QTextEdit 中输出带时间戳和分类颜色的日志。
 *
 * 设计要点:
 * - UTF-8 编码: 所有中文字符串使用 QString::fromUtf8() 确保正确显示
 * - 分类颜色: 根据不同日志类别自动设置颜色
 * - 线程安全: 仅在主线程(UI线程)调用
 */

#ifndef GUI_LOGGER_HPP
#define GUI_LOGGER_HPP

#include <QString>
#include <QTextEdit>
#include <QDateTime>

class GuiLogger {
public:
    /**
     * @brief 在 QTextEdit 中输出带颜色的日志
     * @param logWidget 日志显示控件
     * @param category 日志类别(如 "系统", "检测", "告警" 等)
     * @param message 日志内容
     *
     * 颜色规则:
     * - 告警/错误: 红色 (#e74c3c)
     * - 检测: 绿色 (#27ae60)
     * - 录像: 橙色 (#e67e22)
     * - WebSocket/MJPEG: 蓝色 (#3498db)
     * - 配置: 紫色 (#9b59b6)
     * - 系统: 深灰 (#34495e)
     * - 其他: 灰色 (#7f8c8d)
     */
    static void log(QTextEdit* logWidget, const QString& category, const QString& message);

    /**
     * @brief 获取当前时间戳(格式: HH:MM:SS)
     * @return 时间戳字符串
     */
    static QString currentTimestamp();

    // 预定义的日志类别常量(避免重复字符串字面量)
    inline static const QString CATEGORY_SYSTEM = QString::fromUtf8("系统");
    inline static const QString CATEGORY_DETECT = QString::fromUtf8("检测");
    inline static const QString CATEGORY_ALERT = QString::fromUtf8("告警");
    inline static const QString CATEGORY_ERROR = QString::fromUtf8("错误");
    inline static const QString CATEGORY_RECORD = QString::fromUtf8("录像");
    inline static const QString CATEGORY_WEBSOCKET = QString::fromUtf8("WebSocket");
    inline static const QString CATEGORY_MJPEG = QString::fromUtf8("MJPEG");
    inline static const QString CATEGORY_CONFIG = QString::fromUtf8("配置");
    inline static const QString CATEGORY_MODEL = QString::fromUtf8("模型");
};

#endif // GUI_LOGGER_HPP
