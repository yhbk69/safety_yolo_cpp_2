/**
 * @file inference_engine.hpp
 * @brief YOLO 推理引擎抽象接口
 *
 * 所有推理后端(TensorRT, RKNN, ONNX Runtime, OpenVINO, PyTorch)
 * 均实现此接口, 上层代码通过 IEngine* 统一调用。
 *
 * 设计要点:
 * - 两阶段初始化: 默认构造 + load(), 便于工厂创建和各后端统一管理
 * - 输入约定: 预处理后的 float 张量 (CHW 格式, [0,1] 归一化)
 * - 输出: 统一为 Detection 列表 (后处理在各引擎内完成)
 * - 批量推理提供默认回退 (逐帧串行), 支持原生 batch 的后端可覆盖
 */

#ifndef INFERENCE_ENGINE_HPP
#define INFERENCE_ENGINE_HPP

#include <vector>
#include <string>
#include <memory>
#include <utility>
#include "types.hpp"

enum class EngineType { TensorRT, RKNN, ONNX, OpenVINO, PyTorch };

class IEngine {
public:
    virtual ~IEngine() = default;

    /// 加载模型文件, 失败抛 std::runtime_error
    virtual void load(const std::string& modelPath) = 0;

    /// 热切换 (默认与 load 相同, 不支持的后端保留空实现)
    virtual void reload(const std::string& newPath) { load(newPath); }

    /// 引擎类型标识
    virtual EngineType type() const = 0;

    /// 是否已成功加载
    virtual bool loaded() const = 0;

    /// 输入张量元素个数 (供预处理器分配缓冲区)
    virtual int inputSize() const = 0;

    /**
     * @brief 单帧推理
     * @param input     预处理后的 float 张量 (CHW, [0,1])
     * @param detections [out] 检测结果列表
     * @param imgWidth  原始图像宽度 (用于坐标缩放)
     * @param imgHeight 原始图像高度
     */
    virtual void infer(const std::vector<float>& input, std::vector<Detection>& detections,
                       int imgWidth, int imgHeight,
                       float confThreshold, float iouThreshold) = 0;

    /**
     * @brief 批量推理
     * @param inputs          batch 个输入张量
     * @param detectionsList  [out] 每帧的检测结果
     * @param imgSizes       每帧原始尺寸 {(w,h), ...}
     *
     * 默认实现逐帧串行回退, 支持原生 batch 的后端 (TensorRT 等) 可覆盖。
     */
    virtual void batchInfer(const std::vector<std::vector<float>>& inputs,
                            std::vector<std::vector<Detection>>& detectionsList,
                            const std::vector<std::pair<int,int>>& imgSizes,
                            float confThreshold, float iouThreshold) {
        detectionsList.resize(inputs.size());
        for (size_t i = 0; i < inputs.size(); ++i) {
            infer(inputs[i], detectionsList[i], imgSizes[i].first, imgSizes[i].second,
                  confThreshold, iouThreshold);
        }
    }
};

#endif // INFERENCE_ENGINE_HPP
