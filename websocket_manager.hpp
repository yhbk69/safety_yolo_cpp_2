/**
 * @file websocket_manager.hpp
 * @brief WebSocket 服务端管理模块
 *
 * 统一管理客户端连接、消息路由、心跳响应、告警广播与重试机制。
 *
 * 设计要点:
 * - 不继承 QObject: 用 lambda 回调代替信号槽,避免 MOC 复杂化
 * - 连接生命周期: 自动管理,客户端断开时自动清理
 * - 告警重试: 内置 ACK 超时重试机制(MAX_RETRY_COUNT)
 * - 线程安全: 客户端列表用 QMutex 保护
 * - 摄像头列表/实时流: 通过回调从外部获取
 */

#ifndef WEBSOCKET_MANAGER_HPP
#define WEBSOCKET_MANAGER_HPP

#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QList>
#include <QMutex>
#include <QTimer>
#include <QMap>
#include <functional>
#include <memory>
#include "output_sink.hpp"

// 围栏区域定义
struct FenceRegion {
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 1.0f;
    float y2 = 1.0f;
};

// 待确认告警信息
struct PendingAlarm {
    QString jsonMessage;    // 告警 JSON
    QTimer* retryTimer;     // 重试定时器
    int retryCount;         // 当前重试次数
};

// 摄像头流信息(供外部提供)
struct StreamInfo {
    QString streamId;
    QString name;
    QString url;
};

class WebSocketManager : public IOutputSink {
public:
    static constexpr int MAX_RETRY_COUNT = 10;  // 最大重试次数

    WebSocketManager();
    ~WebSocketManager();

    /**
     * @brief 初始化并启动 WebSocket 服务
     * @param port 监听端口(默认从 Config::WEBSOCKET_PORT 读取)
     */
    void start(quint16 port = 0);

    /**
     * @brief 停止服务
     */
    void stop();

    /**
     * @brief 服务是否正在运行
     */
    bool isRunning() const;

    /**
     * @brief 获取当前连接数
     */
    int clientCount() const;

    /**
     * @brief 设置日志回调
     * @param callback(category, message)
     */
    void setLogCallback(std::function<void(const QString&, const QString&)> callback);

    // ============================================================
    // 外部数据提供者回调 (由 MainWindow 设置)
    // ============================================================

    /**
     * @brief 设置获取摄像头列表的回调
     * @param callback 返回 StreamInfo 列表
     */
    void setGetStreamsCallback(std::function<QList<StreamInfo>()> callback);

    /**
     * @brief 设置查看实时流的回调
     * @param callback(streamId) -> 返回流 URL
     */
    void setViewStreamCallback(std::function<QString(const QString&)> callback);

    // ============================================================
    // 消息广播与告警管理
    // ============================================================

    /**
     * @brief 广播消息给所有连接的客户端
     * @param message JSON 字符串
     */
    void broadcast(const QString& message);
    void onAlert(const AlertData& data) override;

    /**
     * @brief 推送告警并启动 ACK 重试机制
     * @param alarmId 告警 ID
     * @param alertJson 告警 JSON 字符串
     */
    void pushAlarm(const QString& alarmId, const QString& alertJson);

    /**
     * @brief 推送告警(自动从 alertJson 解析 alarm_id)
     * @param alertJson 告警 JSON 字符串
     *
     * JSON 格式: { "data": { "alarm_id": "...", "alarm_type": "..." } }
     */
    void pushAlarm(const QString& alertJson);

    /**
     * @brief 告警推送回调(供 UI 层更新状态)
     */
    using AlarmPushedCallback = std::function<void(const QString& alarmType, const QString& alarmId)>;
    void setAlarmPushedCallback(AlarmPushedCallback callback);

    /**
     * @brief 确认告警(停止重试)
     * @param alarmId 告警 ID
     */
    void ackAlarm(const QString& alarmId);

    /**
     * @brief 获取待确认告警列表
     */
    QStringList pendingAlarmIds() const;

    /**
     * @brief 同步推送指定 ID 之后的所有待确认告警
     * @param lastAlarmId 上次已知的告警 ID(为空则推送所有)
     */
    void syncAlarms(const QString& lastAlarmId);

    // ============================================================
    // 围栏管理
    // ============================================================

    /**
     * @brief 设置围栏区域
     * @param streamId 流 ID
     * @param fence 围栏区域
     */
    void setFence(const QString& streamId, const FenceRegion& fence);

    /**
     * @brief 获取围栏区域
     * @param streamId 流 ID
     */
    FenceRegion getFence(const QString& streamId) const;

    /**
     * @brief 检查是否有围栏
     */
    bool hasFence(const QString& streamId) const;

    // ============================================================
    // 客户端管理
    // ============================================================

    /**
     * @brief 响应客户端请求
     * @param client 客户端指针
     * @param response JSON 对象
     */
    void replyToClient(QWebSocket* client, const QJsonObject& response);

private:
    void onNewConnection();
    void onTextMessageForClient(QWebSocket* client, const QString& message);
    void onClientDisconnected(QWebSocket* client);
    void handlePing(QWebSocket* client);
    void handleSyncRequest(QWebSocket* client, const QString& lastAlarmId);
    void handleGetStreams(QWebSocket* client);
    void handleSetFence(QWebSocket* client, const QString& streamId, const QJsonObject& fence);
    void handleViewStream(QWebSocket* client, const QString& streamId);
    void retryAlarm(const QString& alarmId);

    QObject* connCtx_ = nullptr;            // 信号槽上下文
    QWebSocketServer* server_ = nullptr;
    quint16 port_ = 0;
    QList<QWebSocket*> clients_;
    mutable QMutex mutex_;

    // 告警重试
    QMap<QString, PendingAlarm> pendingAlarms_;

    // 围栏区域
    QMap<QString, FenceRegion> fenceRegions_;

    // 外部回调
    std::function<QList<StreamInfo>()> getStreamsCallback_;
    std::function<QString(const QString&)> viewStreamCallback_;

    // 日志回调
    std::function<void(const QString&, const QString&)> logCallback_;

    // 告警推送回调
    AlarmPushedCallback alarmPushedCallback_;
};

#endif // WEBSOCKET_MANAGER_HPP
