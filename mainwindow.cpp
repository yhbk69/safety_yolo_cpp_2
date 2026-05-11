/**
 * @file mainwindow.cpp
 * @brief YOLO11 TensorRT 推理系统 - 主窗口实现
 */

#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "gui_logger.hpp"
#include "settings_dialog.hpp"
#include "detection_utils.hpp"
#include "video_source.hpp"
#include "mjpeg_streamer.hpp"
#include "websocket_manager.hpp"
#include "video_recorder.hpp"

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
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QDebug>

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

    // 摄像头列表
    cameraListWidget_ = new QListWidget(this);
    cameraListWidget_->setMaximumHeight(150);
    cameraListWidget_->setContextMenuPolicy(Qt::CustomContextMenu);
    cameraListWidget_->setStyleSheet(
        "QListWidget { background-color: #1e1e1e; border: 1px solid #444; border-radius: 4px; }"
        "QListWidget::item { padding: 4px 8px; border-bottom: 1px solid #333; }"
        "QListWidget::item:selected { background-color: #2a3f5f; }");
    {
        auto* rightLayout = qobject_cast<QVBoxLayout*>(ui->rightPanel->layout());
        if (rightLayout) {
            int idx = rightLayout->indexOf(ui->resultTitleLabel);
            rightLayout->insertWidget(idx + 1, cameraListWidget_);
        }
    }
    connect(cameraListWidget_, &QListWidget::itemClicked, this, &MainWindow::onCameraListClicked);
    // "开始检测" 按钮 (插入到 stopBtn 前)
    {
        auto* btnLayout = qobject_cast<QHBoxLayout*>(ui->buttonLayout);
        if (btnLayout) {
            startDetectBtn_ = new QPushButton(QString::fromUtf8("开始检测"), this);
            startDetectBtn_->setStyleSheet(
                "QPushButton { background-color: #5cb85c; color: white; font-weight: bold; padding: 6px 16px; }"
                "QPushButton:disabled { background-color: #555; color: #888; }");
            startDetectBtn_->setEnabled(false);
            // 找到 stopBtn 的位置, 插入到它前面
            int stopIdx = btnLayout->indexOf(ui->stopBtn);
            if (stopIdx >= 0)
                btnLayout->insertWidget(stopIdx, startDetectBtn_);
            else
                btnLayout->addWidget(startDetectBtn_);
            connect(startDetectBtn_, &QPushButton::clicked, this, &MainWindow::onStartDetection);
        }
    }

    connect(cameraListWidget_, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        auto* item = cameraListWidget_->itemAt(pos);
        if (!item) return;
        int camId = item->data(Qt::UserRole).toInt();
        qDebug() << "[DEBUG] context menu for cam" << camId;
        QMenu menu;
        menu.addAction(QString::fromUtf8("[摄像头%1]").arg(camId))->setEnabled(false);
        auto* aliasAction = menu.addAction(QString::fromUtf8("设置别名"));
        connect(aliasAction, &QAction::triggered, this, [this, camId]() {
            QString oldAlias = cameraAliases_.count(camId) ? cameraAliases_[camId] : QString();
            bool ok = false;
            QString label = cameraAliases_.count(camId)
                ? QString::fromUtf8("当前别名: %1\n\n输入新别名:").arg(oldAlias)
                : QString::fromUtf8("为此摄像头设置别名:");
            QString newAlias = QInputDialog::getText(this,
                QString::fromUtf8("设置别名 - 摄像头 %1").arg(camId),
                label, QLineEdit::Normal, oldAlias, &ok);
            if (!ok) return;
            cameraAliases_[camId] = newAlias;
            // 同步到配置文件
            auto& cfg = RuntimeConfig::instance();
            auto cams = cfg.cameras();
            QString src = cameraSources_.count(camId) ? cameraSources_[camId] : QString();
            for (auto& cam : cams) {
                if (cam.source == src) {
                    cam.alias = newAlias;
                    break;
                }
            }
            cfg.setCameras(cams);
            cfg.saveToFile(cfg.configFilePath());
            refreshCameraList();
        });
        auto* removeAction = menu.addAction(QString::fromUtf8("移除摄像头 %1").arg(camId));
        connect(removeAction, &QAction::triggered, this, [this, camId]() {
            qDebug() << "[DEBUG] remove action triggered for cam" << camId;
            onRemoveCamera(camId);
            qDebug() << "[DEBUG] remove action done";
        });
        menu.exec(cameraListWidget_->mapToGlobal(pos));
        qDebug() << "[DEBUG] context menu done";
    });

    statusMessageLabel_->setText("就绪 - 请加载模型后开始检测");
    ui->modelPathEdit->setText(QString::fromStdString(Config::MODEL_PATH));

    QDir().mkpath(QString::fromStdString(Config::OUTPUT_DIR));

    // 加载运行时配置(JSON)
    loadRuntimeConfig();


    // 初始化 MJPEG 推流服务
    {
        auto mjpeg = std::make_unique<MjpegStreamer>();
        mjpeg->setLogCallback(GuiLogger::makeLogCallback(ui->logTextEdit));
        auto& cfg = RuntimeConfig::instance();
        mjpeg->start((quint16)cfg.streamPort(), QString::fromStdString(Config::HOST_IP));
        sinks_.push_back(std::move(mjpeg));
    }

    // 初始化 WebSocket 管理器
    {
        auto ws = std::make_unique<WebSocketManager>();
        ws->setLogCallback(GuiLogger::makeLogCallback(ui->logTextEdit));
        ws->setGetStreamsCallback([this]() -> QList<StreamInfo> {
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
        ws->setViewStreamCallback([this](const QString& streamId) -> QString {
            int camId = streamId.toInt();
            if (!cameraManager_->contains(camId)) return QString();
            return QString("http://%1:%2/stream")
                .arg(QString::fromStdString(Config::HOST_IP))
                .arg(RuntimeConfig::instance().streamPort());
        });
        ws->start();
        ws->setAlarmPushedCallback([this](const QString& alarmType, const QString& alarmId) {
            statusMessageLabel_->setText(
                QString("[告警] %1 - 视频已保存, 等待确认").arg(alarmType));
            GuiLogger::log(ui->logTextEdit, "告警", QString("发送告警: %1, ID: %2").arg(alarmType, alarmId.left(8)));
        });
        sinks_.push_back(std::move(ws));
    }
    wsAddressLabel_->setText(QString("WebSocket: ws://%1:%2")
        .arg(QString::fromStdString(Config::HOST_IP))
        .arg(Config::WEBSOCKET_PORT));

    // 初始化 VideoRecorder
    {
        auto recorder = std::make_unique<VideoRecorder>();
        VideoRecorder::Callbacks vcb = {
            .log = GuiLogger::makeLogCallback(ui->logTextEdit),
            .updateButtons = [this](bool isRecording) {
                ui->startRecordBtn->setEnabled(!isRecording);
                ui->stopRecordBtn->setEnabled(isRecording);
            }
        };
        recorder->setCallbacks(vcb);
        videoRecorder_ = recorder.get();
        sinks_.push_back(std::move(recorder));
    }

    // 初始化 ModelManager
    modelManager_ = std::make_unique<ModelManager>();
    {
        ModelManager::Callbacks mcb = {
            .log = GuiLogger::makeLogCallback(ui->logTextEdit),
            .onError = [this](const QString& msg) {
                GuiLogger::log(ui->logTextEdit, "模型", "加载失败: " + msg);
            }
        };
        modelManager_->setCallbacks(mcb);
    }

    // 初始化 CameraManager
    cameraManager_ = std::make_unique<CameraManager>();
    {
        CameraManager::Callbacks ccb = {
            .log = GuiLogger::makeLogCallback(ui->logTextEdit)
        };
        cameraManager_->setCallbacks(ccb);
    }

    // 初始化 InferenceManager
    inferenceManager_ = std::make_unique<InferenceManager>();
    {
        InferenceManager::Callbacks icb = {
            .log = GuiLogger::makeLogCallback(ui->logTextEdit)
        };
        inferenceManager_->setCallbacks(icb);
    }

    if (fs::exists(Config::MODEL_PATH)) onLoadModel();
    updateModelButtons(false);

    ui->batchInferenceCheck->setChecked(Config::USE_BATCH_INFERENCE);

    GuiLogger::log(ui->logTextEdit, "系统", "YOLO11 PPE 检测系统已启动");
    GuiLogger::log(ui->logTextEdit, "配置", QString("模型路径: %1").arg(QString::fromStdString(Config::MODEL_PATH)));
}

MainWindow::~MainWindow() {
    stopAllCameras();
    if (isProcessing_) {
        isProcessing_ = false;
    }

    // 备份: 确保所有工作线程已退出, 避免 ~QThread 触发 assert
    const auto threads = findChildren<QThread*>();
    for (auto* t : threads) {
        t->quit();
        if (!t->wait(2000)) {
            t->terminate();
            t->wait(1000);
        }
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
// 告警处理
// ============================================================
void MainWindow::onAlertSaved(int cameraId, const QString& videoPath, const QString& imagePath,
                               const QString& alertJson) {
    AlertData ad{cameraId, alertJson};
    for (auto& sink : sinks_) {
        sink->onAlert(ad);
    }
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
    if (isDetecting()) {
        GuiLogger::log(ui->logTextEdit, "系统", "检测进行中, 无法加载模型");
        statusMessageLabel_->setText("请先停止检测后再加载模型");
        return;
    }
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
        updateModelButtons(false);
        autoStartCameras();
        // 模型加载完成: 如果有摄像头在运行则按钮已禁用, 否则启用
        if (startDetectBtn_) startDetectBtn_->setEnabled(cameraManager_->isEmpty());
    } else {
        ui->modelStatusLabel->setText("✗ 加载失败");
        ui->modelStatusLabel->setStyleSheet("color: red; font-weight: bold;");
        statusMessageLabel_->setText("模型加载失败");
        enableControls(false);
    }
    ui->loadModelBtn->setEnabled(true);
}

void MainWindow::onReloadModel() {
    if (isDetecting()) {
        GuiLogger::log(ui->logTextEdit, "系统", "检测进行中, 无法热切换模型");
        statusMessageLabel_->setText("请先停止检测后再切换模型");
        return;
    }
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
    if (startDetectBtn_) startDetectBtn_->setEnabled(false);
    isProcessing_ = true;
    activeDisplayCamera_ = -1;
    updateModelButtons(true);

    startVideoWorker(filePath);
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

        updateModelButtons(true);
        if (startDetectBtn_) startDetectBtn_->setEnabled(false);
        startCameraWorker(0, "camera_0", "");
        refreshCameraList();

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

    // 自定义对话框: 源地址 + 别名 合并到一个界面
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("添加摄像头"));
    dlg.setMinimumWidth(420);
    auto* layout = new QFormLayout(&dlg);
    auto* srcEdit = new QLineEdit(&dlg);
    srcEdit->setPlaceholderText("摄像头设备ID(0,1,2...) 或 RTSP 地址");
    auto* aliasEdit = new QLineEdit(&dlg);
    aliasEdit->setPlaceholderText("可选, 留空则使用源地址");
    auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addRow(QString::fromUtf8("视频源:"), srcEdit);
    layout->addRow(QString::fromUtf8("别名:"), aliasEdit);
    layout->addRow(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    QString source = srcEdit->text().trimmed();
    if (source.isEmpty()) return;

    QString alias = aliasEdit->text().trimmed();
    if (alias.isEmpty()) alias = source;

    int camId = cameraManager_->allocateId();
    QString camName = QString("camera_%1").arg(camId);

    GuiLogger::log(ui->logTextEdit, "检测", QString("添加摄像头 %1: %2").arg(camId).arg(source));
    activeDisplayCamera_ = camId;

    // 保存到记忆列表(带当前阈值和别名)
    camConfThresholds_[camId] = confThreshold_;
    camNmsThresholds_[camId]  = nmsThreshold_;
    cameraAliases_[camId]     = alias;
    RuntimeConfig::instance().addCamera({source, alias, confThreshold_, nmsThreshold_});
    RuntimeConfig::instance().saveToFile(RuntimeConfig::instance().configFilePath());

    updateModelButtons(true);
    startCameraWorker(camId, camName, source);

    // 更新摄像头状态显示
    ui->cameraStatusLabel->setStyleSheet("font-size: 20px; font-weight: bold; padding: 0 12px; color: green;");
    ui->cameraStatusLabel->setText(QString::fromUtf8("● %1路运行").arg(cameraManager_->count()));
    ui->stopBtn->setEnabled(true);
    if (startDetectBtn_) startDetectBtn_->setEnabled(false);

    refreshCameraList();

    // 自动开始录制
    QTimer::singleShot(500, this, [this, camId]() {
        activeDisplayCamera_ = camId;
        onStartRecording();
    });
}

void MainWindow::onStartDetection() {
    qDebug() << "[DEBUG] onStartDetection";
    if (!modelManager_->isLoaded()) {
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请先加载模型"));
        return;
    }
    auto& cfg = RuntimeConfig::instance();
    auto cams = cfg.cameras();
    if (cams.empty()) {
        QMessageBox::information(this, QString::fromUtf8("提示"),
            QString::fromUtf8("没有已保存的摄像头, 请先使用添加摄像头按钮添加"));
        return;
    }
    autoStartCameras();
    ui->stopBtn->setEnabled(true);
    startDetectBtn_->setEnabled(false);
    statusMessageLabel_->setText(QString::fromUtf8("检测运行中..."));
}

void MainWindow::onRemoveCamera(int cameraId) {
    qDebug() << "[DEBUG] onRemoveCamera" << cameraId;
    // 先保存source(后面stopCamera会清空)
    QString src = cameraSources_.count(cameraId) ? cameraSources_[cameraId] : QString();
    stopCamera(cameraId);

    // 从配置文件删除该摄像头
    auto& cfg = RuntimeConfig::instance();
    auto cams = cfg.cameras();
    for (size_t i = 0; i < cams.size(); ++i) {
        if (cams[i].source == src) {
            cfg.removeCamera(static_cast<int>(i));
            break;
        }
    }
    cfg.saveToFile(cfg.configFilePath());

    qDebug() << "[DEBUG] onRemoveCamera done" << cameraId;
}

void MainWindow::stopCamera(int cameraId) {
    qDebug() << "[DEBUG] stopCamera" << cameraId << "START";
    if (!cameraManager_->contains(cameraId)) { qDebug() << "[DEBUG] stopCamera" << cameraId << "- not in manager"; return; }

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
        if (startDetectBtn_) startDetectBtn_->setEnabled(modelManager_->isLoaded());
        fpsLabel_->setText("FPS: --");
    } else {
        ui->cameraStatusLabel->setText(QString::fromUtf8("● %1路运行").arg(cameraManager_->count()));
    }

    cameraSources_.erase(cameraId);
    cameraAliases_.erase(cameraId);
    camConfThresholds_.erase(cameraId);
    camNmsThresholds_.erase(cameraId);

    GuiLogger::log(ui->logTextEdit, "系统", QString("摄像头 %1 已停止").arg(cameraId));
    refreshCameraList();
    qDebug() << "[DEBUG] stopCamera" << cameraId << "DONE";
}

void MainWindow::stopAllCameras() {
    auto ids = cameraManager_->cameraIds();
    for (int id : ids) {
        stopCamera(id);
    }
}

void MainWindow::autoStartCameras() {
    if (!modelManager_->isLoaded()) return;
    auto& cfg = RuntimeConfig::instance();
    auto cams = cfg.cameras();
    if (cams.empty()) return;

    GuiLogger::log(ui->logTextEdit, "系统", QString("自动启动 %1 路摄像头...").arg(cams.size()));
    for (const auto& cam : cams) {
        int camId = cameraManager_->allocateId();
        QString camName = QString("camera_%1").arg(camId);
        cameraAliases_[camId] = cam.alias;
        confThreshold_ = cam.confThreshold;
        nmsThreshold_  = cam.nmsThreshold;
        camConfThresholds_[camId] = cam.confThreshold;
        camNmsThresholds_[camId]  = cam.nmsThreshold;
        startCameraWorker(camId, camName, cam.source);
    }
    // 恢复第一路摄像头的阈值到 UI
    if (!cams.empty()) {
        confThreshold_ = camConfThresholds_.begin()->second;
        nmsThreshold_  = camNmsThresholds_.begin()->second;
    }
    ui->confSlider->setValue(static_cast<int>(confThreshold_ * 100));
    ui->nmsSlider->setValue(static_cast<int>(nmsThreshold_ * 100));
    ui->cameraBtn->setChecked(true);
    ui->cameraBtn->setText(QString::fromUtf8("关闭摄像头"));
    ui->cameraStatusLabel->setStyleSheet("font-size: 20px; font-weight: bold; padding: 0 12px; color: green;");
    ui->cameraStatusLabel->setText(QString("● %1路运行").arg(cameraManager_->count()));
    ui->stopBtn->setEnabled(true);
    if (startDetectBtn_) startDetectBtn_->setEnabled(false);
    updateModelButtons(true);
    GuiLogger::log(ui->logTextEdit, "系统", QString("自动启动完成, 当前 %1 路摄像头").arg(cameraManager_->count()));
    refreshCameraList();
}

void MainWindow::onCameraListClicked(QListWidgetItem* item) {
    qDebug() << "[DEBUG] onCameraListClicked";
    if (!item) { qDebug() << "[DEBUG] onCameraListClicked: null item"; return; }
    bool ok = false;
    int camId = item->data(Qt::UserRole).toInt(&ok);
    qDebug() << "[DEBUG] onCameraListClicked camId:" << camId << "ok:" << ok;
    if (!ok) return;
    activeDisplayCamera_ = camId;
    QString alias = cameraAliases_.count(activeDisplayCamera_) ? cameraAliases_[activeDisplayCamera_] : QString::number(activeDisplayCamera_);
    GuiLogger::log(ui->logTextEdit, "系统", QString("切换到: %1").arg(alias));
    // 延迟刷新
    QTimer::singleShot(0, this, [this]() { refreshCameraList(); });
    // 切换后同步阈值
    auto it = camConfThresholds_.find(activeDisplayCamera_);
    if (it != camConfThresholds_.end()) {
        confThreshold_ = it->second;
        nmsThreshold_  = camNmsThresholds_[activeDisplayCamera_];
        ui->confSlider->setValue(static_cast<int>(confThreshold_ * 100));
        ui->nmsSlider->setValue(static_cast<int>(nmsThreshold_ * 100));
    }
    qDebug() << "[DEBUG] onCameraListClicked done";
}

void MainWindow::refreshCameraList() {
    cameraListWidget_->clear();
    auto ids = cameraManager_->cameraIds();

    // 更新 activeDisplayCamera_ 保证有效
    if (activeDisplayCamera_ >= 0 && !ids.contains(activeDisplayCamera_)) {
        activeDisplayCamera_ = ids.isEmpty() ? 0 : ids.first();
    }

    for (int id : ids) {
        QString alias = cameraAliases_.count(id) ? cameraAliases_[id] : QString();
        QString src = cameraSources_.count(id) ? cameraSources_[id] : QString();
        bool active = (id == activeDisplayCamera_);

        // 显示名: 别名 / 源地址 / 默认名
        QString displayName;
        if (!alias.isEmpty()) displayName = alias;
        else if (!src.isEmpty()) {
            QString shortSrc = src;
            if (shortSrc.startsWith("rtsp://")) shortSrc = shortSrc.mid(7);
            if (shortSrc.length() > 40) shortSrc = shortSrc.left(37) + "...";
            displayName = shortSrc;
        }
        else displayName = QString("摄像头 %1").arg(id);

        QString text = displayName;
        if (active) text += "  ●";
        // 源地址显示在第二行(仅当有别名的场景, 且不是默认摄像头)
        if (!alias.isEmpty() && !src.isEmpty()) {
            QString shortSrc = src;
            if (shortSrc.startsWith("rtsp://")) shortSrc = shortSrc.mid(7);
            if (shortSrc.length() > 40) shortSrc = shortSrc.left(37) + "...";
            text += "\n" + shortSrc;
        }

        auto* item = new QListWidgetItem(text, cameraListWidget_);
        item->setData(Qt::UserRole, id);
        item->setSizeHint(QSize(0, active ? 52 : 42));

        QFont f = item->font();
        f.setPointSize(active ? 14 : 12);
        f.setBold(active);
        item->setFont(f);

        if (active) item->setForeground(QColor("#2196F3"));
        else item->setForeground(QColor("#cccccc"));
    }
}

void MainWindow::savePerCameraThresholds() {
    auto& cfg = RuntimeConfig::instance();
    auto cams = cfg.cameras();
    bool changed = false;
    for (auto& cam : cams) {
        for (const auto& [camId, src] : cameraSources_) {
            if (src == cam.source) {
                auto it = camConfThresholds_.find(camId);
                if (it != camConfThresholds_.end()) {
                    cam.confThreshold = it->second;
                    cam.nmsThreshold  = camNmsThresholds_[camId];
                    changed = true;
                }
                break;
            }
        }
    }
    if (changed) {
        cfg.setCameras(cams);
        cfg.saveToFile(cfg.configFilePath());
    }
}

void MainWindow::startCameraWorker(int cameraId, const QString& name, const QString& source) {
    qDebug() << "[DEBUG] startCameraWorker" << cameraId << "name:" << name << "source:" << source;
    auto* thread = new QThread(this);
    auto* worker = new InferenceWorker(modelManager_->engine(), cameraId, name);
    worker->moveToThread(thread);

    connect(worker, &InferenceWorker::frameProcessed, this, &MainWindow::onFrameProcessed);
    connect(worker, &InferenceWorker::alertSaved, this, &MainWindow::onAlertSaved);
    connect(worker, &InferenceWorker::finished, this, &MainWindow::onWorkerFinished);
    connect(worker, &InferenceWorker::errorOccurred, this, &MainWindow::onWorkerError);
    connect(thread, &QThread::started, worker, [this, worker, thread, source, cameraId, ct = confThreshold_, nt = nmsThreshold_]() {
        worker->setBatchInference(ui->batchInferenceCheck->isChecked());
        worker->process(std::make_unique<CameraVideoSource>(cameraId, source), ct, nt);
        thread->quit();
    });
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);

    cameraSources_[cameraId] = source;
    cameraManager_->add(cameraId, thread, worker);
    thread->start();
}

void MainWindow::startVideoWorker(const QString& filePath) {
    auto* thread = new QThread(this);
    auto* worker = new InferenceWorker(modelManager_->engine(), -1, "video");
    worker->moveToThread(thread);

    connect(worker, &InferenceWorker::frameProcessed, this, &MainWindow::onFrameProcessed);
    connect(worker, &InferenceWorker::alertSaved, this, &MainWindow::onAlertSaved);
    connect(worker, &InferenceWorker::finished, this, &MainWindow::onWorkerFinished);
    connect(worker, &InferenceWorker::errorOccurred, this, [this](int, const QString& msg) {
        QMessageBox::critical(this, QString::fromUtf8("处理错误"), msg);
    });
    connect(thread, &QThread::started, worker, [this, worker, thread, filePath]() {
        worker->setBatchInference(ui->batchInferenceCheck->isChecked());
        worker->process(std::make_unique<FileVideoSource>(filePath),
                        confThreshold_, nmsThreshold_);
        thread->quit();
    });
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);

    thread->start();
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
    updateModelButtons(false);
    ui->stopBtn->setEnabled(false);
    if (startDetectBtn_) startDetectBtn_->setEnabled(modelManager_->isLoaded());
    fpsLabel_->setText("FPS: --");
    statusMessageLabel_->setText(QString::fromUtf8("已停止"));
    GuiLogger::log(ui->logTextEdit, "系统", "所有处理已停止");
}

void MainWindow::onFrameProcessed(int cameraId, QImage image, std::vector<Detection> detections, double elapsedMs) {
    FrameData fd{cameraId, image, detections, elapsedMs};
    for (auto& sink : sinks_) {
        sink->onFrame(fd);
    }

    // 切换活跃摄像头时同步阈值滑块
    if (cameraId >= 0 && cameraId != prevActiveCam_) {
        prevActiveCam_ = cameraId;
        auto it = camConfThresholds_.find(cameraId);
        if (it != camConfThresholds_.end()) {
            confThreshold_ = it->second;
            nmsThreshold_  = camNmsThresholds_[cameraId];
            ui->confSlider->setValue(static_cast<int>(confThreshold_ * 100));
            ui->nmsSlider->setValue(static_cast<int>(nmsThreshold_ * 100));
        }
    }

    // 只更新当前活跃显示的摄像头画面
    if (cameraId == activeDisplayCamera_) {
        updateDisplay(image);
        updateDetectionList(detections, elapsedMs);
    }
    if (elapsedMs > 0 && cameraId == activeDisplayCamera_) {
        double fps = 1000.0 / elapsedMs;
        fpsLabel_->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
    }
    // 每5帧输出一次检测详情日志(按摄像头独立计数)
    int& logFrameCount = logFrameCounts_[cameraId];
    if (++logFrameCount % 5 == 0 && !detections.empty()) {
        double fps = (elapsedMs > 0) ? 1000.0 / elapsedMs : 0;
        GuiLogger::log(ui->logTextEdit, "检测", QString("%1 | %2 | %3ms, FPS:%4")
            .arg(detections.size())
            .arg(InferenceManager::formatClassSummary(detections))
            .arg(elapsedMs, 0, 'f', 1).arg(fps, 0, 'f', 1));
    }
}

void MainWindow::onWorkerFinished(int cameraId) {
    qDebug() << "[DEBUG] onWorkerFinished" << cameraId << "- contains:" << cameraManager_->contains(cameraId);
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
        updateModelButtons(false);
        ui->stopBtn->setEnabled(false);
        if (startDetectBtn_) startDetectBtn_->setEnabled(modelManager_->isLoaded());
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
    if (activeDisplayCamera_ >= 0 && camConfThresholds_.count(activeDisplayCamera_)) {
        camConfThresholds_[activeDisplayCamera_] = confThreshold_;
        savePerCameraThresholds();
    }
}

void MainWindow::onNmsThresholdChanged(int value) {
    nmsThreshold_ = value / 100.0f;
    updateThresholdLabels();
    if (activeDisplayCamera_ >= 0 && camNmsThresholds_.count(activeDisplayCamera_)) {
        camNmsThresholds_[activeDisplayCamera_] = nmsThreshold_;
        savePerCameraThresholds();
    }
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
            if (item->text() != detectionText(name, detections[i].conf)) { needRebuild = true; break; }
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
                auto* item = new QListWidgetItem(detectionText(name, det.conf), ui->resultListWidget);
                item->setForeground(detectionColor(name));
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
            item->setText(detectionText(name, sorted[i].conf));
            item->setForeground(detectionColor(name));
        }
    }

    ui->totalCountLabel->setText(QString("目标总数: %1").arg(detections.size()));
    timeLabel_->setText(QString("耗时: %1ms").arg(elapsedMs, 0, 'f', 1));
}

bool MainWindow::isDetecting() const {
    return !cameraManager_->isEmpty() || isProcessing_;
}

void MainWindow::updateModelButtons(bool detecting) {
    ui->loadModelBtn->setEnabled(!detecting);
    ui->reloadModelBtn->setEnabled(!detecting && modelManager_->isLoaded());
    ui->browseModelBtn->setEnabled(!detecting);
    ui->modelPathEdit->setReadOnly(detecting);
}

void MainWindow::enableControls(bool enabled) {
    ui->openImageBtn->setEnabled(enabled);
    ui->openVideoBtn->setEnabled(enabled);
    ui->cameraBtn->setEnabled(enabled);
    ui->folderBtn->setEnabled(enabled);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveRuntimeConfig();
    stopAllCameras();

    // 清理输出通道 (必须在 stopAllCameras 之后, 避免 videoRecorder_ 悬空)
    videoRecorder_ = nullptr;
    sinks_.clear();

    // 等待所有工作线程退出 (应用关闭时可以阻塞等待)
    const auto threads = findChildren<QThread*>();
    for (auto* t : threads) {
        t->quit();
        if (!t->wait(3000)) {
            qDebug() << "[WARN] closeEvent: thread" << t << "timed out, terminating";
            t->terminate();
            t->wait(1000);
        }
    }

    event->accept();
}

void MainWindow::onStartRecording() {
    int cameraId = activeDisplayCamera_;
    if (cameraId < 0) {
        QMessageBox::warning(this, QString::fromUtf8("录像"), QString::fromUtf8("请先打开一个摄像头"));
        return;
    }
    QString alias = cameraAliases_.count(cameraId) ? cameraAliases_[cameraId] : QString();
    QString source = cameraSources_.count(cameraId) ? cameraSources_[cameraId] : QString();
    videoRecorder_->startRecording(cameraId, 30.0, alias, source);
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
    switch (cfg.init()) {
    case RuntimeConfig::InitResult::Loaded:
        GuiLogger::log(ui->logTextEdit, QString::fromUtf8("配置"), QString::fromUtf8("已加载运行时配置: %1").arg(cfg.configFilePath()));
        break;
    case RuntimeConfig::InitResult::Created:
        GuiLogger::log(ui->logTextEdit, QString::fromUtf8("配置"), QString::fromUtf8("已生成默认配置: %1").arg(cfg.configFilePath()));
        break;
    case RuntimeConfig::InitResult::Failed:
        GuiLogger::log(ui->logTextEdit, QString::fromUtf8("配置"), QString::fromUtf8("配置文件解析失败, 使用默认值"));
        break;
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
    cfg.saveToFile(cfg.configFilePath());
    GuiLogger::log(ui->logTextEdit, QString::fromUtf8("配置"), QString::fromUtf8("配置已保存: %1").arg(cfg.configFilePath()));
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


