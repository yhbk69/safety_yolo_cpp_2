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
#include "yolo_trt_engine.hpp"
#include "inference_worker.hpp"
#include "http_file_server.hpp"
#include "config.hpp"
#include "runtime_config.hpp"

namespace Ui { class MainWindow; }


// ============================================================
// MainWindow: 涓荤獥鍙?
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

    // 鍛婅
    void onAlertSaved(int cameraId, const QString& videoPath, const QString& imagePath, const QString& alertJson);

    // WebSocket 娑堟伅澶勭悊

private:
    void setupConnections();
    void updateThresholdLabels();
    void processSingleImage(const std::string& path);
    void updateDisplay(const QImage& image);
    void updateDetectionList(const std::vector<Detection>& detections, double elapsedMs);
    void enableControls(bool enabled);
    void stopCamera(int cameraId);
    void stopAllCameras();
    void closeEvent(QCloseEvent* event) override;
    void loadRuntimeConfig();
    void saveRuntimeConfig();

    // 澶氭憚鍍忓ご绠＄悊
    struct CameraWorker {
        QThread* thread = nullptr;
        InferenceWorker* worker = nullptr;
        QLabel* displayLabel = nullptr;  // 璇ヨ矾瀵瑰簲鐨勬樉绀烘爣绛?
    };
    QMap<int, CameraWorker> cameraWorkers_;
    int nextCameraId_ = 1;  // 涓嬩竴涓彲鐢ㄧ殑鎽勫儚澶碔D(0淇濈暀缁欓粯璁ゆ憚鍍忓ご鎸夐挳)
    int activeDisplayCamera_ = 0;  // 褰撳墠鏄剧ず鐢婚潰鐨勬憚鍍忓ごID

    Ui::MainWindow* ui;
    QLabel* statusMessageLabel_;
    QLabel* fpsLabel_;
    QLabel* timeLabel_;
    QLabel* wsAddressLabel_;

    // 鏃ュ織杈撳嚭鍑芥暟
    void log(const QString& category, const QString& message);
    QString currentTimestamp();

    std::unique_ptr<YoloTrtEngine> engine_;

    std::unique_ptr<HttpFileServer> httpFileServer_;
    std::unique_ptr<WebSocketManager> wsManager_;  // 新: WebSocket 管理器

    // MJPEG 鎺ㄦ祦鏈嶅姟
    std::unique_ptr<MjpegStreamer> mjpegStreamer_;

    // 寰呯‘璁ょ殑鍛婅: alarm_id 鈫?{json娑堟伅, 閲嶈瘯瀹氭椂鍣? 閲嶈瘯娆℃暟}
    struct PendingAlarm {
        QString jsonMessage;
        QTimer* retryTimer = nullptr;
        int retryCount = 0;
    };

    static constexpr int MAX_RETRY_COUNT = 10;

    // 鍥存爮璁剧疆: stream_id 鈫?fence鍖哄煙
    struct FenceRegion {
        float x1, y1, x2, y2;
    };

    // ====== 瑙嗛褰曞埗 ======
    struct CameraRecording {
        int cameraId = 0;
        QString videoPath;        // 褰撳墠褰曞儚鏂囦欢璺緞
        cv::VideoWriter* writer = nullptr;  // OpenCV褰曞儚 writer
        bool isRecording = false;  // 鏀逛负鏅€歜ool
        QDateTime startTime;   // 寮€濮嬫椂闂?
    };
    QMap<int, CameraRecording*> cameraRecordings_;  // 鐢ㄥ師濮嬫寚閽?
    QString getRecordDir(int cameraId);  // 鑾峰彇褰曞儚鐩綍
    void writeRecordingFrame(int cameraId, const cv::Mat& frame);  // 鍐欏抚鍒板綍鍍?

private slots:
    // 褰曞埗鐩稿叧
    void onStartRecording();
    void onStopRecording();
    void onViewRecordings();
    void onClearOldRecordings();

private:
    bool isProcessing_ = false;  // 瑙嗛妯″紡鐢?
    float confThreshold_;
    float nmsThreshold_;
};

#endif // MAINWINDOW_HPP
