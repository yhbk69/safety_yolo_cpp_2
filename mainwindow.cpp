/**
 * @file mainwindow.cpp
 * @brief YOLO11 TensorRT 推理系统 - 主窗口实现
 */

#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "gui_logger.hpp"
#include "settings_dialog.hpp"

#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QCloseEvent>
#include <QScreen>
#include <QPixmap>
#include <QListWidgetItem>
#include <QDir>
#include <QUuid>
#include <QFile>
#include <QInputDialog>
#include <QBuffer>
#include <QDialog>
#include <QDialogButtonBox>

#include <filesystem>

namespace fs = std::filesystem;



// ============================================================
// MainWindow
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isProcessing_(false)
    , confThreshold_(Config::CONF_THRESHOLD)
    , nmsThreshold_(Config::IOU_THRESHOLD)
{
    ui = new Ui::MainWindow;
    ui->setupUi(this);

    statusMessageLabel_ = new QLabel("就绪", this);
    fpsLabel_  = new QLabel("FPS: --", this);
    timeLabel_ = new QLabel("耗时: --", this);
    wsAddressLabel_ = new QLabel("WebSocket: 未启动", this);
    fpsLabel_->setStyleSheet("margin-right: 15px;");
    timeLabel_->setStyleSheet("margin-right: 15px;");
    wsAddressLabel_->setStyleSheet("margin-right: 15px; color: #337ab7; font-weight: bold;");
    statusBar()->addWidget(statusMessageLabel_, 1);
    statusBar()->addPermanentWidget(wsAddressLabel_);
    statusBar()->addPermanentWidget(fpsLabel_);
    statusBar()->addPermanentWidget(timeLabel_);

    setupConnections();
    statusMessageLabel_->setText("就绪 - 请加载模型后开始检测");
    ui->modelPathEdit->setText(QString::fromStdString(Config::MODEL_PATH));

    QDir().mkpath(QString::fromStdString(Config::OUTPUT_DIR));

    // 加载运行时配置(JSON)
    loadRuntimeConfig();


    // 初始化 MJPEG 推流服务
    mjpegStreamer_ = std::make_unique<MjpegStreamer>();
    mjpegStreamer_->setLogCallback([this](const QString& cat, const QString& msg) {
        GuiLogger::log(ui->logTextEdit, cat, msg);
    });
    auto& cfg = RuntimeConfig::instance();
    mjpegStreamer_->start((quint16)cfg.streamPort(), QString::fromStdString(Config::HOST_IP));

    // 初始化 WebSocket 管理器
    wsManager_ = std::make_unique<WebSocketManager>();
    wsManager_->setLogCallback([this](const QString& cat, const QString& msg) {
        GuiLogger::log(ui->logTextEdit, cat, msg);
    });
    wsManager_->setGetStreamsCallback([this]() -> QList<StreamInfo> {
        QList<StreamInfo> streams;
        for (int id : cameraManager_->cameraIds()) {
            StreamInfo info;
            info.streamId = QString::number(id);
            auto* w = cameraManager_->worker(id);
            info.name = w ? w->cameraName() : QString("摄像头%1").arg(id);
            info.url = QString("http://%1:%2/stream")
                .arg(QString::fromStdString(Config::HOST_IP))
                .arg(RuntimeConfig::instance().streamPort());
            streams.append(info);
        }
        return streams;
    });
    wsManager_->setViewStreamCallback([this](const QString& streamId) -> QString {
        int camId = streamId.toInt();
        if (!cameraManager_->contains(camId)) return QString();
        return QString("http://%1:%2/stream")
            .arg(QString::fromStdString(Config::HOST_IP))
            .arg(RuntimeConfig::instance().streamPort());
    });
    wsManager_->start();
    wsAddressLabel_->setText(QString("WebSocket: ws://%1:%2")
        .arg(QString::fromStdString(Config::HOST_IP))
        .arg(Config::WEBSOCKET_PORT));

    // 初始化 VideoRecorder
    videoRecorder_ = std::make_unique<VideoRecorder>();
    videoRecorder_->setCallbacks({
        .log = [this](const QString& cat, const QString& msg) {
            GuiLogger::log(ui->logTextEdit, cat, msg);
        },
        .updateButtons = [this](bool isRecording) {
            ui->startRecordBtn->setEnabled(!isRecording);
            ui->stopRecordBtn->setEnabled(isRecording);
        }
    });

    // 初始化 ModelManager
    modelManager_ = std::make_unique<ModelManager>();
    modelManager_->setCallbacks({
        .log = [this](const QString& cat, const QString& msg) {
            GuiLogger::log(ui->logTextEdit, cat, msg);
        },
        .onError = [this](const QString& msg) {
            GuiLogger::log(ui->logTextEdit, "模型", "加载失败: " + msg);
        }
    });

    // 初始化 CameraManager
    cameraManager_ = std::make_unique<CameraManager>();
    cameraManager_->setCallbacks({
        .log = [this](const QString& cat, const QString& msg) {
            GuiLogger::log(ui->logTextEdit, cat, msg);
        }
    });

    // 初始化 InferenceManager
    inferenceManager_ = std::make_unique<InferenceManager>();
    inferenceManager_->setCallbacks({
        .log = [this](const QString& cat, const QString& msg) {
            GuiLogger::log(ui->logTextEdit, cat, msg);
        }
    });

    if (fs::exists(Config::MODEL_PATH)) onLoadModel();

    ui->batchInferenceCheck->setChecked(Config::USE_BATCH_INFERENCE);

    GuiLogger::log(ui->logTextEdit, "系统", "YOLO11 PPE 检测系统已启动");
    GuiLogger::log(ui->logTextEdit, "配置", QString("模型路径: %1").arg(QString::fromStdString(Config::MODEL_PATH)));
}

MainWindow::~MainWindow() {
    stopAllCameras();
    if (isProcessing_) {
        isProcessing_ = false;
    }
    httpFileServer_.reset();
    delete ui;
}

void MainWindow::setupConnections() {
    connect(ui->openImageBtn,  &QPushButton::clicked, this, &MainWindow::onOpenImage);
    connect(ui->openVideoBtn,  &QPushButton::clicked, this, &MainWindow::onOpenVideo);
    connect(ui->cameraBtn,     &QPushButton::toggled, this, &MainWindow::onOpenCamera);
    connect(ui->addCameraBtn,  &QPushButton::clicked, this, &MainWindow::onAddCamera);
    connect(ui->settingsBtn,    &QPushButton::clicked, this, &MainWindow::onSettings);
    connect(ui->folderBtn,     &QPushButton::clicked, this, &MainWindow::onOpenFolder);
    connect(ui->stopBtn,       &QPushButton::clicked, this, &MainWindow::onStopProcessing);
    connect(ui->browseModelBtn, &QPushButton::clicked, this, &MainWindow::onBrowseModel);
    connect(ui->loadModelBtn,   &QPushButton::clicked, this, &MainWindow::onLoadModel);
    connect(ui->reloadModelBtn, &QPushButton::clicked, this, &MainWindow::onReloadModel);
    connect(ui->confSlider, &QSlider::valueChanged, this, &MainWindow::onConfThresholdChanged);
    connect(ui->nmsSlider,  &QSlider::valueChanged, this, &MainWindow::onNmsThresholdChanged);
    connect(ui->batchInferenceCheck, &QCheckBox::toggled, this, &MainWindow::onBatchInferenceToggled);
    connect(ui->clearLogBtn, &QPushButton::clicked, this, [this]() {
        ui->logTextEdit->clear();
        GuiLogger::log(ui->logTextEdit, "系统", "日志已清空");
    });
    // 录制按钮
    connect(ui->startRecordBtn, &QPushButton::clicked, this, &MainWindow::onStartRecording);
    connect(ui->stopRecordBtn, &QPushButton::clicked, this, &MainWindow::onStopRecording);
    connect(ui->viewRecordBtn, &QPushButton::clicked, this, &MainWindow::onViewRecordings);
    connect(ui->clearRecordBtn, &QPushButton::clicked, this, &MainWindow::onClearOldRecordings);
    connect(ui->actionOpenImage, &QAction::triggered, this, &MainWindow::onOpenImage);
    connect(ui->actionOpenVideo, &QAction::triggered, this, &MainWindow::onOpenVideo);
    connect(ui->actionOpenCamera, &QAction::triggered, this, [this]() {
        ui->cameraBtn->toggle();
    });
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionLoadModel, &QAction::triggered, this, &MainWindow::onLoadModel);
}

// ============================================================
// WebSocket 服务器





// ============================================================
// WebSocket 消息处理 - 同步请求
// ============================================================

// ============================================================
// WebSocket 消息处理 - 获取摄像头列表
// ============================================================

// ============================================================
// WebSocket 消息处理 - 设置围栏
// ============================================================

// ============================================================
// WebSocket 消息处理 - 查看实时视频流
// ============================================================

// ============================================================
// 告警处理
// ============================================================
void MainWindow::onAlertSaved(int cameraId, const QString& videoPath, const QString& imagePath,
                               const QString& alertJson) {
    // 解析 alarm_id
    QJsonDocument doc = QJsonDocument::fromJson(alertJson.toUtf8());
    QString alarmId = doc.object()["data"].toObject()["alarm_id"].toString();
    if (alarmId.isEmpty()) return;

    // 使用 WebSocketManager 推送告警(含 ACK 重试)
    wsManager_->pushAlarm(alarmId, alertJson);

    // 告警重试已由 wsManager_ 内部管理

    // 状态栏
    QString alarmType = doc.object()["data"].toObject()["alarm_type"].toString();
    statusMessageLabel_->setText(
        QString("[告警] %1 - 视频已保存, 等待确认").arg(alarmType));
    GuiLogger::log(ui->logTextEdit, "告警", QString("发送告警: %1, ID: %2").arg(alarmType, alarmId.left(8)));
    qDebug() << "Alarm sent:" << alertJson;
}

// ============================================================
// 模型管理
// ============================================================
void MainWindow::onBrowseModel() {
    QString filePath = QFileDialog::getOpenFileName(
        this, "选择YOLO11模型文件",
        QString::fromStdString(Config::MODEL_PATH),
        "TensorRT Engine文件 (*.engine);;所有文件 (*)");
    if (!filePath.isEmpty()) ui->modelPathEdit->setText(filePath);
}

void MainWindow::onLoadModel() {
    QString modelPath = ui->modelPathEdit->text().trimmed();
    if (modelPath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择模型文件路径");
        return;
    }
    if (!fs::exists(modelPath.toStdString())) {
        QMessageBox::warning(this, "错误", "模型文件不存在:\n" + modelPath);
        return;
    }

    ui->loadModelBtn->setEnabled(false);
    ui->modelStatusLabel->setText("加载中...");
    ui->modelStatusLabel->setStyleSheet("color: orange; font-weight: bold;");
    statusMessageLabel_->setText("正在加载模型, 请稍候...");
    QApplication::processEvents();

    if (modelManager_->load(modelPath.toStdString())) {
        ui->modelStatusLabel->setText("✓ 已加载");
        ui->modelStatusLabel->setStyleSheet("color: green; font-weight: bold;");
        statusMessageLabel_->setText("模型加载成功, 可以开始检测");
        GuiLogger::log(ui->logTextEdit, "模型", QString("模型加载成功: %1").arg(modelPath));
        enableControls(true);
        ui->reloadModelBtn->setEnabled(true);
    } else {
        ui->modelStatusLabel->setText("✗ 加载失败");
        ui->modelStatusLabel->setStyleSheet("color: red; font-weight: bold;");
        statusMessageLabel_->setText("模型加载失败");
        enableControls(false);
    }
    ui->loadModelBtn->setEnabled(true);
}

void MainWindow::onReloadModel() {
    QString modelPath = ui->modelPathEdit->text().trimmed();
    if (modelPath.isEmpty() || !modelManager_->isLoaded()) {
        QMessageBox::warning(this, "警告", "当前没有已加载的模型");
        return;
    }
    if (!fs::exists(modelPath.toStdString())) {
        QMessageBox::warning(this, "警告", "模型文件不存在:\n" + modelPath);
        return;
    }
    GuiLogger::log(ui->logTextEdit, "模型", QString("正在热切换模型: %1").arg(modelPath));
    statusMessageLabel_->setText("正在热切换模型...");
    ui->reloadModelBtn->setEnabled(false);
    stopAllCameras();
    QApplication::processEvents();

    if (modelManager_->reload(modelPath.toStdString())) {
        ui->modelStatusLabel->setText("✓ 已重载");
        ui->modelStatusLabel->setStyleSheet("color: green; font-weight: bold;");
        statusMessageLabel_->setText("模型热切换成功");
        GuiLogger::log(ui->logTextEdit, "模型", QString("模型热切换成功: %1").arg(modelPath));
    } else {
        ui->modelStatusLabel->setText("✗ 重载失败");
        ui->modelStatusLabel->setStyleSheet("color: red; font-weight: bold;");
        statusMessageLabel_->setText("模型热切换失败");
        GuiLogger::log(ui->logTextEdit, "错误", "模型热切换失败");
    }
    ui->reloadModelBtn->setEnabled(true);
}

// ============================================================
// 推理模式
// ============================================================
void MainWindow::onOpenImage() {
    if (!modelManager_->isLoaded()) { QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请先加载模型")); return; }
    stopAllCameras();
    QString filePath = QFileDialog::getOpenFileName(
        this, "选择图片", "", "图片文件 (*.jpg *.jpeg *.png *.bmp);;所有文件 (*)");
    if (filePath.isEmpty()) return;
    GuiLogger::log(ui->logTextEdit, "检测", QString("打开图片: %1").arg(filePath));
    statusMessageLabel_->setText("正在推理...");
    QApplication::processEvents();
    processSingleImage(filePath.toStdString());
}

void MainWindow::processSingleImage(const std::string& path) {
    cv::Mat img = cv::imread(path);
    if (img.empty()) {
        QMessageBox::warning(this, "错误", "无法读取图像");
        statusMessageLabel_->setText("图像读取失败");
        return;
    }

    auto result = inferenceManager_->processImage(
        modelManager_->engine(), img, confThreshold_, nmsThreshold_);

    if (!result.success) {
        QMessageBox::critical(this, "推理错误", result.errorMsg);
        statusMessageLabel_->setText("推理出错");
        return;
    }

    cv::Mat rgb;
    cv::cvtColor(result.annotated, rgb, cv::COLOR_BGR2RGB);
    QImage qimg(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    updateDisplay(qimg.copy());
    updateDetectionList(result.detections, result.elapsedMs);
    statusMessageLabel_->setText(
        QString("推理完成 - 检测到 %1 个目标").arg(result.detections.size()));
}

void MainWindow::onOpenVideo() {
    if (!modelManager_->isLoaded()) { QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请先加载模型")); return; }
    stopAllCameras();
    if (isProcessing_) onStopProcessing();

    QString filePath = QFileDialog::getOpenFileName(
        this, QString::fromUtf8("选择视频文件"), "", QString::fromUtf8("视频文件 (*.mp4 *.avi *.mov *.mkv);;所有文件 (*)"));
    if (filePath.isEmpty()) return;

    GuiLogger::log(ui->logTextEdit, "检测", QString("打开视频: %1").arg(filePath));
    statusMessageLabel_->setText(QString::fromUtf8("正在处理视频..."));
    enableControls(false);
    ui->cameraBtn->setChecked(false);
    ui->stopBtn->setEnabled(true);
    isProcessing_ = true;

    auto* thread = new QThread(this);
    auto* worker = new InferenceWorker(modelManager_->engine(), -1, "video", filePath);
    worker->moveToThread(thread);

    connect(worker, &InferenceWorker::frameProcessed, this, &MainWindow::onFrameProcessed);
    connect(worker, &InferenceWorker::alertSaved, this, &MainWindow::onAlertSaved);
    connect(worker, &InferenceWorker::finished, this, [this](int cameraId) {
        // 视频模式结束
        isProcessing_ = false;
        enableControls(true);
        ui->stopBtn->setEnabled(false);
        statusMessageLabel_->setText(QString::fromUtf8("视频处理完成"));
        fpsLabel_->setText("FPS: --");
        GuiLogger::log(ui->logTextEdit, "系统", "视频处理完成");
        // 视频worker不在CameraManager中, 手动清理
        sender()->deleteLater();
    });
    connect(worker, &InferenceWorker::errorOccurred, this, [this](int, const QString& msg) {
        QMessageBox::critical(this, QString::fromUtf8("处理错误"), msg);
    });
    connect(thread, &QThread::started, worker, [this, worker, filePath]() {
        worker->setBatchInference(ui->batchInferenceCheck->isChecked());
        worker->processVideo(filePath, confThreshold_, nmsThreshold_);
    });
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);

    activeDisplayCamera_ = -1;
    thread->start();
}

void MainWindow::onOpenCamera(bool checked) {
    if (checked) {
        if (!modelManager_->isLoaded()) {
            QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请先加载模型"));
            ui->cameraBtn->setChecked(false);
            return;
        }
        // 如果默认摄像头已在运行, 先停止
        if (cameraManager_->contains(0)) stopCamera(0);

        GuiLogger::log(ui->logTextEdit, "检测", "启动默认摄像头");
        statusMessageLabel_->setText(QString::fromUtf8("正在启动摄像头..."));
        ui->cameraBtn->setText(QString::fromUtf8("关闭摄像头"));
        ui->cameraStatusLabel->setStyleSheet("font-size: 20px; font-weight: bold; padding: 0 12px; color: green;");
        ui->cameraStatusLabel->setText(QString::fromUtf8("● 已开启"));
        activeDisplayCamera_ = 0;

        auto* thread = new QThread(this);
        auto* worker = new InferenceWorker(modelManager_->engine(), 0, "camera_0");
        worker->moveToThread(thread);

        connect(worker, &InferenceWorker::frameProcessed, this, &MainWindow::onFrameProcessed);
        connect(worker, &InferenceWorker::alertSaved, this, &MainWindow::onAlertSaved);
        connect(worker, &InferenceWorker::finished, this, &MainWindow::onWorkerFinished);
        connect(worker, &InferenceWorker::errorOccurred, this, &MainWindow::onWorkerError);
        connect(thread, &QThread::started, worker, [this, worker]() {
            worker->setBatchInference(ui->batchInferenceCheck->isChecked());
            worker->processCamera(confThreshold_, nmsThreshold_);
        });
        connect(thread, &QThread::finished, worker, &QObject::deleteLater);

        cameraManager_->add(0, thread, worker);
        thread->start();

        // 自动开始录制
        QTimer::singleShot(500, this, [this]() {
            onStartRecording();
        });
    } else {
        // 停止录制（如果正在录制）
        if (videoRecorder_->isRecording(0)) {
            videoRecorder_->stopRecording(0);
        }
        stopCamera(0);
    }
}

void MainWindow::onAddCamera() {
    if (!modelManager_->isLoaded()) {
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请先加载模型"));
        return;
    }

    // 弹出对话框: 输入摄像头设备ID或RTSP地址
    bool ok = false;
    QString source = QInputDialog::getText(this,
        QString::fromUtf8("添加摄像头"),
        QString::fromUtf8("输入摄像头设备ID(0,1,2...)或RTSP地址:"),
        QLineEdit::Normal, "", &ok);
    if (!ok || source.isEmpty()) return;

    int camId = cameraManager_->allocateId();
    QString camName = QString("camera_%1").arg(camId);

    GuiLogger::log(ui->logTextEdit, "检测", QString("添加摄像头 %1: %2").arg(camId).arg(source));
    activeDisplayCamera_ = camId;

    auto* thread = new QThread(this);
    auto* worker = new InferenceWorker(modelManager_->engine(), camId, camName, source);
    worker->moveToThread(thread);

    connect(worker, &InferenceWorker::frameProcessed, this, &MainWindow::onFrameProcessed);
    connect(worker, &InferenceWorker::alertSaved, this, &MainWindow::onAlertSaved);
    connect(worker, &InferenceWorker::finished, this, &MainWindow::onWorkerFinished);
    connect(worker, &InferenceWorker::errorOccurred, this, &MainWindow::onWorkerError);
    connect(thread, &QThread::started, worker, [this, worker]() {
        worker->setBatchInference(ui->batchInferenceCheck->isChecked());
        worker->processSource(confThreshold_, nmsThreshold_);
    });
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);

    cameraManager_->add(camId, thread, worker);
    thread->start();

    // 更新摄像头状态显示
    ui->cameraStatusLabel->setStyleSheet("font-size: 20px; font-weight: bold; padding: 0 12px; color: green;");
    ui->cameraStatusLabel->setText(QString::fromUtf8("● %1路运行").arg(cameraManager_->count()));
    ui->stopBtn->setEnabled(true);

    // 自动开始录制
    QTimer::singleShot(500, this, [this, camId]() {
        activeDisplayCamera_ = camId;
        onStartRecording();
    });
}

void MainWindow::onRemoveCamera(int cameraId) {
    stopCamera(cameraId);
}

void MainWindow::stopCamera(int cameraId) {
    if (!cameraManager_->contains(cameraId)) return;

    GuiLogger::log(ui->logTextEdit, "系统", QString("正在停止摄像头 %1...").arg(cameraId));

    // 如果该摄像头正在录制，先停止录制
    if (videoRecorder_->isRecording(cameraId)) {
        GuiLogger::log(ui->logTextEdit, "录像", QString("摄像头%1正在录制，自动停止录制").arg(cameraId));
        activeDisplayCamera_ = cameraId;
        onStopRecording();
    }

    // 委托 CameraManager 清理工作线程
    cameraManager_->stop(cameraId);

    // 更新UI状态
    if (cameraId == 0) {
        ui->cameraBtn->setChecked(false);
        ui->cameraBtn->setText(QString::fromUtf8("开启摄像头"));
    }

    if (cameraManager_->isEmpty()) {
        ui->cameraStatusLabel->setStyleSheet("font-size: 20px; font-weight: bold; padding: 0 12px;");
        ui->cameraStatusLabel->setText(QString::fromUtf8("⏹ 未开启"));
        ui->stopBtn->setEnabled(false);
        fpsLabel_->setText("FPS: --");
    } else {
        ui->cameraStatusLabel->setText(QString::fromUtf8("● %1路运行").arg(cameraManager_->count()));
    }

    GuiLogger::log(ui->logTextEdit, "系统", QString("摄像头 %1 已停止").arg(cameraId));
}

void MainWindow::stopAllCameras() {
    auto ids = cameraManager_->cameraIds();
    for (int id : ids) {
        stopCamera(id);
    }
}

void MainWindow::onOpenFolder() {
    if (!modelManager_->isLoaded()) { QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请先加载模型")); return; }
    stopAllCameras();

    QString dirPath = QFileDialog::getExistingDirectory(
        this, "选择图片文件夹", "", QFileDialog::ShowDirsOnly);
    if (dirPath.isEmpty()) return;

    GuiLogger::log(ui->logTextEdit, "检测", QString("批量处理文件夹: %1").arg(dirPath));

    statusMessageLabel_->setText("正在批量处理...");
    QApplication::processEvents();

    auto folderResult = inferenceManager_->processFolder(
        modelManager_->engine(),
        dirPath.toStdString(),
        confThreshold_, nmsThreshold_,
        Config::OUTPUT_DIR,
        [this](int succ, int total) {
            statusMessageLabel_->setText(
                QString("批量处理: %1/%2").arg(succ).arg(total));
            QApplication::processEvents();
        });

    statusMessageLabel_->setText(
        QString("批量处理完成 - 成功处理 %1/%2 张图片")
            .arg(folderResult.succ).arg(folderResult.total));
    QMessageBox::information(this, "批量处理完成",
        QString("共扫描 %1 张\n成功处理 %2 张\n结果保存至: %3")
            .arg(folderResult.total).arg(folderResult.succ)
            .arg(QString::fromStdString(Config::OUTPUT_DIR)));
}

void MainWindow::onStopProcessing() {
    stopAllCameras();
    isProcessing_ = false;
    enableControls(true);
    ui->stopBtn->setEnabled(false);
    fpsLabel_->setText("FPS: --");
    statusMessageLabel_->setText(QString::fromUtf8("已停止"));
    GuiLogger::log(ui->logTextEdit, "系统", "所有处理已停止");
}

void MainWindow::onFrameProcessed(int cameraId, QImage image, std::vector<Detection> detections, double elapsedMs) {
    // 录像: 写帧到录像文件 (VideoRecorder 内部处理 RGB→BGR 和缩放)
    QImage convImg = image.convertToFormat(QImage::Format_RGB888);
    cv::Mat rgbMat(convImg.height(), convImg.width(), CV_8UC3,
                   const_cast<uchar*>(convImg.constBits()), convImg.bytesPerLine());
    videoRecorder_->writeFrame(cameraId, rgbMat);

    // MJPEG推流: 每帧都推
    QByteArray jpegData;
    {
        QBuffer buf(&jpegData);
        buf.open(QIODevice::WriteOnly);
        image.save(&buf, "JPEG", 60);
    }
    mjpegStreamer_->pushFrame(jpegData);

    // 只更新当前活跃显示的摄像头画面
    if (cameraId == activeDisplayCamera_) {
        updateDisplay(image);
        updateDetectionList(detections, elapsedMs);
    }
    if (elapsedMs > 0 && cameraId == activeDisplayCamera_) {
        double fps = 1000.0 / elapsedMs;
        fpsLabel_->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
    }
    // 每5帧输出一次检测详情日志（只在检测到目标时输出）
    static int logFrameCount = 0;
    if (++logFrameCount % 5 == 0) {
        if (!detections.empty()) {
            // 统计各类别数量
            std::map<int, int> classCounts;
            for (const auto& det : detections) {
                classCounts[det.class_id]++;
            }
            QString detail;
            for (const auto& [cid, cnt] : classCounts) {
                if (!detail.isEmpty()) detail += ", ";
                detail += QString("%1×%2").arg(cnt).arg(QString::fromStdString(Config::CLASS_NAMES[cid]));
            }
            double fps = (elapsedMs > 0) ? 1000.0 / elapsedMs : 0;
            GuiLogger::log(ui->logTextEdit, "检测", QString("%1 | %2 | %3ms, FPS:%4")
                .arg(detections.size()).arg(detail)
                .arg(elapsedMs, 0, 'f', 1).arg(fps, 0, 'f', 1));
        }
    }
}

void MainWindow::onWorkerFinished(int cameraId) {
    if (cameraManager_->contains(cameraId)) {
        // 如果是默认摄像头, 重置UI
        if (cameraId == 0) {
            ui->cameraBtn->setChecked(false);
            ui->cameraBtn->setText(QString::fromUtf8("开启摄像头"));
            ui->cameraStatusLabel->setStyleSheet("font-size: 20px; font-weight: bold; padding: 0 12px;");
            ui->cameraStatusLabel->setText(QString::fromUtf8("⏹ 未开启"));
        }
        cameraManager_->stop(cameraId);
        GuiLogger::log(ui->logTextEdit, "系统", QString("摄像头 %1 处理完成").arg(cameraId));
    }

    // 如果没有任何活跃worker, 重置UI
    if (cameraManager_->isEmpty()) {
        isProcessing_ = false;
        enableControls(true);
        ui->stopBtn->setEnabled(false);
        fpsLabel_->setText("FPS: --");
        statusMessageLabel_->setText("处理完成");
    }
}

void MainWindow::onWorkerError(int cameraId, const QString& message) {
    GuiLogger::log(ui->logTextEdit, "错误", QString("[摄像头%1] %2").arg(cameraId).arg(message));
    if (cameraId == 0) {
        ui->cameraBtn->setChecked(false);
    }
    onWorkerFinished(cameraId);
}

void MainWindow::onConfThresholdChanged(int value) {
    confThreshold_ = value / 100.0f;
    updateThresholdLabels();
}

void MainWindow::onNmsThresholdChanged(int value) {
    nmsThreshold_ = value / 100.0f;
    updateThresholdLabels();
}

void MainWindow::updateThresholdLabels() {
    ui->confValueLabel->setText(QString::number(confThreshold_, 'f', 2));
    ui->nmsValueLabel->setText(QString::number(nmsThreshold_, 'f', 2));
}

void MainWindow::updateDisplay(const QImage& image) {
    QPixmap pixmap = QPixmap::fromImage(image);
    QSize labelSize = ui->displayLabel->size();
    // 实时视频用FastTransformation，避免SmoothTransformation卡死主线程
    pixmap = pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::FastTransformation);
    ui->displayLabel->setPixmap(pixmap);
}

void MainWindow::updateDetectionList(const std::vector<Detection>& detections, double elapsedMs) {
    // 增量更新：只在检测数量变化时才重建列表，避免每帧clear+rebuild
    int newCount = static_cast<int>(detections.size());
    int oldCount = ui->resultListWidget->count();
    bool needRebuild = (newCount != oldCount);

    // 如果数量相同，检查类别是否变化
    if (!needRebuild && newCount > 0) {
        for (int i = 0; i < newCount && i < oldCount; ++i) {
            auto* item = ui->resultListWidget->item(i);
            const auto& name = Config::CLASS_NAMES[detections[i].class_id];
            QString text = QString("%1  %2")
                .arg(QString::fromStdString(name), -12).arg(detections[i].conf, 0, 'f', 3);
            if (item->text() != text) { needRebuild = true; break; }
        }
    }

    if (needRebuild) {
        ui->resultListWidget->clear();
        if (detections.empty()) {
            ui->resultListWidget->addItem("未检测到目标");
        } else {
            std::vector<Detection> sorted = detections;
            std::sort(sorted.begin(), sorted.end(),
                      [](const Detection& a, const Detection& b) { return a.conf > b.conf; });

            for (const auto& det : sorted) {
                const auto& name = Config::CLASS_NAMES[det.class_id];
                QString text = QString("%1  %2")
                    .arg(QString::fromStdString(name), -12).arg(det.conf, 0, 'f', 3);
                auto* item = new QListWidgetItem(text, ui->resultListWidget);
                if (name.find("no_") == 0)
                    item->setForeground(QColor("#d9534f"));
                else if (name == "Person" || name == "none")
                    item->setForeground(QColor("#888888"));
                else
                    item->setForeground(QColor("#5cb85c"));
            }
        }
    } else {
        // 数量相同，只更新置信度数值
        std::vector<Detection> sorted = detections;
        std::sort(sorted.begin(), sorted.end(),
                  [](const Detection& a, const Detection& b) { return a.conf > b.conf; });
        for (int i = 0; i < static_cast<int>(sorted.size()); ++i) {
            auto* item = ui->resultListWidget->item(i);
            const auto& name = Config::CLASS_NAMES[sorted[i].class_id];
            QString text = QString("%1  %2")
                .arg(QString::fromStdString(name), -12).arg(sorted[i].conf, 0, 'f', 3);
            item->setText(text);
            if (name.find("no_") == 0)
                item->setForeground(QColor("#d9534f"));
            else if (name == "Person" || name == "none")
                item->setForeground(QColor("#888888"));
            else
                item->setForeground(QColor("#5cb85c"));
        }
    }

    ui->totalCountLabel->setText(QString("目标总数: %1").arg(detections.size()));
    timeLabel_->setText(QString("耗时: %1ms").arg(elapsedMs, 0, 'f', 1));
}

void MainWindow::enableControls(bool enabled) {
    ui->openImageBtn->setEnabled(enabled);
    ui->openVideoBtn->setEnabled(enabled);
    ui->cameraBtn->setEnabled(enabled);
    ui->folderBtn->setEnabled(enabled);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    stopAllCameras();
    if (isProcessing_) isProcessing_ = false;
    event->accept();
}

void MainWindow::onStartRecording() {
    int cameraId = activeDisplayCamera_;
    if (cameraId < 0) {
        QMessageBox::warning(this, QString::fromUtf8("录像"), QString::fromUtf8("请先打开一个摄像头"));
        return;
    }
    videoRecorder_->startRecording(cameraId);
}

void MainWindow::onStopRecording() {
    int cameraId = activeDisplayCamera_;
    if (!videoRecorder_->isRecording(cameraId)) {
        QMessageBox::warning(this, QString::fromUtf8("录像"), QString::fromUtf8("当前没有正在录制的摄像头"));
        return;
    }
    videoRecorder_->stopRecording(cameraId);
}

void MainWindow::onViewRecordings() {
    videoRecorder_->openRecordDir();
    GuiLogger::log(ui->logTextEdit, QString::fromUtf8("录像"), QString::fromUtf8("打开录像目录"));
}

void MainWindow::onClearOldRecordings() {
    int keepDays = ui->recordDaysSpin->value();
    int deleted = videoRecorder_->cleanOldRecordings(keepDays);
    if (deleted > 0) {
        GuiLogger::log(ui->logTextEdit, QString::fromUtf8("录像"), QString::fromUtf8("已清理 %1 个旧录像目录").arg(deleted));
    }
    QMessageBox::information(this, QString::fromUtf8("清理完成"),
        QString::fromUtf8("已清理 %1 个旧录像目录").arg(deleted));
}

// ============================================================
// 批量推理开关
// ============================================================
void MainWindow::onBatchInferenceToggled(bool checked) {
    if (checked) {
        if (Config::BATCH_SIZE <= 1) {
            ui->batchInferenceCheck->setChecked(false);
            GuiLogger::log(ui->logTextEdit, QString::fromUtf8("配置"), QString::fromUtf8("当前BATCH_SIZE=1, 无法启用批量推理"));
            return;
        }
        cameraManager_->setBatchInference(true);
        statusMessageLabel_->setText(QString::fromUtf8("批量推理已启用 (batch=%1)").arg(Config::BATCH_SIZE));
        GuiLogger::log(ui->logTextEdit, QString::fromUtf8("配置"), QString::fromUtf8("批量推理已启用 (batch=%1)").arg(Config::BATCH_SIZE));
    } else {
        cameraManager_->setBatchInference(false);
        statusMessageLabel_->setText(QString::fromUtf8("批量推理已禁用, 使用单帧推理"));
        GuiLogger::log(ui->logTextEdit, QString::fromUtf8("配置"), QString::fromUtf8("批量推理已禁用"));
    }
}

// ============================================================
// 运行时配置
// ============================================================
void MainWindow::loadRuntimeConfig() {
    auto& cfg = RuntimeConfig::instance();
    QString cfgPath = QDir::currentPath() + "/config.json";
    if (QFile::exists(cfgPath)) {
        if (cfg.loadFromFile(cfgPath)) {
            GuiLogger::log(ui->logTextEdit, QString::fromUtf8("配置"), QString::fromUtf8("已加载运行时配置: %1").arg(cfgPath));
        } else {
            GuiLogger::log(ui->logTextEdit, QString::fromUtf8("配置"), QString::fromUtf8("配置文件解析失败, 使用默认值"));
        }
    } else {
        // 首次运行, 生成默认配置文件
        cfg.saveToFile(cfgPath);
        GuiLogger::log(ui->logTextEdit, QString::fromUtf8("配置"), QString::fromUtf8("已生成默认配置: %1").arg(cfgPath));
    }

    // 用运行时配置覆盖UI初始值
    confThreshold_ = cfg.confThreshold();
    nmsThreshold_  = cfg.iouThreshold();
    ui->modelPathEdit->setText(cfg.modelPath());
    ui->confSlider->setValue(static_cast<int>(confThreshold_ * 100));
    ui->nmsSlider->setValue(static_cast<int>(nmsThreshold_ * 100));
}

void MainWindow::saveRuntimeConfig() {
    auto& cfg = RuntimeConfig::instance();
    cfg.setConfThreshold(confThreshold_);
    cfg.setIouThreshold(nmsThreshold_);
    cfg.setModelPath(ui->modelPathEdit->text());
    QString cfgPath = QDir::currentPath() + "/config.json";
    cfg.saveToFile(cfgPath);
    GuiLogger::log(ui->logTextEdit, QString::fromUtf8("配置"), QString::fromUtf8("配置已保存: %1").arg(cfgPath));
}

void MainWindow::onSettings() {
    auto& cfg = RuntimeConfig::instance();

    SettingsDialog dlg(SettingsResult{
        .confThreshold = confThreshold_,
        .nmsThreshold = nmsThreshold_,
        .websocketPort = cfg.websocketPort(),
        .httpPort = cfg.httpPort(),
        .streamPort = cfg.streamPort(),
        .ackTimeoutMs = cfg.ackTimeoutMs(),
        .alertCooldownMs = cfg.alertCooldownMs(),
        .ringBufferFrames = cfg.ringBufferFrames(),
        .modelPath = cfg.modelPath(),
        .outputDir = cfg.outputDir(),
        .recordDir = cfg.recordDir(),
    }, this);

    if (dlg.exec() == QDialog::Accepted) {
        auto r = dlg.result();
        confThreshold_ = r.confThreshold;
        nmsThreshold_ = r.nmsThreshold;
        ui->confSlider->setValue(static_cast<int>(confThreshold_ * 100));
        ui->nmsSlider->setValue(static_cast<int>(nmsThreshold_ * 100));

        cfg.setConfThreshold(r.confThreshold);
        cfg.setIouThreshold(r.nmsThreshold);
        cfg.setWebsocketPort(r.websocketPort);
        cfg.setHttpPort(r.httpPort);
        cfg.setStreamPort(r.streamPort);
        cfg.setAckTimeoutMs(r.ackTimeoutMs);
        cfg.setAlertCooldownMs(r.alertCooldownMs);
        cfg.setRingBufferFrames(r.ringBufferFrames);
        cfg.setModelPath(r.modelPath);
        cfg.setOutputDir(r.outputDir);
        cfg.setRecordDir(r.recordDir);

        saveRuntimeConfig();

        GuiLogger::log(ui->logTextEdit, QString::fromUtf8("配置"),
            QString::fromUtf8("运行时设置已更新(端口变更需重启生效)"));
    }
}

// ============================================================
// 工具方法
// ============================================================
QString MainWindow::getHostIp() {
    // 优先获取非回环的IPv4地址
    //const auto interfaces = QNetworkInterface::allInterfaces();
    //for (const auto& iface : interfaces) {
    //    if (iface.flags() & QNetworkInterface::IsLoopBack) continue;
    //    if (!(iface.flags() & QNetworkInterface::IsUp)) continue;
    //    if (!(iface.flags() & QNetworkInterface::IsRunning)) continue;
    //    for (const auto& addr : iface.addressEntries()) {
    //        if (addr.ip().protocol() == QAbstractSocket::IPv4Protocol &&
    //            addr.ip() != QHostAddress::LocalHost) {
    //            return addr.ip().toString();
    //        }
    //    }
    //}
    // 回退到配置文件中的IP
    return QString::fromStdString(Config::HOST_IP);
}
