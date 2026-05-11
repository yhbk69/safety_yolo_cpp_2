/**
 * @file video_source.cpp
 * @brief 视频输入源实现
 */

#include "video_source.hpp"

// ============================================================
// CameraVideoSource
// ============================================================

CameraVideoSource::CameraVideoSource(int cameraId, const QString& source) {
    if (!source.isEmpty() && source.startsWith("rtsp://")) {
        name_ = source;
        cap_.open(source.toStdString());
    } else {
        bool ok = false;
        int devId = source.isEmpty() ? cameraId : source.toInt(&ok);
        name_ = QString("camera_%1").arg(ok ? devId : cameraId);
#ifdef _WIN32
        cap_.open(ok ? devId : cameraId, cv::CAP_DSHOW);
#else
        cap_.open(ok ? devId : cameraId);
#endif
    }
    if (cap_.isOpened()) {
        cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);
        // 设置读取超时, 避免网络断开时 cap_.read() 永久阻塞
        cap_.set(cv::CAP_PROP_OPEN_TIMEOUT_MSEC, 3000);
    }
}

CameraVideoSource::~CameraVideoSource() {
    if (cap_.isOpened()) cap_.release();
}

bool CameraVideoSource::readFrame(cv::Mat& frame) {
    if (!cap_.isOpened()) return false;
    return cap_.read(frame);
}

// ============================================================
// FileVideoSource
// ============================================================

FileVideoSource::FileVideoSource(const QString& filePath)
    : name_(filePath)
{
    cap_.open(filePath.toStdString());
}

FileVideoSource::~FileVideoSource() {
    if (cap_.isOpened()) cap_.release();
}

bool FileVideoSource::readFrame(cv::Mat& frame) {
    if (!cap_.isOpened()) return false;
    return cap_.read(frame);
}
