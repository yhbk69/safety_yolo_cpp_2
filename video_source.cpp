/**
 * @file video_source.cpp
 * @brief 视频输入源实现
 */

#include "video_source.hpp"
#include <QStringList>
#include <QRegularExpression>

// 将用户输入标准化为完整 RTSP URL
// 支持格式:
//   192.168.1.100:8554         → rtsp://192.168.1.100:8554/live/stream
//   192.168.1.100              → rtsp://192.168.1.100:554/live/stream
//   rtsp://192.168.1.100:8554  → 保持不变
//   rtsp://admin:pass@ip:554/x → 保持不变
static QString normalizeRtspUrl(const QString& source) {
    // 已经是完整 RTSP URL, 直接返回
    if (source.startsWith("rtsp://") || source.startsWith("rtspt://") || source.startsWith("rtspu://")) {
        return source;
    }

    // 纯 IP:端口 格式: 192.168.1.100:8554
    static QRegularExpression re(R"(^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(\d+)$)");
    QRegularExpressionMatch match = re.match(source);
    if (match.hasMatch()) {
        QString ip = match.captured(1);
        QString port = match.captured(2);
        return QString("rtsp://%1:%2/live/stream").arg(ip).arg(port);
    }

    // 纯 IP (无端口), 使用默认 554
    static QRegularExpression ipRe(R"(^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})$)");
    QRegularExpressionMatch ipMatch = ipRe.match(source);
    if (ipMatch.hasMatch()) {
        return QString("rtsp://%1:554/live/stream").arg(ipMatch.captured(1));
    }

    // 其他格式, 原样返回
    return source;
}

// 为 RTSP URL 追加 FFmpeg 选项 (TCP 传输 + 超时)
static QString appendFfmpegOptions(const QString& url) {
    if (!url.startsWith("rtsp://")) return url;

    QStringList options;
    options << "rtsp_transport=tcp";
    options << "stimeout=5000000";     // 连接超时 5 秒 (微秒)
    options << "rw_timeout=10000000";  // 读取超时 10 秒 (微秒)

    QString result = url;
    if (!result.contains('?')) {
        result += "?";
        result += options.join("&");
    }
    return result;
}

// ============================================================
// CameraVideoSource
// ============================================================

CameraVideoSource::CameraVideoSource(int cameraId, const QString& source) {
    QString normalizedSource = normalizeRtspUrl(source);

    if (!normalizedSource.isEmpty() && normalizedSource.startsWith("rtsp://")) {
        name_ = normalizedSource;
        QString ffmpegUrl = appendFfmpegOptions(normalizedSource);
        cap_.open(ffmpegUrl.toStdString(), cv::CAP_FFMPEG);
    } else {
        bool ok = false;
        int devId = normalizedSource.isEmpty() ? cameraId : normalizedSource.toInt(&ok);
        name_ = QString("camera_%1").arg(ok ? devId : cameraId);
#ifdef _WIN32
        cap_.open(ok ? devId : cameraId, cv::CAP_DSHOW);
#else
        cap_.open(ok ? devId : cameraId);
#endif
    }
    if (cap_.isOpened()) {
        cap_.set(cv::CAP_PROP_BUFFERSIZE, 1);
    }
}

bool CameraVideoSource::validate(const QString& source, int cameraId) {
    cv::VideoCapture cap;
    QString normalizedSource = normalizeRtspUrl(source);

    if (!normalizedSource.isEmpty() && normalizedSource.startsWith("rtsp://")) {
        // 验证阶段使用更短的超时, 避免 UI 卡死
        QString url = normalizedSource;
        if (!url.contains('?')) {
            url += "?rtsp_transport=tcp&stimeout=3000000";  // 3 秒超时
        }
        cap.open(url.toStdString(), cv::CAP_FFMPEG);
    } else {
        bool ok = false;
        int devId = normalizedSource.isEmpty() ? cameraId : normalizedSource.toInt(&ok);
#ifdef _WIN32
        cap.open(ok ? devId : cameraId, cv::CAP_DSHOW);
#else
        cap.open(ok ? devId : cameraId);
#endif
    }
    bool valid = cap.isOpened();
    if (valid) cap.release();
    return valid;
}

CameraVideoSource::~CameraVideoSource() {
    close();
}

void CameraVideoSource::close() {
    std::lock_guard<std::mutex> lock(capMutex_);
    if (cap_.isOpened()) {
        cap_.release();
    }
}

bool CameraVideoSource::readFrame(cv::Mat& frame) {
    std::lock_guard<std::mutex> lock(capMutex_);
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
    std::lock_guard<std::mutex> lock(capMutex_);
    if (cap_.isOpened()) cap_.release();
}

bool FileVideoSource::readFrame(cv::Mat& frame) {
    std::lock_guard<std::mutex> lock(capMutex_);
    if (!cap_.isOpened()) return false;
    return cap_.read(frame);
}
