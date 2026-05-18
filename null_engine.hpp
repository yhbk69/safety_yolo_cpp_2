/**
 * @file null_engine.hpp
 * @brief 空推理引擎桩 (无 GPU 环境下的占位实现)
 *
 * 当系统无 TensorRT/CUDA/RKNN 等推理后端时使用,
 * 仅返回空检测结果, 使 GUI 部分可独立编译运行。
 */

#ifndef NULL_ENGINE_HPP
#define NULL_ENGINE_HPP

#include "inference_engine.hpp"
#include <iostream>

class NullEngine : public IEngine {
public:
    void load(const std::string& modelPath) override {
        std::cout << "[NullEngine] Stub loaded (no-op): " << modelPath << std::endl;
    }

    EngineType type() const override { return EngineType::ONNX; } // 占位
    bool loaded() const override { return false; }
    int inputSize() const override { return 640 * 640 * 3; }

    void infer(const std::vector<float>& /*input*/,
               std::vector<Detection>& detections,
               int /*imgWidth*/, int /*imgHeight*/,
               float /*confThreshold*/, float /*iouThreshold*/) override
    {
        detections.clear(); // 返回空结果
    }

    void batchInfer(const std::vector<std::vector<float>>& inputs,
                    std::vector<std::vector<Detection>>& detectionsList,
                    const std::vector<std::pair<int,int>>& /*imgSizes*/,
                    float /*confThreshold*/, float /*iouThreshold*/) override
    {
        detectionsList.resize(inputs.size());
        for (auto& d : detectionsList) d.clear();
    }
};

#endif // NULL_ENGINE_HPP
