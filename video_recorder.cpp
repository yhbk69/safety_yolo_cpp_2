/**
 * @file video_recorder.cpp
 * @brief 视频录像管理模块实现
 */

#include "video_recorder.hpp"
#include "config.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include <QRegularExpression>
#include <algorithm>

VideoRecorder::VideoRecorder() {
}

VideoRecorder::~VideoRecorder() {
    stopAll();
}

void VideoRecorder::setCallbacks(const Callbacks& callbacks) {
    QMutexLocker locker(&mutex_);
    callbacks_ = callbacks;
}

// ============================================================
// 录制控制
// ============================================================

bool VideoRecorder::startRecording(int cameraId, double fps, const QString& alias, const QString& source) {
    QMutexLocker locker(&mutex_);

    // 检查是否已经在录制
    auto it = sessions_.find(cameraId);
    if (it != sessions_.end() && it->second->isRecording) {
        if (callbacks_.log) {
            callbacks_.log("录像", QString("摄像头%1 已经在录制中").arg(cameraId));
        }
        return false;
    }

    // 创建或获取会话
    auto* session = getOrCreateSession(cameraId);
    if (!session) return false;
    if (!alias.isEmpty())  session->alias  = alias;
    if (!source.isEmpty()) session->source = source;

    // 获取录像目录
    QString cameraDir = getCameraDirPath(cameraId);
    if (cameraDir.isEmpty()) return false;

    // 生成文件名: camera_ID_HHMMSS.mp4
    QString startTime = session->startTime.toString("HHmmss");
    session->videoPath = QString("%1/camera_%2_%3.mp4").arg(cameraDir).arg(cameraId).arg(startTime);
    session->fps = fps;
    session->isRecording = true;
    // writer 延迟到 writeFrame 首次调用时创建(此时才能拿到真实帧尺寸)

    if (callbacks_.log) {
        callbacks_.log("录像", QString("摄像头%1 开始录制: %2").arg(cameraId).arg(session->videoPath));
    }

    // 更新 UI 按钮
    if (callbacks_.updateButtons) {
        callbacks_.updateButtons(true);
    }

    return true;
}

bool VideoRecorder::stopRecording(int cameraId) {
    QMutexLocker locker(&mutex_);

    auto it = sessions_.find(cameraId);
    if (it == sessions_.end() || !it->second->isRecording) {
        if (callbacks_.log) {
            callbacks_.log("录像", QString("摄像头%1 未在录制中").arg(cameraId));
        }
        return false;
    }

    auto* session = it->second.get();
    QDateTime endTime = QDateTime::currentDateTime();

    // 释放 writer
    session->writer.release();

    // 重命名文件,添加结束时间
    QString startTimeStr = session->startTime.toString("HHmmss");
    QString endTimeStr = endTime.toString("HHmmss");
    QString oldPath = session->videoPath;
    QFileInfo fi(oldPath);
    QString newPath = fi.absolutePath() + "/" + fi.completeBaseName() + "-" + endTimeStr + ".mp4";

    if (QFile::exists(oldPath)) {
        QFile::rename(oldPath, newPath);
        session->videoPath = newPath;
    }

    session->isRecording = false;

    if (callbacks_.log) {
        callbacks_.log("录像", QString("摄像头%1 录制完成: %2").arg(cameraId).arg(newPath));
    }

    // 更新 UI 按钮
    if (callbacks_.updateButtons) {
        callbacks_.updateButtons(false);
    }

    return true;
}

bool VideoRecorder::isRecording(int cameraId) const {
    QMutexLocker locker(&mutex_);
    auto it = sessions_.find(cameraId);
    return it != sessions_.end() && it->second->isRecording;
}

void VideoRecorder::stopAll() {
    QMutexLocker locker(&mutex_);
    for (auto& [id, session] : sessions_) {
        if (session->isRecording) {
            session->writer.release();
            session->isRecording = false;
        }
    }
    sessions_.clear();
}

QList<int> VideoRecorder::recordingCameraIds() const {
    QMutexLocker locker(&mutex_);
    QList<int> ids;
    for (const auto& [id, session] : sessions_) {
        if (session->isRecording) {
            ids.append(id);
        }
    }
    return ids;
}

// ============================================================
// 写帧
// ============================================================

void VideoRecorder::onFrame(const FrameData& data) {
    if (data.image.isNull()) return;
    QImage conv = data.image.convertToFormat(QImage::Format_RGB888);
    cv::Mat rgb(conv.height(), conv.width(), CV_8UC3,
                const_cast<uchar*>(conv.constBits()), conv.bytesPerLine());
    writeFrame(data.cameraId, rgb);
}

void VideoRecorder::writeFrame(int cameraId, const cv::Mat& frame) {
    QMutexLocker locker(&mutex_);

    auto it = sessions_.find(cameraId);
    if (it == sessions_.end()) return;

    auto* session = it->second.get();
    if (!session->isRecording) return;

    // 首次写入时延迟创建 VideoWriter(此时才能拿到真实帧尺寸)
    if (!session->writer.isOpened()) {
        int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
        cv::Size frameSize(frame.cols, frame.rows);
        if (!session->writer.open(session->videoPath.toStdString(), fourcc, session->fps, frameSize)) {
            // avc1 不可用, 回退到 mp4v
            fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
            if (!session->writer.open(session->videoPath.toStdString(), fourcc, session->fps, frameSize)) {
                if (callbacks_.log) {
                    callbacks_.log("录像", QString("摄像头%1 视频编码器初始化失败").arg(cameraId));
                }
                session->isRecording = false;
                return;
            }
        }
        session->frameWidth  = frame.cols;
        session->frameHeight = frame.rows;
        if (callbacks_.log) {
            callbacks_.log("录像", QString("摄像头%1 编码器: %2, 分辨率: %3x%4")
                .arg(cameraId).arg(fourcc == cv::VideoWriter::fourcc('a','v','c','1') ? "H.264" : "MP4V")
                .arg(frame.cols).arg(frame.rows));
        }
    }

    // 将 RGB 转为 BGR (OpenCV 需要)
    cv::Mat bgr;
    cv::cvtColor(frame, bgr, cv::COLOR_RGB2BGR);

    session->writer.write(bgr);
}

// ============================================================
// 文件管理
// ============================================================

QString VideoRecorder::recordDir() const {
    return getRecordDirPath();
}

QString VideoRecorder::cameraRecordDir(int cameraId) const {
    return getCameraDirPath(cameraId);
}

int VideoRecorder::cleanOldRecordings(int keepDays) {
    QMutexLocker locker(&mutex_);

    QString baseDir = getRecordDirPath();
    QDir dir(baseDir);
    if (!dir.exists()) return 0;

    QDateTime cutoff = QDateTime::currentDateTime().addDays(-keepDays);
    int deletedCount = 0;

    QFileInfoList dateDirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& dateDirInfo : dateDirs) {
        QDateTime dirTime = dateDirInfo.lastModified();
        if (dirTime < cutoff) {
            QDir subDir(dateDirInfo.absoluteFilePath());
            subDir.removeRecursively();
            deletedCount++;
            if (callbacks_.log) {
                callbacks_.log("录像", QString("删除旧录像目录: %1").arg(dateDirInfo.fileName()));
            }
        }
    }

    if (callbacks_.log && deletedCount > 0) {
        callbacks_.log("录像", QString("清理完成, 删除 %1 个目录").arg(deletedCount));
    }

    return deletedCount;
}

void VideoRecorder::openRecordDir() const {
    QString dir = getRecordDirPath();
    QDir().mkpath(dir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

// ============================================================
// 内部方法
// ============================================================

RecordingSession* VideoRecorder::getOrCreateSession(int cameraId) {
    // 调用者已持有 mutex_
    auto it = sessions_.find(cameraId);
    if (it == sessions_.end() || !it->second) {
        auto session = std::make_unique<RecordingSession>();
        session->cameraId = cameraId;
        session->startTime = QDateTime::currentDateTime();
        session->isRecording = false;
        sessions_[cameraId] = std::move(session);
    }
    return sessions_[cameraId].get();
}

void VideoRecorder::releaseSession(int cameraId) {
    // 调用者已持有 mutex_
    sessions_.erase(cameraId);
}

QString VideoRecorder::getRecordDirPath() const {
    return QDir::cleanPath(QDir::currentPath() + "/" + QString::fromStdString(Config::RECORD_DIR));
}

QString VideoRecorder::getDateDirPath() const {
    QString baseDir = getRecordDirPath();
    QString dateDir = QDateTime::currentDateTime().toString("yyyyMMdd");
    return baseDir + "/" + dateDir;
}

QString VideoRecorder::getCameraDirPath(int cameraId) const {
    QString dateDir = getDateDirPath();
    QString cameraDir;

    auto it = sessions_.find(cameraId);
    if (it != sessions_.end()) {
        QString alias = it->second->alias;
        QString src   = it->second->source;
        // 构造: alias_src  (不包含协议前缀)
        if (!alias.isEmpty()) cameraDir = alias;
        if (!src.isEmpty()) {
            QString shortSrc = src;
            // 去掉 rtsp:// / http:// 前缀
            if (shortSrc.startsWith("rtsp://"))  shortSrc = shortSrc.mid(7);
            if (shortSrc.startsWith("http://"))  shortSrc = shortSrc.mid(7);
            // 替换特殊字符为下划线
            shortSrc.replace(QRegularExpression(R"([:\/\?@#&=\.\s,;!])"), "_");
            if (shortSrc.length() > 40) shortSrc = shortSrc.left(40);
            if (!cameraDir.isEmpty()) cameraDir += "_";
            cameraDir += shortSrc;
        }
    }
    if (cameraDir.isEmpty()) cameraDir = QString("camera_%1").arg(cameraId);

    QString fullPath = dateDir + "/" + cameraDir;
    QDir().mkpath(fullPath);
    return fullPath;
}
