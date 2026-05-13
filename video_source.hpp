/**
 * @file video_source.hpp
 * @brief 视频输入源抽象接口
 *
 * 统一封装摄像头、视频文件、RTSP 等输入源，
 * 新增源类型只需实现 IVideoSource 接口。
 */

#ifndef VIDEO_SOURCE_HPP
#define VIDEO_SOURCE_HPP

#include <QString>
#include <opencv2/opencv.hpp>
#include <mutex>

class IVideoSource {
public:
    virtual ~IVideoSource() = default;
    virtual bool readFrame(cv::Mat& frame) = 0;
    virtual QString name() const = 0;
    virtual int frameDelayMs() const = 0;
    virtual bool isLive() const { return false; }
    /// 关闭视频源, 解除 readFrame 阻塞, 线程安全
    virtual void close() {}
};

class CameraVideoSource : public IVideoSource {
public:
    CameraVideoSource(int cameraId, const QString& source = {});
    ~CameraVideoSource() override;

    /// 验证摄像头/RTSP源在当前是否可达(尝试打开后立即关闭)
    static bool validate(const QString& source, int cameraId = 0);

    bool readFrame(cv::Mat& frame) override;
    QString name() const override { return name_; }
    int frameDelayMs() const override { return 0; }  // 实时源无需额外延迟
    bool isLive() const override { return true; }
    void close() override;

private:
    std::mutex capMutex_;
    cv::VideoCapture cap_;
    QString name_;
};

class FileVideoSource : public IVideoSource {
public:
    explicit FileVideoSource(const QString& filePath);
    ~FileVideoSource() override;

    bool readFrame(cv::Mat& frame) override;
    QString name() const override { return name_; }
    int frameDelayMs() const override { return 0; }

private:
    std::mutex capMutex_;
    cv::VideoCapture cap_;
    QString name_;
};

#endif // VIDEO_SOURCE_HPP
