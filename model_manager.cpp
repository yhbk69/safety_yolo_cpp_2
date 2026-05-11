/**
 * @file model_manager.cpp
 * @brief 模型加载管理模块实现
 */

#include "model_manager.hpp"
#include "yolo_trt_engine.hpp"
#include <QString>

ModelManager::ModelManager()
    : engine_(nullptr)
{
}

bool ModelManager::load(const std::string& path) {
    try {
        auto eng = std::make_unique<YoloTrtEngine>();
        eng->load(path);
        engine_ = std::move(eng);
        if (callbacks_.log) {
            callbacks_.log("模型", QString("模型加载成功: %1").arg(QString::fromStdString(path)));
        }
        return true;
    } catch (const std::exception& e) {
        engine_.reset();
        if (callbacks_.onError) {
            callbacks_.onError(QString::fromUtf8(e.what()));
        }
        return false;
    }
}

bool ModelManager::reload(const std::string& path) {
    if (!engine_) return false;
    try {
        engine_->reload(path);
        if (callbacks_.log) {
            callbacks_.log("模型", QString("模型热切换成功: %1").arg(QString::fromStdString(path)));
        }
        return true;
    } catch (const std::exception& e) {
        if (callbacks_.onError) {
            callbacks_.onError(QString::fromUtf8(e.what()));
        }
        return false;
    }
}
