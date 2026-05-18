/**
 * @file rknn_inference_engine.hpp
 * @brief RKNN (Rockchip NPU) 推理引擎实现 (IEngine 接口)
 * @note 需要 librknnrt.so + rknn_api.h
 */
#ifndef RKNN_INFERENCE_ENGINE_HPP
#define RKNN_INFERENCE_ENGINE_HPP

#include "inference_engine.hpp"

class RknnInferenceEngine : public IEngine {
public:
    RknnInferenceEngine();
    ~RknnInferenceEngine() override;

    // ---- IEngine 接口实现 ----
    void load(const std::string& modelPath) override;
    EngineType type() const override { return EngineType::RKNN; }
    bool loaded() const override;
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
    int inputSize_ = 640;
};

#endif // RKNN_INFERENCE_ENGINE_HPP
