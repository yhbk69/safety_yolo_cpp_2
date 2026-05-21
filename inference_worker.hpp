/**
 * @file inference_worker.hpp
 * @brief YOLO11 推理工作线程模块
 *
 * 独立于 MainWindow 的后台推理线程，负责从摄像头/视频/RTSP 等输入源
 * 持续读取帧、执行 TensorRT 推理、触发告警、推送帧数据。
 *
 * 设计要点:
 * - 线程安全：环形缓冲区用 mutex 保护
 * - 零拷贝：共享 shared_ptr<cv::Mat> 避免深拷贝
 * - 热切换：InferenceWorker 自身不管理 engine 生命周期，由 MainWindow 控制
 */

#ifndef INFERENCE_WORKER_HPP
#define INFERENCE_WORKER_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QImage>
#include <atomic>
#include <mutex>
#include <deque>
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>

#include <opencv2/opencv.hpp>

#include "inference_engine.hpp"
#include "video_source.hpp"
#include "preprocessor.hpp"
#include "postprocessor.hpp"
#include "types.hpp"
#include "config.hpp"

class MainWindow;

class InferenceWorker : public QObject {
    Q_OBJECT

public:
    explicit InferenceWorker(IEngine* engine, int cameraId = 0,
                             const QString& cameraName = "camera_0");
    ~InferenceWorker() override = default;

    int cameraId() const { return cameraId_; }
    QString cameraName() const { return cameraName_; }

public:
    /// 统一推理入口（非 slot，unique_ptr 无法被 MOC 处理）
    void process(std::unique_ptr<IVideoSource> source, float confThresh, float nmsThresh);

public slots:
    void stop();
    void setBatchInference(bool enabled) { useBatchInference_ = enabled; }

signals:
    void frameProcessed(int cameraId, QImage image, std::vector<Detection> detections, double elapsedMs);
    /** 告警视频已保存: 摄像头ID, 视频路径, 截图路径, JSON消息 */
    void alertSaved(int cameraId, QString videoPath, QString imagePath, QString alertJson);
    void finished(int cameraId);
    void errorOccurred(int cameraId, const QString& message);

private:
    struct FrameResult {
        QImage image;
        std::vector<Detection> detections;
    };

    FrameResult processOneFrame(const cv::Mat& frame, float confThresh, float nmsThresh);
    void checkAlert(const std::vector<Detection>& detections, const std::shared_ptr<cv::Mat>& annotatedFrame);
    void saveAlertFiles(const QString& alarmId, const QString& alarmType);

    IEngine* engine_;
    int cameraId_;
    QString cameraName_;
    std::atomic<bool> running_{false};

    // 批量推理状态
    std::atomic<bool> useBatchInference_{false};
    std::vector<std::vector<float>> batchTensors_;
    std::vector<std::pair<int,int>> batchImgSizes_;
    int batchCounter_ = 0;

    // 环形缓冲区(共享指针避免深拷贝)
    std::mutex bufferMutex_;
    std::deque<std::shared_ptr<cv::Mat>> frameBuffer_;

    // 告警冷却(独立 mutex 保护, 避免与环形缓冲区锁竞争)
    mutable std::mutex alertCooldownMutex_;
    std::unordered_map<int, std::chrono::steady_clock::time_point> lastAlertTime_;

    // 告警录制状态 (用 mutex 保护, 避免数据竞争)
    std::atomic<bool> alertRecording_{false};
    mutable std::mutex alertRecordMutex_;
    int alertRemainingFrames_ = 0;
    std::deque<std::shared_ptr<cv::Mat>> alertBuffer_;
    QString pendingAlarmType_;
    std::shared_ptr<cv::Mat> triggerFrame_;  // 触发告警的那一帧(用于截图)

    // 告警输出目录
    QString outputDir_;
};

#endif // INFERENCE_WORKER_HPP