/**
 * @file runtime_config.hpp
 * @brief 运行时可变配置 — 运行时默认值的单一来源
 *
 * 所有运行时可调的参数（阈值、端口、冷却时间、路径等）的**默认值**
 * 仅在此处定义，SettingsDialog 从此处读取，Config 命名空间同步引用。
 *
 * 新增可调参数时：
 *   1. 在此文件添加成员变量 + getter/setter
 *   2. 在 toJson() / applyJson() 中添加序列化
 *   3. 在 Config 命名空间添加同名 constexpr（如需编译期访问）
 *   4. 在 SettingsDialog 中添加 UI 控件
 */

#ifndef RUNTIME_CONFIG_HPP
#define RUNTIME_CONFIG_HPP

#include <QString>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QDateTime>
#include <mutex>
#include <string>
#include <vector>

/**
 * 运行时配置(线程安全单例)
 * 
 * 用法:
 *   auto& cfg = RuntimeConfig::instance();
 *   float conf = cfg.confThreshold();
 *   cfg.loadFromFile("config.json");  // 热重载
 */
/** 记忆的摄像头条目 */
struct CameraEntry {
    QString source;       // 设备ID(e.g. "0") 或 RTSP URL
    QString alias;        // 用户自定义别名
    float confThreshold = 0.25f;
    float nmsThreshold  = 0.45f;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["source"]         = source;
        obj["alias"]          = alias;
        obj["conf_threshold"] = confThreshold;
        obj["nms_threshold"]  = nmsThreshold;
        return obj;
    }

    static CameraEntry fromJson(const QJsonObject& obj) {
        CameraEntry e;
        e.source         = obj["source"].toString();
        e.alias          = obj["alias"].toString();
        e.confThreshold  = (float)obj["conf_threshold"].toDouble(0.25);
        e.nmsThreshold   = (float)obj["nms_threshold"].toDouble(0.45);
        return e;
    }

    QString displayName() const {
        return alias.isEmpty() ? source : alias;
    }
};

class RuntimeConfig {
public:
    static RuntimeConfig& instance() {
        static RuntimeConfig inst;
        return inst;
    }

    // ---- 运行时可变参数 (getter/setter) ----

    float confThreshold() const       { std::lock_guard<std::mutex> lk(mu_); return confThreshold_; }
    void setConfThreshold(float v)    { std::lock_guard<std::mutex> lk(mu_); confThreshold_ = v; }

    float iouThreshold() const        { std::lock_guard<std::mutex> lk(mu_); return iouThreshold_; }
    void setIouThreshold(float v)     { std::lock_guard<std::mutex> lk(mu_); iouThreshold_ = v; }

    int websocketPort() const         { std::lock_guard<std::mutex> lk(mu_); return websocketPort_; }
    void setWebsocketPort(int v)      { std::lock_guard<std::mutex> lk(mu_); websocketPort_ = v; }

    int alertWsPort() const           { std::lock_guard<std::mutex> lk(mu_); return alertWsPort_; }
    void setAlertWsPort(int v)        { std::lock_guard<std::mutex> lk(mu_); alertWsPort_ = v; }

    int streamPort() const            { std::lock_guard<std::mutex> lk(mu_); return streamPort_; }
    void setStreamPort(int v)         { std::lock_guard<std::mutex> lk(mu_); streamPort_ = v; }

    int ackTimeoutMs() const          { std::lock_guard<std::mutex> lk(mu_); return ackTimeoutMs_; }
    void setAckTimeoutMs(int v)       { std::lock_guard<std::mutex> lk(mu_); ackTimeoutMs_ = v; }

    int ringBufferFrames() const      { std::lock_guard<std::mutex> lk(mu_); return ringBufferFrames_; }
    void setRingBufferFrames(int v)   { std::lock_guard<std::mutex> lk(mu_); ringBufferFrames_ = v; }

    int alertAfterFrames() const      { std::lock_guard<std::mutex> lk(mu_); return alertAfterFrames_; }
    void setAlertAfterFrames(int v)   { std::lock_guard<std::mutex> lk(mu_); alertAfterFrames_ = v; }

    int alertCooldownMs() const       { std::lock_guard<std::mutex> lk(mu_); return alertCooldownMs_; }
    void setAlertCooldownMs(int v)    { std::lock_guard<std::mutex> lk(mu_); alertCooldownMs_ = v; }

    QString modelPath() const         { std::lock_guard<std::mutex> lk(mu_); return modelPath_; }
    void setModelPath(const QString& v) { std::lock_guard<std::mutex> lk(mu_); modelPath_ = v; }

    QString outputDir() const         { std::lock_guard<std::mutex> lk(mu_); return outputDir_; }
    void setOutputDir(const QString& v) { std::lock_guard<std::mutex> lk(mu_); outputDir_ = v; }

    QString recordDir() const         { std::lock_guard<std::mutex> lk(mu_); return recordDir_; }
    void setRecordDir(const QString& v) { std::lock_guard<std::mutex> lk(mu_); recordDir_ = v; }

    int recordKeepDays() const        { std::lock_guard<std::mutex> lk(mu_); return recordKeepDays_; }
    void setRecordKeepDays(int v)     { std::lock_guard<std::mutex> lk(mu_); recordKeepDays_ = v; }

    // ---- 记忆摄像头列表 ----

    std::vector<CameraEntry> cameras() const {
        std::lock_guard<std::mutex> lk(mu_);
        return cameras_;
    }
    void setCameras(const std::vector<CameraEntry>& v) {
        std::lock_guard<std::mutex> lk(mu_);
        cameras_ = v;
    }
    void addCamera(const CameraEntry& cam) {
        std::lock_guard<std::mutex> lk(mu_);
        cameras_.push_back(cam);
    }
    void removeCamera(int index) {
        std::lock_guard<std::mutex> lk(mu_);
        if (index >= 0 && index < (int)cameras_.size())
            cameras_.erase(cameras_.begin() + index);
    }
    void clearCameras() {
        std::lock_guard<std::mutex> lk(mu_);
        cameras_.clear();
    }

    // ---- 文件 I/O ----

    /** init() 返回值: 已加载 / 已创建默认 / 解析失败 */
    enum class InitResult { Loaded, Created, Failed };

    /**
     * @brief 初始化: 从 configFilePath() 加载配置; 文件不存在则创建默认配置
     * @return Loaded 成功加载 / Created 已创建默认文件 / Failed 解析失败
     */
    InitResult init() {
        if (QFile::exists(configFilePath_)) {
            return loadFromFile(configFilePath_) ? InitResult::Loaded : InitResult::Failed;
        }
        return saveToFile(configFilePath_) ? InitResult::Created : InitResult::Failed;
    }

    bool loadFromFile(const QString& path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "[RuntimeConfig] Cannot open:" << path;
            return false;
        }
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "[RuntimeConfig] Parse error:" << err.errorString();
            return false;
        }
        return applyJson(doc.object());
    }

    bool saveToFile(const QString& path) const {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        QJsonObject root = toJson();
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }

    // 导出当前配置为JSON
    QJsonObject toJson() const {
        std::lock_guard<std::mutex> lk(mu_);
        QJsonObject root;
        root["conf_threshold"]     = confThreshold_;
        root["iou_threshold"]      = iouThreshold_;
        root["websocket_port"]     = websocketPort_;
        root["alert_ws_port"]      = alertWsPort_;
        root["stream_port"]        = streamPort_;
        root["ack_timeout_ms"]     = ackTimeoutMs_;
        root["ring_buffer_frames"] = ringBufferFrames_;
        root["alert_after_frames"] = alertAfterFrames_;
        root["alert_cooldown_ms"]  = alertCooldownMs_;
        root["model_path"]         = modelPath_;
        root["output_dir"]         = outputDir_;
        root["record_dir"]         = recordDir_;
        root["record_keep_days"]   = recordKeepDays_;
        QJsonArray camArr;
        for (const auto& c : cameras_) {
            camArr.append(c.toJson());
        }
        root["cameras"] = camArr;
        return root;
    }

    QString configFilePath() const { return configFilePath_; }

private:
    RuntimeConfig() = default;

    bool applyJson(const QJsonObject& root) {
        std::lock_guard<std::mutex> lk(mu_);
        if (root.contains("conf_threshold"))     confThreshold_     = (float)root["conf_threshold"].toDouble(confThreshold_);
        if (root.contains("iou_threshold"))      iouThreshold_      = (float)root["iou_threshold"].toDouble(iouThreshold_);
        if (root.contains("websocket_port"))     websocketPort_     = root["websocket_port"].toInt(websocketPort_);
        if (root.contains("alert_ws_port"))      alertWsPort_       = root["alert_ws_port"].toInt(alertWsPort_);
        else if (root.contains("http_port"))     alertWsPort_       = root["http_port"].toInt(alertWsPort_); // 兼容旧配置
        if (root.contains("stream_port"))        streamPort_        = root["stream_port"].toInt(streamPort_);
        if (root.contains("ack_timeout_ms"))     ackTimeoutMs_      = root["ack_timeout_ms"].toInt(ackTimeoutMs_);
        if (root.contains("ring_buffer_frames")) ringBufferFrames_  = root["ring_buffer_frames"].toInt(ringBufferFrames_);
        if (root.contains("alert_after_frames")) alertAfterFrames_  = root["alert_after_frames"].toInt(alertAfterFrames_);
        if (root.contains("alert_cooldown_ms"))  alertCooldownMs_   = root["alert_cooldown_ms"].toInt(alertCooldownMs_);
        if (root.contains("model_path"))         modelPath_         = root["model_path"].toString(modelPath_);
        if (root.contains("output_dir"))         outputDir_         = root["output_dir"].toString(outputDir_);
        if (root.contains("record_dir"))         recordDir_         = root["record_dir"].toString(recordDir_);
        if (root.contains("record_keep_days"))   recordKeepDays_    = root["record_keep_days"].toInt(recordKeepDays_);
        if (root.contains("cameras")) {
            cameras_.clear();
            QJsonArray arr = root["cameras"].toArray();
            for (const auto& v : arr) {
                if (v.isString()) {
                    // 旧格式: 纯字符串 source
                    CameraEntry e;
                    e.source = v.toString();
                    cameras_.push_back(std::move(e));
                } else {
                    cameras_.push_back(CameraEntry::fromJson(v.toObject()));
                }
            }
        }
        return true;
    }

    mutable std::mutex mu_;

    // 默认值从Config命名空间同步
    float confThreshold_     = 0.25f;
    float iouThreshold_      = 0.45f;
    int   websocketPort_     = 9090;
    int   alertWsPort_       = 9091;
    int   streamPort_        = 9092;
    int   ackTimeoutMs_      = 5000;
    int   ringBufferFrames_  = 90;
    int   alertAfterFrames_  = 60;
    int   alertCooldownMs_   = 10000;
    QString modelPath_       = "model/best.engine";
    QString outputDir_       = "output";
    QString recordDir_       = "recordings";
    int   recordKeepDays_    = 7;
    std::vector<CameraEntry> cameras_;
    QString configFilePath_  = "config.json";
};

#endif // RUNTIME_CONFIG_HPP
