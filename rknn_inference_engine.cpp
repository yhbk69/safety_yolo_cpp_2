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

    // ----- 输入 (RKNN 只支持 NHWC 格式) -----
    // 将 CHW 张量转换为 HWC (NHWC) 格式
    // input 是 CHW 格式: [C0全部, C1全部, C2全部]
    // 需要转换为 HWC 格式: [H0W0C0, H0W0C1, H0W0C2, H1W1C0, ...]
    std::vector<uint8_t> nhwcInput(impl_->inputSize * sizeof(float));
    {
        const float* chwData = input.data();
        float* nhwcData = reinterpret_cast<float*>(nhwcInput.data());
        int H = impl_->inputHeight;
        int W = impl_->inputWidth;
        int C = impl_->channel;
        int planeSize = H * W;

        for (int h = 0; h < H; ++h) {
            for (int w = 0; w < W; ++w) {
                for (int c = 0; c < C; ++c) {
                    nhwcData[(h * W + w) * C + c] = chwData[c * planeSize + h * W + w];
                }
            }
        }
    }

    rknn_input inputs[1];
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index        = 0;
    inputs[0].type         = RKNN_TENSOR_FLOAT32;
    inputs[0].size         = nhwcInput.size();
    inputs[0].fmt          = RKNN_TENSOR_NHWC;   // RKNN 需要 NHWC 格式
    inputs[0].buf          = nhwcInput.data();
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
    // RKNN dims 数组: dims[0]=N, dims[1]=C, dims[2]=H/W (对于1D输出)
    // YOLO11 通常输出 [1, 84, 8400] 或 [1, 8400, 84] 取决于导出方式
    int n_dims = output_attr.n_dims;
    int numChannels = 0;
    int numAnchors = 0;

    // 打印所有维度用于调试
    std::cout << "[RKNN] Output dims: ";
    for (int i = 0; i < n_dims; ++i) {
        std::cout << output_attr.dims[i] << " ";
    }
    std::cout << "(n_dims=" << n_dims << ")" << std::endl;

    if (n_dims >= 3) {
        // 尝试 [1, C, N] 格式 (NCHW)
        numChannels = output_attr.dims[1];
        numAnchors = output_attr.dims[2];
    } else if (n_dims == 2) {
        // 2D 输出 [C, N]
        numChannels = output_attr.dims[0];
        numAnchors = output_attr.dims[1];
    }

    // 验证: numAnchors 应该 > 1000 (YOLO11 典型值 8400)
    // 如果 numAnchors 太小, 说明维度解析错误, 交换 channels 和 anchors
    if (numAnchors < 1000 && numChannels < 1000) {
        // 两个都很小, 可能是一维输出被展开了, 用 total size 推断
        int totalElements = output_attr.n_elems;
        // 假设 numClasses = Config::NUM_CLASSES = 11, 则 numChannels = 4 + 11 = 15
        int assumedClasses = 15;
        numAnchors = totalElements / assumedClasses;
        numChannels = assumedClasses;
        std::cout << "[RKNN] Inferred from total elements: " << totalElements
                  << " -> channels=" << numChannels << ", anchors=" << numAnchors << std::endl;
    }

    std::cout << "[RKNN] Using numChannels=" << numChannels << ", numAnchors=" << numAnchors << std::endl;

    // 输出类型检查 (FP16 是正常的, SDK 会自动转 FP32 因为 want_float=1)
    if (output_attr.type != RKNN_TENSOR_FLOAT32) {
        std::cout << "[RKNN] Output type: " << get_type_string(output_attr.type)
                  << " (will be converted to FP32 by SDK)" << std::endl;
    }

    // 后处理 (动态锚点数)
    float* outputData = static_cast<float*>(outputs[0].buf);

    // 调试: 打印输出数据统计信息
    float maxVal = -1e30f, minVal = 1e30f;
    double sumVal = 0;
    int totalElems = numChannels * numAnchors;
    for (int i = 0; i < totalElems; ++i) {
        float v = outputData[i];
        if (v > maxVal) maxVal = v;
        if (v < minVal) minVal = v;
        sumVal += v;
    }
    float meanVal = static_cast<float>(sumVal / totalElems);
    std::cout << "[RKNN] Output stats: min=" << minVal << ", max=" << maxVal
              << ", mean=" << meanVal << std::endl;

    // 打印前3个锚点的置信度 (通道4) 用于调试
    std::cout << "[RKNN] First 3 anchor confidences (channel 4): ";
    for (int i = 0; i < 3 && i < numAnchors; ++i) {
        std::cout << outputData[4 * numAnchors + i] << " ";
    }
    std::cout << std::endl;

    // 检查所有通道的最大值和平均值
    std::vector<float> chMax(numChannels, -1e30f);
    std::vector<double> chSum(numChannels, 0);
    for (int c = 0; c < numChannels; ++c) {
        for (int i = 0; i < numAnchors; ++i) {
            float v = outputData[c * numAnchors + i];
            if (v > chMax[c]) chMax[c] = v;
            chSum[c] += v;
        }
    }
    std::cout << "[RKNN] Per-channel max: ";
    for (int c = 0; c < numChannels; ++c) {
        std::cout << "ch" << c << "=" << chMax[c] << " ";
    }
    std::cout << std::endl;

    // 打印最高置信度的锚点信息
    float globalMaxConf = -1.0f;
    int bestAnchor = -1, bestClass = -1;
    for (int i = 0; i < numAnchors; ++i) {
        float maxClassScore = -1.0f;
        int bestCls = -1;
        for (int c = 0; c < numChannels - 4; ++c) {
            float s = outputData[(4 + c) * numAnchors + i];
            if (s > maxClassScore) { maxClassScore = s; bestCls = c; }
        }
        if (maxClassScore > globalMaxConf) {
            globalMaxConf = maxClassScore;
            bestAnchor = i;
            bestClass = bestCls;
        }
    }
    std::cout << "[RKNN] Best anchor: #" << bestAnchor << ", class=" << bestClass
              << ", conf=" << globalMaxConf
              << ", x1=" << outputData[bestAnchor]
              << ", y1=" << outputData[numAnchors + bestAnchor]
              << ", x2=" << outputData[2*numAnchors + bestAnchor]
              << ", y2=" << outputData[3*numAnchors + bestAnchor]
              << ", w=" << (outputData[2*numAnchors + bestAnchor] - outputData[bestAnchor])
              << ", h=" << (outputData[3*numAnchors + bestAnchor] - outputData[numAnchors + bestAnchor])
              << std::endl;

    // 计算 letterbox 变换信息 (用于坐标反变换)
    auto lbInfo = Postprocessor::LetterboxInfo::compute(imgWidth, imgHeight);

    detections = Postprocessor::decodeDetections(
        outputData, numAnchors, numChannels,
        lbInfo, Postprocessor::XYXY_CORNER,
        confThreshold, iouThreshold);

    std::cout << "[RKNN] Decoded " << detections.size() << " detections" << std::endl;

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
