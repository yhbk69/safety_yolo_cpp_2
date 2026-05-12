# 告警图片/视频推送异常 — 根因分析

> 问题：客户端收到告警后，图片和视频链接无法查看

---

## 根因：HTTP 文件服务器从未启动

### 证据

**`mainwindow.hpp:121`** — 声明了成员变量：
```cpp
std::unique_ptr<HttpFileServer> httpFileServer_;
```

**`mainwindow.cpp` — 构造函数初始化顺序**：
```
MjpegStreamer     ✅ 创建 + start()   (line 155-158)
WebSocketManager  ✅ 创建 + start()   (line 162-188)
VideoRecorder     ✅ 创建              (line 200-220)
ModelManager      ✅ 创建              (line 223+)
CameraManager     ✅ 创建              (line 236+)
InferenceManager  ✅ 创建              (line 246+)
HttpFileServer    ❌ 从未创建/启动！    ← BUG
```

**`mainwindow.cpp:269`** — 析构函数中唯一的引用：
```cpp
httpFileServer_.reset();  // reset 一个从未创建的 unique_ptr = no-op
```

### 完整异常链路

```
1. InferenceWorker::saveAlertFiles()
   └─ 写入 output/alarm_{uuid}_{type}.mp4   ✅ 文件生成
   └─ 写入 output/alarm_{uuid}_{type}.jpg   ✅ 文件生成
   └─ 构造 JSON: {
        "video_url": "http://192.168.124.28:9091/alarm_xxx.mp4",
        "image_url": "http://192.168.124.28:9091/alarm_xxx.jpg"
      }

2. WebSocketManager::broadcast()
   └─ 通过 WebSocket 发送 JSON              ✅ 客户端收到

3. 客户端解析 JSON，尝试访问 http://192.168.124.28:9091/alarm_xxx.jpg
   └─ 端口 9091 上没有任何服务在监听       ❌ CONNECTION REFUSED
   └─ 客户端无法获取图片/视频               ❌
```

---

## 修复方案

### Fix 1：在构造函数中启动 HTTP 文件服务器

在 `mainwindow.cpp` 构造函数中，`WebSocketManager` 创建之后、`wsAddressLabel_` 之前插入：

```cpp
// 在 line 195 "wsAddressLabel_->setText(...)" 之前插入

// 初始化 HTTP 文件服务 (用于告警图片/视频下载)
httpFileServer_ = std::make_unique<HttpFileServer>();
{
    auto actualPort = RuntimeConfig::instance().httpPort();
    QString rootDir = QDir::cleanPath(QDir::currentPath() + "/" +
        QString::fromStdString(Config::OUTPUT_DIR));
    httpFileServer_->start(actualPort, rootDir);
    httpFileServer_->setLogCallback(GuiLogger::makeLogCallback(ui->logTextEdit));
}
```

位置选择：
- 要在 `QDir().mkpath(Config::OUTPUT_DIR)` 之后（line 145 已执行）
- 要在 `wsAddressLabel_->setText(...)` 之前（可选，可以一起更新地址标签）
- 建议在 line 195（`wsAddressLabel_->setText`）之前插入

### Fix 2：添加视频保存失败的错误处理（inference_worker.cpp）

```cpp
void InferenceWorker::saveAlertFiles(const QString& alarmId, const QString& alarmType) {
    if (alertBuffer_.empty()) return;

    // ... toBgr lambda ...

    int frameW = alertBuffer_.back()->cols;
    int frameH = alertBuffer_.back()->rows;
    if (frameW <= 0 || frameH <= 0) return;

    QString baseName = QString("alarm_%1_%2").arg(alarmId).arg(alarmType);
    QString videoPath = outputDir_ + "/" + baseName + ".mp4";
    QString imagePath = outputDir_ + "/" + baseName + ".jpg";

    // ---- 新增: 多 codec 尝试 ----
    bool videoSaved = false;
    // 尝试 H264 → 现代浏览器可直接播放
    int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');
    cv::VideoWriter writer(videoPath.toStdString(), fourcc, 30.0,
                           cv::Size(frameW, frameH));
    if (!writer.isOpened()) {
        // 回退到 MP4V
        fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
        writer.open(videoPath.toStdString(), fourcc, 30.0, cv::Size(frameW, frameH));
    }
    if (!writer.isOpened()) {
        // 最后回退到 XVID (AVI 容器, 一定可用)
        fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
        QString aviPath = outputDir_ + "/" + baseName + ".avi";
        writer.open(aviPath.toStdString(), fourcc, 30.0, cv::Size(frameW, frameH));
        if (writer.isOpened()) videoPath = aviPath; // 更新路径
    }

    if (writer.isOpened()) {
        for (auto& f : alertBuffer_) {
            writer.write(toBgr(f));
        }
        writer.release();
        videoSaved = true;
        qDebug() << "[Alert] 视频已保存:" << videoPath
                 << "codec:" << (fourcc == cv::VideoWriter::fourcc('a','v','c','1') ? "H264"
                              : fourcc == cv::VideoWriter::fourcc('m','p','4','v') ? "MP4V"
                              : "XVID");
    } else {
        qWarning() << "[Alert] 视频保存失败: 无可用编码器";
        // 视频文件不存在，从 JSON 中移除 video_url
    }
    // ----

    bool imageSaved = cv::imwrite(imagePath.toStdString(), toBgr(alertBuffer_.front()));
    if (!imageSaved) {
        qWarning() << "[Alert] 图片保存失败:" << imagePath;
    }

    // 构造告警 JSON
    QString hostIp = QString::fromStdString(Config::HOST_IP);
    QJsonObject data;
    data["alarm_id"]   = alarmId;
    data["alarm_type"] = alarmType;
    data["timestamp"]  = QDateTime::currentDateTime().toMSecsSinceEpoch();

    // ---- 新增: 根据实际保存结果填充 URL ----
    if (videoSaved) {
        QFileInfo vi(videoPath);
        data["video_url"] = QString("http://%1:%2/%3").arg(
            hostIp).arg(Config::HTTP_PORT).arg(vi.fileName());
    }
    if (imageSaved) {
        QFileInfo im(imagePath);
        data["image_url"] = QString("http://%1:%2/%3").arg(
            hostIp).arg(Config::HTTP_PORT).arg(im.fileName());
    }
    // ----
    // 额外标记用于客户端降级显示
    data["video_available"] = videoSaved;
    data["image_available"] = imageSaved;

    QJsonObject root;
    root["type"] = "alarm";
    root["data"] = data;
    QString alertJson = QString::fromUtf8(
        QJsonDocument(root).toJson(QJsonDocument::Compact));

    emit alertSaved(cameraId_, videoPath, imagePath, alertJson);
}
```

### Fix 3：修正 HOST_IP 替换（可选，建议一起做）

在 `saveAlertFiles` 里 `Config::HOST_IP` 是硬编码的 `"192.168.124.28"`，放到其他机器上就跑不通。建议自动获取本机 IP：

```cpp
// 在 inference_worker.hpp 或 saveAlertFiles 开头添加
static QString getLocalIP() {
    for (const QHostAddress& addr : QNetworkInterface::allAddresses()) {
        if (addr != QHostAddress::LocalHost && addr.protocol() == QAbstractSocket::IPv4Protocol)
            return addr.toString();
    }
    return QString::fromStdString(Config::HOST_IP);  // fallback
}
```

---

## 修复确认清单

- [ ] `httpFileServer_->start()` 在构造中调用 → 端口 9091 开始监听
- [ ] `saveAlertFiles()` 多 codec 回退 → 即使 MP4V 不可用也能保存视频
- [ ] `saveAlertFiles()` 不输出不存在文件的 URL → 客户端不会收到 404
- [ ] JSON 中添加 `video_available` / `image_available` → 客户端可做降级展示
- [ ] 日志中打印保存结果 → 方便排查后续问题

---

## 次要问题（可能遇见但非当前根因）

| 问题 | 状态 | 说明 |
|------|------|------|
| `output` 目录不存在 | ✅ 已处理 | `QDir().mkpath(...)` 和 `InferenceWorker` 构造中都创建 |
| mp4v 编码器不可用 | ⚠️ 待验证 | Fix 2 中用多 codec 回退兜底 |
| `alertBuffer_` 空 | ✅ 已处理 | `saveAlertFiles` 开头检查 `empty()` |
| 跨线程 `writer.release()` 未刷写 | ✅ 无影响 | `release()` 后 `emit alertSaved` 排队到主线程，文件已写盘 |

---

*分析时间：2026-05-12 | 基于 `D:\dltt\yolo_cpp\yolo11_2` 当前代码*