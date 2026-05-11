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
    } else if (!source.isEmpty()) {
        bool ok = false;
        int devId = source.toInt(&ok);
        name_ = QString("camera_%1").arg(ok ? devId : cameraId);
        cap_.open(ok ? devId : cameraId, cv::CAP_DSHOW);
    } else {
        name_ = QString("camera_%1").arg(cameraId);
        cap_.open(cameraId, cv::CAP_DSHOW);
    }
}

CameraVideoSource::~CameraVideoSource() {
    if (cap_.isOpened()) cap_.release();
}

bool CameraVideoSource::readFrame(cv::Mat& frame) {
    if (!cap_.isOpened()) return false;
    cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);
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
