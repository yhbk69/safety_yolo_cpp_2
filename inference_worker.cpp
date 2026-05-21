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
#include <QDebug>
#include <cstdint>

InferenceWorker::InferenceWorker(IEngine* engine, int cameraId,
                                     const QString& cameraName)
    : engine_(engine)
    , cameraId_(cameraId)
    , cameraName_(cameraName)
{
    outputDir_ = QDir::cleanPath(QDir::currentPath() + "/" +
        QString::fromStdString(Config::OUTPUT_DIR));
    QDir().mkpath(outputDir_);
}

void InferenceWorker::process(std::unique_ptr<IVideoSource> source,
                               float confThresh, float nmsThresh) {
    if (!source) {
        emit finished(cameraId_);
        return;
    }

    running_ = true;
    const int delayMs = source->frameDelayMs();
    cv::Mat frame;
    bool firstFrame = true;

    int64_t frameCount = 0;
    while (running_) {
        if (!source->readFrame(frame)) {
            if (firstFrame) {
                emit errorOccurred(cameraId_,
                    QString::fromUtf8("无法打开视频源: ") + source->name());
            }
            break;
        }
        firstFrame = false;

        frameCount++;
        auto t0 = std::chrono::steady_clock::now();
        auto result = processOneFrame(frame, confThresh, nmsThresh);

        // 未满 batch 时跳过后续处理(避免 emit 空图像)
        if (result.image.isNull()) {
            continue;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - t0).count();
        emit frameProcessed(cameraId_, result.image, result.detections, elapsed);

        if (delayMs > 0) {
            QThread::msleep(delayMs);
        } else if (source->isLive()) {
            // 实时源: 丢弃积压帧, 始终处理最新帧, 降低延迟
            cv::Mat skip;
            int drained = 0;
            while (running_ && source->readFrame(skip) && drained < 5) {
                drained++;
            }
        }
    }

    qDebug() << "[TRACE] InferenceWorker::process" << cameraId_ << "- loop exited, emitting finished, frames:" << frameCount;
    emit finished(cameraId_);
}

void InferenceWorker::stop() {
    qDebug() << "[TRACE] InferenceWorker::stop" << cameraId_ << "- setting running_ = false";
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
            // 未满batch, 等待下一帧凑齐
            return FrameResult{};
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
    if (alertRecording_) {
        bool shouldSave = false;
        {
            std::lock_guard<std::mutex> lock(alertRecordMutex_);
            if (alertRemainingFrames_ > 0) {
                alertBuffer_.push_back(displayImg);
                alertRemainingFrames_--;
                if (alertRemainingFrames_ == 0) {
                    shouldSave = true;
                }
            }
        }
        if (shouldSave) {
            saveAlertFiles(QUuid::createUuid().toString(QUuid::WithoutBraces),
                          pendingAlarmType_);
            alertRecording_ = false;
            alertBuffer_.clear();
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
        {
            std::lock_guard<std::mutex> lock(alertCooldownMutex_);
            auto it = lastAlertTime_.find(det.class_id);
            if (it != lastAlertTime_.end()) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - it->second).count();
                if (elapsed < Config::ALERT_COOLDOWN_MS) {
                    // 冷却中, 跳过
                    continue;
                }
            }
            lastAlertTime_[det.class_id] = now;
        }

        // 触发告警, 开始录制
        alertRecording_ = true;
        {
            std::lock_guard<std::mutex> lock(alertRecordMutex_);
            alertRemainingFrames_ = Config::ALERT_AFTER_FRAMES;
            pendingAlarmType_ = QString::fromStdString(name);
            triggerFrame_ = annotatedFrame;  // 保存触发告警的那一帧
        }

        qDebug() << "[InferenceWorker] 触发告警! camera=" << cameraName_
                 << ", type=" << QString::fromStdString(name) << ", 开始录制" << Config::ALERT_AFTER_FRAMES << "帧";

        {
            std::lock_guard<std::mutex> lock(bufferMutex_);
            alertBuffer_ = frameBuffer_;
        }
        alertBuffer_.push_back(annotatedFrame);

        return;
    }
}

void InferenceWorker::saveAlertFiles(const QString& alarmId, const QString& alarmType) {
    qDebug() << "[InferenceWorker] saveAlertFiles 被调用! alarmId=" << alarmId 
             << ", type=" << alarmType << ", alertBuffer_.size()=" << alertBuffer_.size()
             << ", outputDir=" << outputDir_;
    
    if (alertBuffer_.empty()) {
        qWarning() << "[InferenceWorker] alertBuffer_ 为空, 跳过保存";
        return;
    }

    auto toBgr = [](const std::shared_ptr<cv::Mat>& rgbFrame) -> cv::Mat {
        cv::Mat bgr;
        cv::cvtColor(*rgbFrame, bgr, cv::COLOR_RGB2BGR);
        return bgr;
    };

    int frameW = alertBuffer_.back()->cols;
    int frameH = alertBuffer_.back()->rows;
    if (frameW <= 0 || frameH <= 0) {
        qWarning() << "[InferenceWorker] 无效帧尺寸, 跳过保存";
        return;
    }

    QString baseName = QString("alarm_%1_%2").arg(alarmId).arg(alarmType);
    QString videoPath = outputDir_ + "/" + baseName + ".mp4";
    QString imagePath = outputDir_ + "/" + baseName + ".jpg";

    qDebug() << "[InferenceWorker] 保存路径 video=" << videoPath << ", image=" << imagePath;

    int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    cv::VideoWriter writer(videoPath.toStdString(), fourcc, 30.0,
                           cv::Size(frameW, frameH));
    bool videoOk = false;
    if (writer.isOpened()) {
        for (auto& f : alertBuffer_) {
            writer.write(toBgr(f));
        }
        writer.release();
        // 验证文件是否确实存在且非空
        QFileInfo fi(videoPath);
        videoOk = fi.exists() && fi.size() > 0;
        if (videoOk) {
            qDebug() << "[InferenceWorker] 视频已写入:" << videoPath << "(" << fi.size() << "bytes)";
        } else {
            qWarning() << "[InferenceWorker] 视频写入失败或文件为空:" << videoPath;
        }
    } else {
        qWarning() << "[InferenceWorker] 无法打开视频写入器:" << videoPath;
    }

    // 保存触发告警的那一帧作为截图
    if (triggerFrame_) {
        bool ok = cv::imwrite(imagePath.toStdString(), toBgr(triggerFrame_));
        qDebug() << "[InferenceWorker] 截图保存" << (ok ? "成功" : "失败") << ":" << imagePath;
    } else if (!alertBuffer_.empty()) {
        qWarning() << "[InferenceWorker] triggerFrame_ 为空, 回退使用 alertBuffer_.back()";
        cv::imwrite(imagePath.toStdString(), toBgr(alertBuffer_.back()));
    }

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

    // 只在视频文件验证成功后才发送告警信号
    if (!videoOk) {
        qWarning() << "[InferenceWorker] 视频写入失败, 跳过告警推送";
        triggerFrame_.reset();
        return;
    }

    qDebug() << "[InferenceWorker] 告警JSON:" << alertJson;
    qDebug() << "[InferenceWorker] 发送 alertSaved 信号...";

    emit alertSaved(cameraId_, videoPath, imagePath, alertJson);

    // 清理触发帧, 准备下一次告警
    triggerFrame_.reset();
}