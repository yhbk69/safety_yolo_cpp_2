#include "alert_websocket_server.hpp"
#include "config.hpp"
#include <QObject>
#include <QHostAddress>
#include <QFileInfo>

AlertWebSocketServer::AlertWebSocketServer()
    : connCtx_(new QObject())
{
}

AlertWebSocketServer::~AlertWebSocketServer() {
    stop();
    delete connCtx_;
}

void AlertWebSocketServer::start(quint16 port) {
    stop();

    server_ = new QWebSocketServer("YOLO11-Alert", QWebSocketServer::NonSecureMode, connCtx_);
    if (!server_->listen(QHostAddress::Any, port)) {
        if (logCallback_) {
            logCallback_(QStringLiteral("告警"), QStringLiteral("服务启动失败, 端口: %1").arg(port));
        }
        delete server_;
        server_ = nullptr;
        return;
    }

    QObject::connect(server_, &QWebSocketServer::newConnection, connCtx_, [this]() {
        onNewConnection();
    });

    if (logCallback_) {
        logCallback_(QStringLiteral("告警"), QStringLiteral("服务已启动 ws://0.0.0.0:%1").arg(port));
    }
}

void AlertWebSocketServer::stop() {
    if (!server_) return;

    // 先停止服务器
    server_->close();
    delete server_;
    server_ = nullptr;

    // 关闭所有客户端 (释放锁后关闭, 避免 deadlock)
    QList<QWebSocket*> toClose;
    {
        QMutexLocker locker(&mutex_);
        toClose = clients_;
        clients_.clear();
    }
    for (auto* client : toClose) {
        if (client) {
            client->close();
            client->deleteLater();
        }
    }
}

bool AlertWebSocketServer::isRunning() const {
    return server_ && server_->isListening();
}

int AlertWebSocketServer::clientCount() const {
    QMutexLocker locker(&mutex_);
    return clients_.size();
}

void AlertWebSocketServer::onAlert(const AlertData& data) {
    pushAlert(data);
}

void AlertWebSocketServer::pushAlert(const AlertData& data) {
    QMutexLocker locker(&mutex_);
    if (clients_.isEmpty()) {
        if (logCallback_) {
            logCallback_(QStringLiteral("告警"), QStringLiteral("无客户端连接, 丢弃告警"));
        }
        return;
    }
    if (data.alertJson.isEmpty()) {
        if (logCallback_) {
            logCallback_(QStringLiteral("告警"), QStringLiteral("alertJson 为空, 丢弃告警"));
        }
        return;
    }

    if (logCallback_) {
        logCallback_(QStringLiteral("告警"), QStringLiteral("开始推送告警: imagePath=%1, videoPath=%2").arg(data.imagePath).arg(data.videoPath));
    }

    // 1. 发送告警元数据 (JSON 文本帧)
    int sentCount = 0;
    for (auto* client : clients_) {
        if (client && client->state() == QAbstractSocket::ConnectedState) {
            client->sendTextMessage(data.alertJson);
            sentCount++;
        }
    }
    if (logCallback_) {
        logCallback_(QStringLiteral("告警"), QStringLiteral("已发送告警元数据到 %1 个客户端").arg(sentCount));
    }

    // 2. 发送告警截图 (二进制 JPEG)
    if (!data.imagePath.isEmpty()) {
        QFile imgFile(data.imagePath);
        if (imgFile.open(QIODevice::ReadOnly)) {
            QByteArray imgData = imgFile.readAll();
            imgFile.close();
            if (!imgData.isEmpty()) {
                int imgSent = 0;
                for (auto* client : clients_) {
                    if (client && client->state() == QAbstractSocket::ConnectedState) {
                        client->sendBinaryMessage(imgData);
                        imgSent++;
                    }
                }
                if (logCallback_) {
                    logCallback_(QStringLiteral("告警"), QStringLiteral("已推送截图(%1 bytes)到 %2 个客户端").arg(imgData.size()).arg(imgSent));
                }
            }
        } else if (logCallback_) {
            logCallback_(QStringLiteral("告警"), QStringLiteral("无法打开截图文件: %1").arg(data.imagePath));
        }
    }

    // 3. 发送告警视频 (二进制 MP4)
    if (!data.videoPath.isEmpty()) {
        QFile vidFile(data.videoPath);
        if (vidFile.open(QIODevice::ReadOnly)) {
            QByteArray vidData = vidFile.readAll();
            vidFile.close();
            if (!vidData.isEmpty()) {
                int vidSent = 0;
                for (auto* client : clients_) {
                    if (client && client->state() == QAbstractSocket::ConnectedState) {
                        client->sendBinaryMessage(vidData);
                        vidSent++;
                    }
                }
                if (logCallback_) {
                    logCallback_(QStringLiteral("告警"), QStringLiteral("已推送视频(%1 bytes)到 %2 个客户端").arg(vidData.size()).arg(vidSent));
                }
            }
        } else if (logCallback_) {
            logCallback_(QStringLiteral("告警"), QStringLiteral("无法打开视频文件: %1").arg(data.videoPath));
        }
    }
}

void AlertWebSocketServer::setLogCallback(std::function<void(const QString&, const QString&)> callback) {
    logCallback_ = std::move(callback);
}

void AlertWebSocketServer::onNewConnection() {
    auto* socket = server_->nextPendingConnection();
    if (!socket) return;

    {
        QMutexLocker locker(&mutex_);
        clients_.append(socket);
    }

    QObject::connect(socket, &QWebSocket::disconnected, connCtx_, [this, socket]() {
        onClientDisconnected(socket);
    });

    if (logCallback_) {
        logCallback_(QStringLiteral("告警"), QStringLiteral("新客户端已连接"));
    }
}

void AlertWebSocketServer::onClientDisconnected(QWebSocket* socket) {
    if (!socket) return;

    {
        QMutexLocker locker(&mutex_);
        clients_.removeAll(socket);
    }

    if (logCallback_) {
        logCallback_(QStringLiteral("告警"), QStringLiteral("客户端已断开"));
    }

    socket->deleteLater();
}
