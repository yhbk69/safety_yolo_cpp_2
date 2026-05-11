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

class IVideoSource {
public:
    virtual ~IVideoSource() = default;
    virtual bool readFrame(cv::Mat& frame) = 0;
    virtual QString name() const = 0;
    virtual int frameDelayMs() const = 0;
};

class CameraVideoSource : public IVideoSource {
public:
    CameraVideoSource(int cameraId, const QString& source = {});
    ~CameraVideoSource() override;

    bool readFrame(cv::Mat& frame) override;
    QString name() const override { return name_; }
    int frameDelayMs() const override { return 33; }

private:
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
    cv::VideoCapture cap_;
    QString name_;
};

#endif // VIDEO_SOURCE_HPP
