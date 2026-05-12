#ifndef ALERT_WEBSOCKET_SERVER_HPP
#define ALERT_WEBSOCKET_SERVER_HPP

#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QList>
#include <QMutex>
#include <QFile>
#include <functional>
#include "output_sink.hpp"

class AlertWebSocketServer : public IOutputSink {
public:
    AlertWebSocketServer();
    ~AlertWebSocketServer();

    void start(quint16 port);
    void stop();
    bool isRunning() const;
    int clientCount() const;
    void onAlert(const AlertData& data) override;
    void setLogCallback(std::function<void(const QString&, const QString&)> callback);

private:
    void onNewConnection();
    void onClientDisconnected(QWebSocket* socket);
    void pushAlert(const AlertData& data);

    QObject* connCtx_;
    QWebSocketServer* server_ = nullptr;
    QList<QWebSocket*> clients_;
    mutable QMutex mutex_;
    std::function<void(const QString&, const QString&)> logCallback_;
};

#endif // ALERT_WEBSOCKET_SERVER_HPP
