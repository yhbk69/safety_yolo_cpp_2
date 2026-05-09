/**
 * @file inference_worker.cpp
 * @brief YOLO11 推理工作线程实现
 */

#include "inference_worker.hpp"

#include <QDir>
#include <QUuid>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QBuffer>
#include <QThread>

InferenceWorker::InferenceWorker(YoloTrtEngine* engine, int cameraId,
                                     const QString& cameraName,
                                     const QString& source)
    : engine_(engine)
    , cameraId_(cameraId)
    , cameraName_(cameraName)
    , source_(source)
{
    outputDir_ = QDir::cleanPath(QDir::currentPath() + "/" +
        QString::fromStdString(Config::OUTPUT_DIR));
    QDir().mkpath(outputDir_);
}

void InferenceWorker::processVideo(const QString& path, float confThresh, float nmsThresh) {
    cv::VideoCapture cap(path.toStdString());
    if (!cap.isOpened()) {
        emit errorOccurred(cameraId_, QString::fromUtf8("无法打开视频文件: ") + path);
        emit finished(cameraId_);
        return;
    }
    running_ = true;
    while (running_) {
        cv::Mat frame;
        if (!cap.read(frame)) break;
        auto result = processOneFrame(frame, confThresh, nmsThresh);
        emit frameProcessed(cameraId_, result.image, result.detections, 0);
    }
    cap.release();
    emit finished(cameraId_);
}

void InferenceWorker::processCamera(float confThresh, float nmsThresh) {
    cv::VideoCapture cap;
    if (!source_.isEmpty() && source_.startsWith("rtsp://")) {
        cap.open(source_.toStdString());
    } else if (!source_.isEmpty()) {
        bool ok = false;
        int devId = source_.toInt(&ok);
        cap.open(ok ? devId : 0, cv::CAP_DSHOW);
    } else {
        cap.open(cameraId_, cv::CAP_DSHOW);
    }

    if (!cap.isOpened()) {
        emit errorOccurred(cameraId_, QString::fromUtf8("无法打开摄像头, 请检查设备连接"));
        emit finished(cameraId_);
        return;
    }
    running_ = true;
    const int frameDelayMs = 33;
    while (running_) {
        cv::Mat frame;
        cap.set(cv::CAP_PROP_BUFFERSIZE, 1);
        if (!cap.read(frame)) {
            if (running_) {
                emit errorOccurred(cameraId_, QString::fromUtf8("摄像头读取失败，设备可能已断开"));
            }
            break;
        }
        auto result = processOneFrame(frame, confThresh, nmsThresh);
        emit frameProcessed(cameraId_, result.image, result.detections, 0);
        QThread::msleep(frameDelayMs);
    }
    cap.release();
    emit finished(cameraId_);
}

void InferenceWorker::processSource(float confThresh, float nmsThresh) {
    if (!source_.isEmpty() && source_.startsWith("rtsp://")) {
        cv::VideoCapture cap(source_.toStdString());
        if (!cap.isOpened()) {
            emit errorOccurred(cameraId_, QString::fromUtf8("无法打开RTSP流: ") + source_);
            emit finished(cameraId_);
            return;
        }
        running_ = true;
        while (running_) {
            cv::Mat frame;
            if (!cap.read(frame)) break;
            auto result = processOneFrame(frame, confThresh, nmsThresh);
            emit frameProcessed(cameraId_, result.image, result.detections, 0);
            QThread::msleep(10);
        }
        cap.release();
        emit finished(cameraId_);
    } else {
        processCamera(confThresh, nmsThresh);
    }
}

void InferenceWorker::stop() {
    running_ = false;
}

InferenceWorker::FrameResult InferenceWorker::processOneFrame(
    const cv::Mat& frame, float confThresh, float nmsThresh)
{
    cv::Mat processed = Preprocessor::letterbox(frame);
    std::vector<float> tensor = Preprocessor::imageToTensor(processed);

    std::vector<Detection> detections;

    if (useBatchInference_ && Config::BATCH_SIZE > 1) {
        batchTensors_.push_back(std::move(tensor));
        batchImgSizes_.emplace_back(frame.cols, frame.rows);
        batchCounter_++;

        if (batchCounter_ >= Config::BATCH_SIZE) {
            std::vector<std::vector<Detection>> batchDetections;
            engine_->batchInfer(batchTensors_, batchDetections, batchImgSizes_,
                               confThresh, nmsThresh);
            if (!batchDetections.empty()) {
                detections = std::move(batchDetections.back());
            }
            batchTensors_.clear();
            batchImgSizes_.clear();
            batchCounter_ = 0;
        } else {
            // 未满batch返回空检测
            FrameResult result;
            auto displayImg = std::make_shared<cv::Mat>(frame.clone());
            Postprocessor::drawDetections(*displayImg, detections);
            cv::cvtColor(*displayImg, *displayImg, cv::COLOR_BGR2RGB);
            QImage qimg(displayImg->data, displayImg->cols, displayImg->rows,
                        displayImg->step, QImage::Format_RGB888);
            result.image = qimg.copy();
            result.detections = std::move(detections);
            return result;
        }
    } else {
        engine_->infer(tensor, detections, frame.cols, frame.rows, confThresh, nmsThresh);
    }

    auto displayImg = std::make_shared<cv::Mat>(frame.clone());
    Postprocessor::drawDetections(*displayImg, detections);
    cv::cvtColor(*displayImg, *displayImg, cv::COLOR_BGR2RGB);
    QImage qimg(displayImg->data, displayImg->cols, displayImg->rows,
                displayImg->step, QImage::Format_RGB888);

    // 环形缓冲区(共享指针, 零拷贝)
    {
        std::lock_guard<std::mutex> lock(bufferMutex_);
        frameBuffer_.push_back(displayImg);
        if (frameBuffer_.size() > Config::RING_BUFFER_FRAMES) {
            frameBuffer_.pop_front();
        }
    }

    checkAlert(detections, displayImg);

    // 告警录制后续帧
    if (alertRecording_ && alertRemainingFrames_ > 0) {
        alertBuffer_.push_back(displayImg);
        alertRemainingFrames_--;
        if (alertRemainingFrames_ == 0) {
            saveAlertFiles(QUuid::createUuid().toString(QUuid::WithoutBraces),
                          pendingAlarmType_);
            alertRecording_ = false;
            alertBuffer_.clear();
            pendingAlarmType_.clear();
        }
    }

    FrameResult result;
    result.image = qimg.copy();
    result.detections = std::move(detections);
    return result;
}

void InferenceWorker::checkAlert(
    const std::vector<Detection>& detections, const std::shared_ptr<cv::Mat>& annotatedFrame)
{
    for (const auto& det : detections) {
        const auto& name = Config::CLASS_NAMES[det.class_id];
        if (name.find("no_") != 0) continue;

        auto now = std::chrono::steady_clock::now();
        auto it = lastAlertTime_.find(det.class_id);
        if (it != lastAlertTime_.end()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - it->second).count();
            if (elapsed < Config::ALERT_COOLDOWN_MS) continue;
        }
        lastAlertTime_[det.class_id] = now;

        alertRecording_ = true;
        alertRemainingFrames_ = Config::ALERT_AFTER_FRAMES;
        pendingAlarmType_ = QString::fromStdString(name);

        {
            std::lock_guard<std::mutex> lock(bufferMutex_);
            alertBuffer_ = frameBuffer_;
        }
        alertBuffer_.push_back(annotatedFrame);

        return;
    }
}

void InferenceWorker::saveAlertFiles(const QString& alarmId, const QString& alarmType) {
    if (alertBuffer_.empty()) return;

    auto toBgr = [](const std::shared_ptr<cv::Mat>& rgbFrame) -> cv::Mat {
        cv::Mat bgr;
        cv::cvtColor(*rgbFrame, bgr, cv::COLOR_RGB2BGR);
        return bgr;
    };

    int frameW = alertBuffer_.back()->cols;
    int frameH = alertBuffer_.back()->rows;
    if (frameW <= 0 || frameH <= 0) return;

    QString baseName = QString("alarm_%1_%2").arg(alarmId).arg(alarmType);
    QString videoPath = outputDir_ + "/" + baseName + ".mp4";
    QString imagePath = outputDir_ + "/" + baseName + ".jpg";

    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    cv::VideoWriter writer(videoPath.toStdString(), fourcc, 30.0,
                           cv::Size(frameW, frameH));
    if (writer.isOpened()) {
        for (auto& f : alertBuffer_) {
            writer.write(toBgr(f));
        }
        writer.release();
    }

    cv::imwrite(imagePath.toStdString(), toBgr(alertBuffer_.front()));

    // 构造告警 JSON
    QString hostIp = QString::fromStdString(Config::HOST_IP);
    QJsonObject data;
    data["alarm_id"]   = alarmId;
    data["alarm_type"] = alarmType;
    data["timestamp"]  = QDateTime::currentDateTime().toMSecsSinceEpoch();
    data["video_url"]  = QString("http://%1:%2/%3").arg(
        hostIp).arg(Config::HTTP_PORT).arg(baseName + ".mp4");
    data["image_url"]  = QString("http://%1:%2/%3").arg(
        hostIp).arg(Config::HTTP_PORT).arg(baseName + ".jpg");

    QJsonObject root;
    root["type"] = "alarm";
    root["data"] = data;

    QString alertJson = QString::fromUtf8(
        QJsonDocument(root).toJson(QJsonDocument::Compact));

    emit alertSaved(cameraId_, videoPath, imagePath, alertJson);
}