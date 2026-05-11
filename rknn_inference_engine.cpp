/**
 * @file rknn_inference_engine.cpp
 * @brief RKNN (Rockchip NPU) 推理引擎实现
 * @note 需要 RKNN SDK 8.30+ (librknnapp.so)
 */
#include "rknn_inference_engine.hpp"
#include "config.hpp"
#include "types.hpp"
#include "postprocessor.hpp"
#include <iostream>
#include <dlfcn.h>
#include <cstring>

// RKNN API 函数指针类型定义
typedef int (*rknn_init_t)(rknn_context*, void*, size_t, rknn_init_extended_t*);
typedef int (*rknn_destroy_t)(rknn_context);
typedef int (*rknn_query_t)(rknn_context, rknn_query_cmd, void*, int);
typedef int (*rknn_inputs_set_t)(rknn_context, uint32_t, rknn_input*);
typedef int (*rknn_run_t)(rknn_context, rknn_run_extended_t*);
typedef int (*rknn_outputs_get_t)(rknn_context, uint32_t, rknn_tensor**);

// RKNN 上下文句柄 (前向声明)
struct rknn_context { int dummy = 0; };

class RknnInferenceEngine::Impl {
public:
    void* libHandle_ = nullptr;
    rknn_init_t rknn_init = nullptr;
    rknn_destroy_t rknn_destroy = nullptr;
    rknn_query_t rknn_query = nullptr;
    rknn_inputs_set_t rknn_inputs_set = nullptr;
    rknn_run_t rknn_run = nullptr;
    rknn_outputs_get_t rknn_outputs_get = nullptr;

    bool loadLibrary() {
        const char* libPaths[] = {
            "/usr/lib/librknnapi.so",
            "/usr/lib64/librknnapi.so",
            "/opt/rknn/lib/librknnapi.so",
            nullptr
        };
        for (int i = 0; libPaths[i]; i++) {
            libHandle_ = dlopen(libPaths[i], RTLD_LAZY);
            if (libHandle_) break;
        }
        if (!libHandle_) {
            std::cerr << "[RKNN] librknnapi.so not found" << std::endl;
            return false;
        }
        rknn_init = (rknn_init_t)dlsym(libHandle_, "rknn_init");
        rknn_destroy = (rknn_destroy_t)dlsym(libHandle_, "rknn_destroy");
        rknn_query = (rknn_query_t)dlsym(libHandle_, "rknn_query");
        rknn_inputs_set = (rknn_inputs_set_t)dlsym(libHandle_, "rknn_inputs_set");
        rknn_run = (rknn_run_t)dlsym(libHandle_, "rknn_run");
        rknn_outputs_get = (rknn_outputs_get_t)dlsym(libHandle_, "rknn_outputs_get");
        return (rknn_init && rknn_destroy && rknn_run);
    }

    ~Impl() {
        if (libHandle_) dlclose(libHandle_);
    }
};

RknnInferenceEngine::RknnInferenceEngine()
    : preprocessor_(std::make_unique<Preprocessor>())
    , postprocessor_(std::make_unique<Postprocessor>()) {
}

RknnInferenceEngine::~RknnInferenceEngine() {
    if (ctx_) {
        // rknn_destroy(ctx_);  // 当 SDK 可用时
    }
}

void RknnInferenceEngine::load(const std::string& modelPath) {
    modelName_ = modelPath;
    // TODO: RKNN SDK 实际加载
    // impl_ = std::make_unique<Impl>();
    // if (!impl_->loadLibrary()) throw std::runtime_error("[RKNN] SDK not found");
    // ... 加载 .rknn 模型文件 ...
    inputSize_ = 640;
    std::cout << "[RknnInferenceEngine] Stub loaded: " << modelPath << std::endl;
}

void RknnInferenceEngine::infer(const std::vector<float>& input,
                                 std::vector<Detection>& detections,
                                 int imgWidth, int imgHeight,
                                 float confThreshold, float iouThreshold) {
    if (!loaded()) return;
    // TODO: RKNN 实际推理
    // 1. 将 float tensor 转换为 RKNN 输入格式 (uint8 量化)
    // 2. rknn_inputs_set(ctx_, 1, &input)
    // 3. rknn_run(ctx_, nullptr)
    // 4. rknn_outputs_get(ctx_, 1, &outputs)
    // 5. detections = postprocessor_->process(outputs, imgWidth, imgHeight, ...)
    std::cerr << "[RKNN] Inference stub - SDK not loaded" << std::endl;
}

void RknnInferenceEngine::batchInfer(const std::vector<std::vector<float>>& inputs,
                                      std::vector<std::vector<Detection>>& detectionsList,
                                      const std::vector<std::pair<int,int>>& imgSizes,
                                      float confThreshold, float iouThreshold) {
    if (!loaded()) return;
    // 默认回退: 串行逐帧推理
    detectionsList.resize(inputs.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
        infer(inputs[i], detectionsList[i], imgSizes[i].first, imgSizes[i].second,
              confThreshold, iouThreshold);
    }
}
