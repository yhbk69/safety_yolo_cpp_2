/**
 * @file rknn_inference_engine.cpp
 * @brief RKNN (Rockchip NPU) 推理引擎实现
 * @note 链接 librknnrt.so, 使用 rknn_api.h 中的 C API
 */
#include "rknn_inference_engine.hpp"
#include "config.hpp"
#include "types.hpp"
#include "postprocessor.hpp"
#include "preprocessor.hpp"

#include <rknn_api.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <vector>

// ============================================================
// PIMPL 实现
// ============================================================
struct RknnInferenceEngine::Impl {
    rknn_context ctx = 0;
    int inputWidth  = 640;
    int inputHeight = 640;
    int channel     = 3;
    int inputSize   = 640 * 640 * 3;

    ~Impl() {
        if (ctx) {
            rknn_destroy(ctx);
            ctx = 0;
        }
    }
};

// ============================================================
// 构造函数 / 析构函数
// ============================================================
RknnInferenceEngine::RknnInferenceEngine()
    : impl_(std::make_unique<Impl>())
{
}

RknnInferenceEngine::~RknnInferenceEngine() = default;

// ============================================================
// load
// ============================================================
void RknnInferenceEngine::load(const std::string& modelPath) {
    // 读取 .rknn 模型文件
    std::ifstream file(modelPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("[RKNN] Cannot open model: " + modelPath);
    }
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> modelData(size);
    file.read(reinterpret_cast<char*>(modelData.data()), size);
    file.close();

    // 初始化 RKNN 上下文
    int ret = rknn_init(&impl_->ctx, modelData.data(), size, 0, nullptr);
    if (ret != 0) {
        throw std::runtime_error("[RKNN] rknn_init failed, ret=" + std::to_string(ret));
    }

    // 查询输入输出信息
    rknn_input_output_num io_num;
    ret = rknn_query(impl_->ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (ret != 0) {
        throw std::runtime_error("[RKNN] rknn_query IN_OUT_NUM failed, ret=" + std::to_string(ret));
    }

    // 查询输入张量属性 (获取 input size)
    std::vector<rknn_tensor_attr> input_attrs(io_num.n_input);
    input_attrs[0].index = 0;
    ret = rknn_query(impl_->ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[0]), sizeof(rknn_tensor_attr));
    if (ret == 0) {
        impl_->inputWidth  = input_attrs[0].dims[2];
        impl_->inputHeight = input_attrs[0].dims[1];
        impl_->channel     = input_attrs[0].dims[3];
        impl_->inputSize   = impl_->inputWidth * impl_->inputHeight * impl_->channel;
        inputSize_ = impl_->inputSize;
    }

    std::cout << "[RKNN] Model loaded: " << modelPath
              << " (" << impl_->inputWidth << "x" << impl_->inputHeight << ")"
              << std::endl;
}

bool RknnInferenceEngine::loaded() const {
    return impl_ && impl_->ctx != 0;
}

// ============================================================
// reload (热切换, 先销毁旧 ctx 再加载新模型)
// ============================================================
void RknnInferenceEngine::reload(const std::string& newPath) {
    if (impl_) {
        impl_.reset();  // 析构函数调用 rknn_destroy, 安全释放旧 ctx
    }
    impl_ = std::make_unique<Impl>();
    load(newPath);      // 加载新模型
}

// ============================================================
// infer
// ============================================================
void RknnInferenceEngine::infer(const std::vector<float>& input,
                                 std::vector<Detection>& detections,
                                 int imgWidth, int imgHeight,
                                 float confThreshold, float iouThreshold)
{
    if (!loaded()) {
        detections.clear();
        return;
    }

    // ----- 输入 -----
    rknn_input inputs[1];
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index        = 0;
    inputs[0].type         = RKNN_TENSOR_FLOAT32;
    inputs[0].size         = impl_->inputSize * sizeof(float);
    inputs[0].fmt          = RKNN_TENSOR_NCHW;
    inputs[0].buf          = const_cast<float*>(input.data());
    inputs[0].pass_through = 0;

    int ret = rknn_inputs_set(impl_->ctx, 1, inputs);
    if (ret != 0) {
        std::cerr << "[RKNN] rknn_inputs_set failed, ret=" << ret << std::endl;
        detections.clear();
        return;
    }

    // ----- 推理 -----
    ret = rknn_run(impl_->ctx, nullptr);
    if (ret != 0) {
        std::cerr << "[RKNN] rknn_run failed, ret=" << ret << std::endl;
        detections.clear();
        return;
    }

    // ----- 输出 -----
    rknn_output outputs[1];
    memset(outputs, 0, sizeof(outputs));
    outputs[0].want_float = 1;
    outputs[0].index      = 0;

    ret = rknn_outputs_get(impl_->ctx, 1, outputs, nullptr);
    if (ret != 0) {
        std::cerr << "[RKNN] rknn_outputs_get failed, ret=" << ret << std::endl;
        detections.clear();
        return;
    }

    // 查询输出张量维度 (动态获取 numAnchors 和 numChannels)
    rknn_tensor_attr output_attr;
    memset(&output_attr, 0, sizeof(output_attr));
    output_attr.index = 0;
    ret = rknn_query(impl_->ctx, RKNN_QUERY_OUTPUT_ATTR, &output_attr, sizeof(output_attr));
    if (ret != 0) {
        std::cerr << "[RKNN] rknn_query OUTPUT_ATTR failed, ret=" << ret << std::endl;
        rknn_outputs_release(impl_->ctx, 1, outputs);
        detections.clear();
        return;
    }

    // 输出格式: [1, numChannels, numAnchors] (NCHW)
    int numChannels = output_attr.dims[1];  // 默认 84 (4+11类别)
    int numAnchors  = output_attr.dims[2];  // 默认 8400 (640x640输入)

    // 验证输出类型 (量化模型可能输出 INT8, 需要不同后处理)
    if (output_attr.type != RKNN_TENSOR_FLOAT32) {
        std::cerr << "[RKNN] WARNING: Model output is not FLOAT32, got type=" << output_attr.type
                  << ". Quantized models may require different postprocessing." << std::endl;
    }

    // 后处理 (动态锚点数)
    float* outputData = static_cast<float*>(outputs[0].buf);
    detections = Postprocessor::decodeDetections(
        outputData, numAnchors, numChannels,
        imgWidth, imgHeight, confThreshold, iouThreshold);

    rknn_outputs_release(impl_->ctx, 1, outputs);
}

// ============================================================
// batchInfer (串行回退)
// ============================================================
void RknnInferenceEngine::batchInfer(
    const std::vector<std::vector<float>>& inputs,
    std::vector<std::vector<Detection>>& detectionsList,
    const std::vector<std::pair<int,int>>& imgSizes,
    float confThreshold, float iouThreshold)
{
    detectionsList.resize(inputs.size());
    for (size_t i = 0; i < inputs.size(); ++i) {
        infer(inputs[i], detectionsList[i],
              imgSizes[i].first, imgSizes[i].second,
              confThreshold, iouThreshold);
    }
}
