#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QStatusBar>
#include <QThread>
#include <QImage>
#include <QLabel>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDir>
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QDesktopServices>
#include <QSharedPointer>
#include <atomic>
#include <memory>
#include <unordered_map>

#include <opencv2/opencv.hpp>

#include "gui_logger.hpp"
#include "inference_engine.hpp"
#include "inference_worker.hpp"
#include "http_file_server.hpp"
#include "inference_manager.hpp"
#include "config.hpp"
#include "runtime_config.hpp"
#include "output_sink.hpp"
#include "model_manager.hpp"
#include "camera_manager.hpp"

class VideoRecorder;

namespace Ui { class MainWindow; }


// ============================================================
// MainWindow: 主窗口
// ============================================================
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

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
    bool isDetecting() const;
    void updateModelButtons(bool detecting);
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

    // 输出通道 (MJPEG, WebSocket 告警, 录像 等)
    std::vector<std::unique_ptr<IOutputSink>> sinks_;
    VideoRecorder* videoRecorder_ = nullptr;  // 别名, UI 控制用

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
    std::unordered_map<int, int> logFrameCounts_;
};

#endif // MAINWINDOW_HPP
