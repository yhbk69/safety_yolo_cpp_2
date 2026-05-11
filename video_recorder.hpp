/**
 * @file video_recorder.hpp
 * @brief 视频录像管理模块
 *
 * 为多摄像头提供录像功能: 开始/停止录制、写帧、文件管理、清理旧录像。
 *
 * 设计要点:
 * - 线程安全: 写帧和状态管理用 QMutex 保护
 * - 自动管理: RAII 风格, VideoWriter 随 RecordingSession 生命周期自动释放
 * - 按目录组织: RECORD_DIR/YYYYMMDD/camera_ID/
 * - 文件命名: camera_ID_HHMMSS.mp4 (带时间戳)
 * - 不继承 QObject: 用回调代替信号槽,避免 MOC 复杂化
 */

#ifndef VIDEO_RECORDER_HPP
#define VIDEO_RECORDER_HPP

#include <QString>
#include <QDateTime>
#include <QMutex>
#include <opencv2/opencv.hpp>
#include <functional>
#include <memory>
#include <map>
#include "output_sink.hpp"

// 录制会话(每个摄像头一个)
struct RecordingSession {
    int cameraId = 0;
    QString alias;              // 摄像头别名
    QString source;             // 摄像头源地址(用于文件夹命名)
    QString videoPath;          // 当前录像文件路径
    cv::VideoWriter writer;     // OpenCV 录像 writer
    bool isRecording = false;
    QDateTime startTime;        // 开始时间
    int frameWidth = 1920;      // 帧宽度
    int frameHeight = 1080;     // 帧高度
    double fps = 30.0;          // 帧率
};

class VideoRecorder : public IOutputSink {
public:
    VideoRecorder();
    ~VideoRecorder();

    /**
     * @brief 设置回调函数
     */
    struct Callbacks {
        // 日志回调: (category, message)
        std::function<void(const QString&, const QString&)> log;
        // 更新 UI 按钮状态: (isRecording)
        std::function<void(bool)> updateButtons;
    };
    void setCallbacks(const Callbacks& callbacks);

    // ============================================================
    // 录制控制
    // ============================================================

    /**
     * @brief 开始录制
     * @param cameraId 摄像头 ID
     * @param fps 帧率(默认 30)
     * @return 成功返回 true
     */
    bool startRecording(int cameraId, double fps = 30.0, const QString& alias = {}, const QString& source = {});

    /**
     * @brief 停止录制
     * @param cameraId 摄像头 ID
     * @return 成功返回 true
     */
    bool stopRecording(int cameraId);

    /**
     * @brief 检查是否正在录制
     * @param cameraId 摄像头 ID
     */
    bool isRecording(int cameraId) const;

    /**
     * @brief 停止所有录制
     */
    void stopAll();

    /**
     * @brief 获取所有正在录制的摄像头 ID
     */
    QList<int> recordingCameraIds() const;

    // ============================================================
    // 写帧
    // ============================================================

    /**
     * @brief 写入一帧到录像文件
     * @param cameraId 摄像头 ID
     * @param frame RGB 格式的 cv::Mat
     * @note 如果未录制或 writer 未打开,自动跳过
     */
    void writeFrame(int cameraId, const cv::Mat& frame);
    void onFrame(const FrameData& data) override;

    // ============================================================
    // 文件管理
    // ============================================================

    /**
     * @brief 获取录像根目录
     */
    QString recordDir() const;

    /**
     * @brief 获取指定摄像头的录像目录
     * @param cameraId 摄像头 ID
     */
    QString cameraRecordDir(int cameraId) const;

    /**
     * @brief 清理旧录像
     * @param keepDays 保留天数(小于该天数的目录将被删除)
     * @return 删除的目录数
     */
    int cleanOldRecordings(int keepDays);

    /**
     * @brief 打开录像目录(供 UI 调用)
     */
    void openRecordDir() const;

private:
    RecordingSession* getOrCreateSession(int cameraId);
    void releaseSession(int cameraId);
    QString getRecordDirPath() const;
    QString getDateDirPath() const;
    QString getCameraDirPath(int cameraId) const;

    mutable QMutex mutex_;
    std::map<int, std::unique_ptr<RecordingSession>> sessions_;
    Callbacks callbacks_;
};

#endif // VIDEO_RECORDER_HPP
