/**
 * @file postprocessor.hpp
 * @brief YOLO11 后处理与可视化模块
 *
 * 本模块负责处理TensorRT模型输出的原始张量, 包括:
 * 1. 解析模型输出, 提取检测框和类别信息
 * 2. NMS(非极大值抑制)去除冗余框
 * 3. 在图像上绘制检测结果
 *
 * 教学要点:
 * YOLO11的输出格式为[num_classes+4, 8400](列优先)
 * NMS是目标检测中必不可少的步骤, 用于去除重叠的冗余框
 */

#ifndef POSTPROCESSOR_HPP
#define POSTPROCESSOR_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include "config.hpp"
#include "types.hpp"

namespace Postprocessor {

    // Letterbox信息结构体 (用于坐标反变换)
    struct LetterboxInfo {
        int origW, origH;       // 原始图像尺寸
        int padW, padH;         // letterbox 填充后图像在画布中的尺寸
        int offsetX, offsetY;   // 填充偏移量 (画布左上角)

        static LetterboxInfo compute(int origW, int origH) {
            float scaleW = static_cast<float>(Config::INPUT_WIDTH)  / origW;
            float scaleH = static_cast<float>(Config::INPUT_HEIGHT) / origH;
            float scale = std::min(scaleW, scaleH);

            int newW = static_cast<int>(origW * scale);
            int newH = static_cast<int>(origH * scale);
            int offX = (Config::INPUT_WIDTH  - newW) / 2;
            int offY = (Config::INPUT_HEIGHT - newH) / 2;

            return {origW, origH, newW, newH, offX, offY};
        }

        // 将模型输出坐标 (640x640 空间) 转换为原始图像坐标
        cv::Rect toOriginal(float mx1, float my1, float mx2, float my2) const {
            // 先减去 letterbox 偏移, 再缩放到原始尺寸
            float scale = static_cast<float>(padW) / Config::INPUT_WIDTH;  // 等比缩放
            float x1 = (mx1 - offsetX) / scale;
            float y1 = (my1 - offsetY) / scale;
            float x2 = (mx2 - offsetX) / scale;
            float y2 = (my2 - offsetY) / scale;

            int finalX = std::max(0, static_cast<int>(x1));
            int finalY = std::max(0, static_cast<int>(y1));
            int finalW = std::min(origW - finalX, static_cast<int>(x2 - x1));
            int finalH = std::min(origH - finalY, static_cast<int>(y2 - y1));

            if (finalW <= 0 || finalH <= 0) return cv::Rect(0, 0, 0, 0);
            return cv::Rect(finalX, finalY, finalW, finalH);
        }
    };

    // 边界框格式枚举
    enum BoxFormat {
        XYWH_CENTER,  // cx, cy, w, h (YOLOv8 传统格式)
        XYXY_CORNER   // x1, y1, x2, y2 (YOLO11 anchor-free 格式)
    };

    // 从模型原始输出中提取有效检测结果
    // YOLO11 输出格式: [1, numChannels, numAnchors] (NCHW)
    // 内存布局: 行优先 (row-major), 即 [channel0全部锚点, channel1全部锚点, ...]
    // 通道0-3=边界框坐标, 通道4起=类别分数
    // outputData:    模型输出张量数据 (float*)
    // numAnchors:    锚点数量 (从模型输出张量动态查询, YOLO11 640x640=8400)
    // numChannels:   通道数 (默认 4+numClasses=15, 即 8400 锚点 x 15 通道)
    // lbInfo:        letterbox 变换信息 (用于坐标反变换)
    // boxFormat:     边界框格式 (XYWH_CENTER 或 XYXY_CORNER)
    // confThreshold: 置信度阈值, 低于该值的检测被过滤
    // iouThreshold:  NMS的IOU阈值, 用于去除重叠框
    static std::vector<Detection> decodeDetections(const float* outputData, int numAnchors, int numChannels,
                                                    const LetterboxInfo& lbInfo,
                                                    BoxFormat boxFormat = XYXY_CORNER,
                                                    float confThreshold = Config::CONF_THRESHOLD,
                                                    float iouThreshold = Config::IOU_THRESHOLD) {
        std::vector<cv::Rect> boxes;
        std::vector<float> confidences;
        std::vector<int> classIds;

        int numClasses = numChannels - 4;  // 通道数 = 4(bbox) + numClasses

        // 遍历所有锚点, 找出置信度高于阈值的目标
        for (int i = 0; i < numAnchors; ++i) {
            // 找出最高分类得分及其类别索引
            int classId = 0;
            float maxConf = -1.0f;
            for (int j = 0; j < numClasses; ++j) {
                // 行优先布局: outputData[channel * numAnchors + anchor_index]
                float score = outputData[(4 + j) * numAnchors + i];
                if (score > maxConf) {
                    maxConf = score;
                    classId = j;
                }
            }

            // 过滤低置信度检测
            if (maxConf < confThreshold) continue;

            // 读取边界框坐标 (在 640x640 模型空间中)
            float b0 = outputData[0 * numAnchors + i];
            float b1 = outputData[1 * numAnchors + i];
            float b2 = outputData[2 * numAnchors + i];
            float b3 = outputData[3 * numAnchors + i];

            cv::Rect box;

            if (boxFormat == XYXY_CORNER) {
                // YOLO11 anchor-free 格式: x1, y1, x2, y2
                box = lbInfo.toOriginal(b0, b1, b2, b3);
            } else {
                // YOLOv8 传统格式: cx, cy, w, h
                float mx1 = b0 - b2 / 2.0f;
                float my1 = b1 - b3 / 2.0f;
                float mx2 = b0 + b2 / 2.0f;
                float my2 = b1 + b3 / 2.0f;
                box = lbInfo.toOriginal(mx1, my1, mx2, my2);
            }

            if (box.width <= 0 || box.height <= 0) continue;

            boxes.push_back(box);
            confidences.push_back(maxConf);
            classIds.push_back(classId);
        }

        // NMS(非极大值抑制)去除冗余框
        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, confidences, confThreshold, iouThreshold, indices);

        // 将保留的框转换为Detection结构体
        std::vector<Detection> detections;
        detections.reserve(indices.size());
        for (int idx : indices) {
            Detection det;
            det.x = static_cast<float>(boxes[idx].x);
            det.y = static_cast<float>(boxes[idx].y);
            det.w = static_cast<float>(boxes[idx].width);
            det.h = static_cast<float>(boxes[idx].height);
            det.conf = confidences[idx];
            det.class_id = classIds[idx];
            detections.push_back(det);
        }

        return detections;
    }

    // 在图像上绘制检测结果(边界框和类别标签)
    static void drawDetections(cv::Mat& img, const std::vector<Detection>& detections) {
        for (const auto& det : detections) {
            std::string className = Config::CLASS_NAMES[det.class_id];
            // no_xx 用红色，正常用绿色
            cv::Scalar boxColor = (className.find("no_") == 0) ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
            char confBuf[8];
            snprintf(confBuf, sizeof(confBuf), "%.3f", det.conf);
            std::string label = className + ": " + confBuf;

            // 绘制边界框
            cv::rectangle(img,
                cv::Point(static_cast<int>(det.x), static_cast<int>(det.y)),
                cv::Point(static_cast<int>(det.x + det.w), static_cast<int>(det.y + det.h)),
                boxColor, 2);

            // 计算文本尺寸
            int baseline = 0;
            cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);

            // 绘制文本背景
            cv::rectangle(img,
                cv::Point(static_cast<int>(det.x), static_cast<int>(det.y) - textSize.height - 5),
                cv::Point(static_cast<int>(det.x) + textSize.width, static_cast<int>(det.y)),
                boxColor, -1);

            // 绘制文本
            cv::putText(img, label,
                cv::Point(static_cast<int>(det.x), static_cast<int>(det.y) - 5),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
        }
    }

} // namespace Postprocessor

#endif // POSTPROCESSOR_HPP
