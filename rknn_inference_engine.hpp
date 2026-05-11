/**
 * @file rknn_inference_engine.hpp
 * @brief RKNN (Rockchip NPU) 推理引擎实现 (IEngine 接口)
 * @note 需要 RKNN SDK 8.30+
 */
#ifndef RKNN_INFERENCE_ENGINE_HPP
#define RKNN_INFERENCE_ENGINE_HPP

#include "inference_engine.hpp"
#include "preprocessor.hpp"
#include "postprocessor.hpp"

// RKNN SDK 头文件 (条件编译)
// #include <rknn_api.h>

// 前向声明
struct rknn_context;

class RknnInferenceEngine : public IEngine {
public:
    RknnInferenceEngine();
    ~RknnInferenceEngine() override;

    // ---- IEngine 接口实现 ----

    void load(const std::string& modelPath) override;
    EngineType type() const override { return EngineType::RKNN; }
    bool loaded() const override { return ctx_ != 0; }
    int inputSize() const override { return inputSize_; }

    void infer(const std::vector<float>& input, std::vector<Detection>& detections,
               int imgWidth, int imgHeight,
               float confThreshold, float iouThreshold) override;

    void batchInfer(const std::vector<std::vector<float>>& inputs,
                    std::vector<std::vector<Detection>>& detectionsList,
                    const std::vector<std::pair<int,int>>& imgSizes,
                    float confThreshold, float iouThreshold) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    rknn_context ctx_ = 0;
    std::unique_ptr<Preprocessor> preprocessor_;
    std::unique_ptr<Postprocessor> postprocessor_;
    std::string modelName_;
    int inputSize_ = 640;
    int inputIndex_ = 0;
    int outputIndex_ = 0;
};

#endif // RKNN_INFERENCE_ENGINE_HPP
