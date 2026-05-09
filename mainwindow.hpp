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
#include "yolo_trt_engine.hpp"
#include "inference_worker.hpp"
#include "http_file_server.hpp"
#include "config.hpp"
#include "runtime_config.hpp"

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
    void onWsClientConnected();
    void onWsTextMessage(const QString& message);
    void retryAlarm(const QString& alarmId);

    // WebSocket 消息处理
    void handleSyncRequest(const QString& lastAlarmId);
    void handleGetStreams();
    void handleSetFence(const QString& streamId, const QJsonObject& fence);
    void handleViewStream(const QString& streamId);

private:
    void setupConnections();
    void updateThresholdLabels();
    void processSingleImage(const std::string& path);
    void updateDisplay(const QImage& image);
    void updateDetectionList(const std::vector<Detection>& detections, double elapsedMs);
    void enableControls(bool enabled);
    void stopCamera(int cameraId);
    void stopAllCameras();
    void startWebSocketServer();
    void startHttpFileServer();
    void closeEvent(QCloseEvent* event) override;
    void loadRuntimeConfig();
    void saveRuntimeConfig();

    // 多摄像头管理
    struct CameraWorker {
        QThread* thread = nullptr;
        InferenceWorker* worker = nullptr;
        QLabel* displayLabel = nullptr;  // 该路对应的显示标签
    };
    QMap<int, CameraWorker> cameraWorkers_;
    int nextCameraId_ = 1;  // 下一个可用的摄像头ID(0保留给默认摄像头按钮)
    int activeDisplayCamera_ = 0;  // 当前显示画面的摄像头ID

    Ui::MainWindow* ui;
    QLabel* statusMessageLabel_;
    QLabel* fpsLabel_;
    QLabel* timeLabel_;
    QLabel* wsAddressLabel_;

    // 日志输出函数
    void log(const QString& category, const QString& message);
    QString currentTimestamp();

    std::unique_ptr<YoloTrtEngine> engine_;

    QWebSocketServer* wsServer_ = nullptr;
    QList<QWebSocket*> wsClients_;
    std::unique_ptr<HttpFileServer> httpFileServer_;

    // MJPEG 推流服务
    std::unique_ptr<MjpegStreamer> mjpegStreamer_;

    // 待确认的告警: alarm_id → {json消息, 重试定时器, 重试次数}
    struct PendingAlarm {
        QString jsonMessage;
        QTimer* retryTimer = nullptr;
        int retryCount = 0;
    };
    QMap<QString, PendingAlarm> pendingAlarms_;

    static constexpr int MAX_RETRY_COUNT = 10;

    // 围栏设置: stream_id → fence区域
    struct FenceRegion {
        float x1, y1, x2, y2;
    };
    QMap<QString, FenceRegion> fenceRegions_;

    // ====== 视频录制 ======
    struct CameraRecording {
        int cameraId = 0;
        QString videoPath;        // 当前录像文件路径
        cv::VideoWriter* writer = nullptr;  // OpenCV录像 writer
        bool isRecording = false;  // 改为普通bool
        QDateTime startTime;   // 开始时间
    };
    QMap<int, CameraRecording*> cameraRecordings_;  // 用原始指针
    QString getRecordDir(int cameraId);  // 获取录像目录
    void writeRecordingFrame(int cameraId, const cv::Mat& frame);  // 写帧到录像

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
