# ============================================================
# CMakeLists.txt - YOLO11 PPE Detection (Qt GUI)
# ============================================================
# Linux (ARM64 无 GPU):
#   mkdir build && cd build
#   cmake .. -DCMAKE_BUILD_TYPE=Release
#   cmake --build . -j"$(nproc)"
#
# Linux (x86_64 with NVIDIA GPU + TensorRT):
#   mkdir build && cd build
#   cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_TENSORRT=ON
#   cmake --build . -j"$(nproc)"
# ============================================================

cmake_minimum_required(VERSION 3.16)

# 提前设置编译器: ARM64 环境使用 g++-10 (支持 C++20)
# 可通过 -DCMAKE_CXX_COMPILER= 覆盖
if(NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(GPP10 g++-10)
    if(GPP10)
        set(CMAKE_CXX_COMPILER "${GPP10}" CACHE FILEPATH "C++ compiler")
        message(STATUS "Using g++-10 for C++20 support")
    endif()
endif()

project(yolo11_2 LANGUAGES CXX)

# ============ 编译选项 ============
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 跨平台 UTF-8 源文件编码
if(MSVC)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# 自动生成 MOC 文件(处理 Q_OBJECT 宏, Qt 必需)
set(CMAKE_AUTOMOC ON)

# 自动处理 .ui 文件(由 uic 生成 ui_*.h)
set(CMAKE_AUTOUIC ON)

# ============ 查找依赖库 ============

# ---- Qt5 (GUI 框架) ----
find_package(Qt5 REQUIRED COMPONENTS Widgets WebSockets)

# ---- OpenCV (图像处理) ----
find_package(OpenCV REQUIRED)

# ---- CUDA (GPU 加速, 可选) ----
find_package(CUDA QUIET)
if(CUDA_FOUND)
    message(STATUS "CUDA found: ${CUDA_INCLUDE_DIRS}")
    add_definitions(-DUSE_CUDA)
else()
    message(STATUS "CUDA not found — building without GPU support")
endif()

# ---- TensorRT (推理引擎, 可选) ----
option(USE_TENSORRT "Build with TensorRT inference engine" OFF)
if(USE_TENSORRT)
    if(NOT DEFINED TENSORRT_DIR)
        set(TENSORRT_DIR "$ENV{TENSORRT_DIR}")
    endif()
    if(NOT TENSORRT_DIR)
        set(TENSORRT_DIR "/usr/local/tensorrt" CACHE PATH "TensorRT installation root")
    endif()
    if(EXISTS "${TENSORRT_DIR}/include/NvInfer.h")
        set(TENSORRT_INCLUDE_DIR "${TENSORRT_DIR}/include")
        set(TENSORRT_LIB_DIR "${TENSORRT_DIR}/lib")
        message(STATUS "TensorRT found: ${TENSORRT_DIR}")
        add_definitions(-DUSE_TENSORRT)
    else()
        message(STATUS "TensorRT not found at ${TENSORRT_DIR} — disabling")
        set(USE_TENSORRT OFF)
    endif()
endif()

# ---- RKNN (Rockchip NPU 推理引擎, 可选) ----
find_path(RKNN_INCLUDE_DIR rknn_api.h
    PATHS
        /usr/include
        /usr/local/include
        /opt/rknn/include
        /home/ztl/dltt/code/rknn/rknn_model_zoo-v2.3.0/3rdparty/rknpu2/include
)
find_library(RKNN_RT_LIB rknnrt
    PATHS
        /usr/lib/aarch64-linux-gnu
        /usr/lib
        /usr/local/lib
)
if(RKNN_INCLUDE_DIR AND RKNN_RT_LIB)
    message(STATUS "RKNN found: ${RKNN_INCLUDE_DIR}")
    add_definitions(-DUSE_RKNN)
else()
    message(STATUS "RKNN not found — building with NullEngine stub")
endif()

# ============ 源文件列表 ============
set(SOURCES
    main.cpp
    mainwindow.cpp
    gui_logger.cpp
    inference_worker.cpp
    video_source.cpp
    camera_manager.cpp
    model_manager.cpp
    settings_dialog.cpp
    video_recorder.cpp
    websocket_manager.cpp
    video_websocket_server.cpp
    alert_websocket_server.cpp
    http_file_server.cpp
    inference_manager.cpp
    mainwindow.ui        # Qt UI 布局文件(由 Qt Creator 可视化编辑)
)

# 按可用后端选择性编译推理引擎源文件
if(USE_TENSORRT)
    list(APPEND SOURCES yolo_trt_engine.cpp)
endif()
if(RKNN_INCLUDE_DIR AND RKNN_RT_LIB)
    list(APPEND SOURCES rknn_inference_engine.cpp)
endif()

set(HEADERS
    mainwindow.hpp
    gui_logger.hpp
    config.hpp
    types.hpp
    logger.hpp
    preprocessor.hpp
    postprocessor.hpp
    inference_engine.hpp
    inference_worker.hpp
    video_source.hpp
    output_sink.hpp
    camera_manager.hpp
    detection_utils.hpp
    inference_manager.hpp
    model_manager.hpp
    runtime_config.hpp
    settings_dialog.hpp
    video_recorder.hpp
    websocket_manager.hpp
    video_websocket_server.hpp
    alert_websocket_server.hpp
    http_file_server.hpp
    null_engine.hpp
)

# ============ 生成可执行文件 ============
if(WIN32)
    add_executable(yolo11_2 WIN32 ${SOURCES} ${HEADERS})
else()
    add_executable(yolo11_2 ${SOURCES} ${HEADERS})
endif()

# ============ 头文件路径 ============
target_include_directories(yolo11_2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

if(CUDA_FOUND)
    target_include_directories(yolo11_2 PRIVATE ${CUDA_INCLUDE_DIRS})
endif()
if(USE_TENSORRT)
    target_include_directories(yolo11_2 PRIVATE ${TENSORRT_INCLUDE_DIR})
endif()
if(RKNN_INCLUDE_DIR)
    target_include_directories(yolo11_2 PRIVATE ${RKNN_INCLUDE_DIR})
endif()

# ============ 链接库 ============
target_link_libraries(yolo11_2 PRIVATE
    Qt5::Widgets
    Qt5::WebSockets
    ${OpenCV_LIBS}
)

if(CUDA_FOUND)
    target_link_libraries(yolo11_2 PRIVATE ${CUDA_LIBRARIES})
endif()
if(USE_TENSORRT)
    target_link_directories(yolo11_2 PRIVATE ${TENSORRT_LIB_DIR})
    if(WIN32)
        target_link_libraries(yolo11_2 PRIVATE nvinfer_10 nvonnxparser_10)
    else()
        target_link_libraries(yolo11_2 PRIVATE nvinfer nvonnxparser)
    endif()
endif()
if(RKNN_RT_LIB)
    target_link_libraries(yolo11_2 PRIVATE ${RKNN_RT_LIB})
endif()

# Linux 特定链接
if(UNIX AND NOT APPLE)
    target_link_libraries(yolo11_2 PRIVATE dl pthread)
endif()

# ============ 输出信息 ============
message(STATUS "Qt5 found: ${Qt5Widgets_INCLUDE_DIRS}")
message(STATUS "OpenCV found: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "CUDA found: ${CUDA_FOUND}")
message(STATUS "TensorRT: ${USE_TENSORRT}")
message(STATUS "RKNN: ${RKNN_INCLUDE_DIR}")
