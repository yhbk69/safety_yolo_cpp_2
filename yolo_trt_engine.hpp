/**
 * @file yolo_trt_engine.hpp
 * @brief YOLO11 TensorRT 推理引擎实现 (IEngine 接口)
 *
 * 封装 TensorRT 引擎加载、CUDA 内存管理和推理执行。
 *
 * 设计要点:
 * - 继承 IEngine 接口, 支持多后端统一调用
 * - 两阶段初始化: 默认构造 + load()
 * - RAII: 析构函数自动释放 GPU 资源
 * - 热切换: reload() 支持不重启更换模型
 */

#ifndef YOLO_TRT_ENGINE_HPP
#define YOLO_TRT_ENGINE_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <cuda_runtime.h>
#include "NvInfer.h"
#include "inference_engine.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "types.hpp"
#include "postprocessor.hpp"

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            throw std::runtime_error(std::string("[CUDA Error] ") + \
                cudaGetErrorName(err) + ": " + cudaGetErrorString(err)); \
        } \
    } while(0)

class YoloTrtEngine : public IEngine {
public:
    YoloTrtEngine() = default;
    ~YoloTrtEngine() override { cleanup(); }

    YoloTrtEngine(const YoloTrtEngine&) = delete;
    YoloTrtEngine& operator=(const YoloTrtEngine&) = delete;
    YoloTrtEngine(YoloTrtEngine&&) = default;
    YoloTrtEngine& operator=(YoloTrtEngine&&) = default;

    // ---- IEngine 接口实现 ----

    void load(const std::string& enginePath) override {
        loaded_ = false;
        CUDA_CHECK(cudaStreamCreate(&stream_));
        loadEngine(enginePath);
        createContext();
        allocateGpuBuffers();
        loaded_ = true;
        std::cout << "[Engine] TensorRT engine loaded successfully" << std::endl;
    }

    void reload(const std::string& newPath) override {
        cleanup();
        load(newPath);
        std::cout << "[Engine] Model hot-reloaded: " << newPath << std::endl;
    }

    EngineType type() const override { return EngineType::TensorRT; }
    bool loaded() const override { return loaded_; }
    int inputSize() const override {
        return Config::INPUT_WIDTH * Config::INPUT_HEIGHT * 3;
    }

    void infer(const std::vector<float>& input, std::vector<Detection>& detections,
               int imgWidth, int imgHeight,
               float confThreshold, float iouThreshold) override
    {
        std::lock_guard<std::mutex> lock(inferMutex_);
        CUDA_CHECK(cudaMemcpyAsync(gpuInputBuffer_, input.data(),
                    input.size() * sizeof(float), cudaMemcpyHostToDevice, stream_));
        CUDA_CHECK(cudaStreamSynchronize(stream_));

        context_->executeV2(gpuBuffers_);

        std::vector<float> output(getOutputSize());
        CUDA_CHECK(cudaMemcpy(output.data(), gpuOutputBuffer_,
                    output.size() * sizeof(float), cudaMemcpyDeviceToHost));

        detections = Postprocessor::decodeDetections(output.data(), imgWidth, imgHeight,
                                                       confThreshold, iouThreshold);
    }

    void batchInfer(const std::vector<std::vector<float>>& inputs,
                    std::vector<std::vector<Detection>>& detectionsList,
                    const std::vector<std::pair<int,int>>& imgSizes,
                    float confThreshold, float iouThreshold) override
    {
        std::lock_guard<std::mutex> lock(inferMutex_);
        const int batchSize = static_cast<int>(inputs.size());
        if (batchSize == 0) return;
        if (batchSize > Config::BATCH_SIZE) {
            throw std::runtime_error(std::string("[Engine] Batch size ") +
                std::to_string(batchSize) + " exceeds max " +
                std::to_string(Config::BATCH_SIZE));
        }

        // 拼接 batch 输入
        size_t elemPerInput = inputs[0].size();
        std::vector<float> batchInput(elemPerInput * batchSize);
        for (int i = 0; i < batchSize; ++i)
            std::copy(inputs[i].begin(), inputs[i].end(), batchInput.begin() + i * elemPerInput);

        CUDA_CHECK(cudaMemcpyAsync(gpuInputBuffer_, batchInput.data(),
                    batchInput.size() * sizeof(float), cudaMemcpyHostToDevice, stream_));
        CUDA_CHECK(cudaStreamSynchronize(stream_));
        context_->executeV2(gpuBuffers_);

        std::vector<float> output(getOutputSize() * batchSize);
        CUDA_CHECK(cudaMemcpy(output.data(), gpuOutputBuffer_,
                    output.size() * sizeof(float), cudaMemcpyDeviceToHost));

        detectionsList.resize(batchSize);
        for (int i = 0; i < batchSize; ++i) {
            float* frameOut = output.data() + i * getOutputSize();
            detectionsList[i] = Postprocessor::decodeDetections(
                frameOut, imgSizes[i].first, imgSizes[i].second, confThreshold, iouThreshold);
        }
    }

private:
    std::mutex inferMutex_;
    bool loaded_ = false;
    cudaStream_t stream_ = nullptr;
    std::unique_ptr<nvinfer1::IRuntime> runtime_;
    std::unique_ptr<nvinfer1::ICudaEngine> engine_;
    std::unique_ptr<nvinfer1::IExecutionContext> context_;
    void* gpuInputBuffer_ = nullptr;
    void* gpuOutputBuffer_ = nullptr;
    void* gpuBuffers_[2] = {nullptr, nullptr};
    TrtLogger logger_;

    void cleanup() {
        if (stream_) {
            cudaStreamSynchronize(stream_);
            cudaStreamDestroy(stream_);
            stream_ = nullptr;
        }
        if (gpuInputBuffer_)  { cudaFree(gpuInputBuffer_);  gpuInputBuffer_ = nullptr; }
        if (gpuOutputBuffer_) { cudaFree(gpuOutputBuffer_); gpuOutputBuffer_ = nullptr; }
        context_.reset();
        engine_.reset();
        runtime_.reset();
        loaded_ = false;
    }

    int getOutputSize() const {
        return (Config::NUM_CLASSES + 4) * 8400;
    }

    void loadEngine(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.good())
            throw std::runtime_error("[Engine] Cannot open: " + path);
        file.seekg(0, file.end);
        size_t size = static_cast<size_t>(file.tellg());
        file.seekg(0, file.beg);
        std::vector<char> data(size);
        file.read(data.data(), size);
        file.close();

        runtime_.reset(nvinfer1::createInferRuntime(logger_));
        engine_.reset(runtime_->deserializeCudaEngine(data.data(), size));
        if (!engine_)
            throw std::runtime_error("[Engine] Failed to deserialize: " + path);
    }

    void createContext() {
        context_.reset(engine_->createExecutionContext());
    }

    void allocateGpuBuffers() {
        CUDA_CHECK(cudaMalloc(&gpuInputBuffer_,  inputSize()  * Config::BATCH_SIZE * sizeof(float)));
        CUDA_CHECK(cudaMalloc(&gpuOutputBuffer_, getOutputSize() * Config::BATCH_SIZE * sizeof(float)));
        gpuBuffers_[0] = gpuInputBuffer_;
        gpuBuffers_[1] = gpuOutputBuffer_;
    }
};

#undef CUDA_CHECK

#endif // YOLO_TRT_ENGINE_HPP
