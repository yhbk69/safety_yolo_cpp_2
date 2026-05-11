#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QStatusBar>
#include <QThread>
#include <QImage>
#include <QLabel>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDir>
// QTcpServer/QTcpSocket no longer needed directly; HttpFileServer manages them
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QDesktopServices>
#include <QSharedPointer>
#include <atomic>
#include <deque>
#include <unordered_map>
#include <mutex>
#include <memory>

#include <opencv2/opencv.hpp>

#include "gui_logger.hpp"
#include "mjpeg_streamer.hpp"
#include "websocket_manager.hpp"
#include "inference_engine.hpp"
#include "inference_worker.hpp"
#include "http_file_server.hpp"
#include "inference_manager.hpp"
#include "config.hpp"
#include "runtime_config.hpp"
#include "video_recorder.hpp"
#include "model_manager.hpp"
#include "camera_manager.hpp"

namespace Ui { class MainWindow; }


// ============================================================
// MainWindow: 主窗口
// ============================================================
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    static QString getHostIp();

private slots:
    void onOpenImage();
    void onOpenVideo();
    void onOpenCamera(bool checked);
    void onAddCamera();
    void onRemoveCamera(int cameraId);
    void onOpenFolder();
    void onBrowseModel();
    void onLoadModel();
    void onReloadModel();
    void onStopProcessing();
    void onConfThresholdChanged(int value);
    void onNmsThresholdChanged(int value);
    void onBatchInferenceToggled(bool checked);
    void onSettings();
    void onFrameProcessed(int cameraId, QImage image, std::vector<Detection> detections, double elapsedMs);
    void onWorkerFinished(int cameraId);
    void onWorkerError(int cameraId, const QString& message);

    // 告警
    void onAlertSaved(int cameraId, const QString& videoPath, const QString& imagePath, const QString& alertJson);

    // WebSocket 消息处理

private:
    void setupConnections();
    void updateThresholdLabels();
    void processSingleImage(const std::string& path);
    void updateDisplay(const QImage& image);
    void updateDetectionList(const std::vector<Detection>& detections, double elapsedMs);
    void enableControls(bool enabled);
    void startCameraWorker(int cameraId, const QString& name, const QString& source);
    void startVideoWorker(const QString& filePath);
    void stopCamera(int cameraId);
    void stopAllCameras();
    void closeEvent(QCloseEvent* event) override;
    void loadRuntimeConfig();
    void saveRuntimeConfig();

    std::unique_ptr<CameraManager> cameraManager_;
    int activeDisplayCamera_ = 0;  // 当前显示画面的摄像头ID

    Ui::MainWindow* ui;
    QLabel* statusMessageLabel_;
    QLabel* fpsLabel_;
    QLabel* timeLabel_;
    QLabel* wsAddressLabel_;

    // 日志输出函数
    void log(const QString& category, const QString& message);
    QString currentTimestamp();

    std::unique_ptr<ModelManager> modelManager_;
    std::unique_ptr<InferenceManager> inferenceManager_;

    std::unique_ptr<HttpFileServer> httpFileServer_;
    std::unique_ptr<WebSocketManager> wsManager_;  // 新: WebSocket 管理器

    // MJPEG 推流服务
    std::unique_ptr<MjpegStreamer> mjpegStreamer_;

    // 待确认的告警: alarm_id → {json消息, 重试定时器, 重试次数}
    struct PendingAlarm {
        QString jsonMessage;
        QTimer* retryTimer = nullptr;
        int retryCount = 0;
    };

    static constexpr int MAX_RETRY_COUNT = 10;

    // 围栏设置: stream_id → fence区域
    struct FenceRegion {
        float x1, y1, x2, y2;
    };

    std::unique_ptr<VideoRecorder> videoRecorder_;

private slots:
    // 录制相关
    void onStartRecording();
    void onStopRecording();
    void onViewRecordings();
    void onClearOldRecordings();

private:
    bool isProcessing_ = false;  // 视频模式用
    float confThreshold_;
    float nmsThreshold_;
};

#endif // MAINWINDOW_HPP
