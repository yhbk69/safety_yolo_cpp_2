/**
 * @file model_manager.cpp
 * @brief 模型加载管理模块实现
 *
 * 自动选择推理后端: RKNN > TensorRT > NullEngine(占位)
 */

#include "model_manager.hpp"
#include "null_engine.hpp"

#ifdef USE_TENSORRT
#include "yolo_trt_engine.hpp"
#endif

#ifdef USE_RKNN
#include "rknn_inference_engine.hpp"
#endif

#include <QString>
#include <QFileInfo>

ModelManager::ModelManager()
    : engine_(nullptr)
{
}

bool ModelManager::load(const std::string& path) {
    try {
        std::unique_ptr<IEngine> eng;
        QString lower = QFileInfo(QString::fromStdString(path)).suffix().toLower();

#ifdef USE_RKNN
        if (lower == "rknn") {
            eng = std::make_unique<RknnInferenceEngine>();
        } else
#endif
#ifdef USE_TENSORRT
        if (lower == "engine") {
            eng = std::make_unique<YoloTrtEngine>();
        } else
#endif
        {
            // 无可用的推理引擎后端, 使用空引擎占位 (GUI 开发用)
            if (callbacks_.log) {
                callbacks_.log("模型", QString("无可用的推理后端, 使用空引擎占位 (GUI 模式)"));
            }
            eng = std::make_unique<NullEngine>();
        }

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
