# ============================================================
# CMakeLists.txt - YOLO11 TensorRT 鎺ㄧ悊绯荤粺 (Qt GUI 鐗堟湰)
# ============================================================
# 浣跨敤鏂规硶:
#   1. 瀹夎 Qt 5.15.2 (閫夋嫨 MSVC 2019 64-bit 缁勪欢)
#   2. 瀹夎 CMake >= 3.16
#   3. 鍦ㄩ」鐩洰褰曚笅鎵ц:
#      mkdir build && cd build
#      cmake .. -G "Visual Studio 18 2026" -A x64 -DQt5_DIR="C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5"
#      cmake --build . --config Release
#
# 娉ㄦ剰: CMake 浼氳嚜鍔ㄥ鐞?Qt 鐨?MOC (Meta-Object Compiler),
# 涓嶉渶瑕侀澶栭厤缃? 姣?.vcxproj + Qt VS Tools 鏇寸畝娲?
# ============================================================

cmake_minimum_required(VERSION 3.16)
project(yolo11_2 LANGUAGES CXX)

# ============ 缂栬瘧閫夐」 ============
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 鍚敤 UTF-8 婧愭枃浠剁紪鐮?瑙ｅ喅 Windows 涓嬩腑鏂囦贡鐮侀棶棰?
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# 鑷姩鐢熸垚 MOC 鏂囦欢(澶勭悊 Q_OBJECT 瀹? Qt 蹇呴渶)
set(CMAKE_AUTOMOC ON)

# 鑷姩澶勭悊 .ui 鏂囦欢(鐢?uic 鐢熸垚 ui_*.h)
set(CMAKE_AUTOUIC ON)

# ============ 鏌ユ壘渚濊禆搴?============

# ---- Qt5 (GUI 妗嗘灦) ----
# 瀹夎鍚庤缃?Qt5_DIR 鐜鍙橀噺, 鎴栧湪 cmake 鏃舵寚瀹?
#   cmake .. -DQt5_DIR="C:/Qt/5.15.2/msvc2019_64/lib/cmake/Qt5"
find_package(Qt5 REQUIRED COMPONENTS Widgets WebSockets)

# ---- OpenCV (鍥惧儚澶勭悊) ----
set(OpenCV_DIR "C:/opencv412/opencv/build")
find_package(OpenCV REQUIRED)

# ---- CUDA (GPU 鍔犻€? ----
find_package(CUDA REQUIRED)

# ---- TensorRT (鎺ㄧ悊寮曟搸) ----
# TensorRT 娌℃湁瀹樻柟鐨?CMake 閰嶇疆鏂囦欢, 鎵嬪姩鎸囧畾璺緞
set(TENSORRT_DIR "C:/tensorRT10/TensorRT-10.1.0.27")
set(TENSORRT_INCLUDE_DIR "${TENSORRT_DIR}/include")
set(TENSORRT_LIB_DIR "${TENSORRT_DIR}/lib")

# ============ 婧愭枃浠跺垪琛?============
set(SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.ui        # Qt UI 甯冨眬鏂囦欢(鐢?Qt Creator 鍙鍖栫紪杈?
)

set(HEADERS
    mainwindow.hpp         # 鍚?Q_OBJECT 瀹? 闇€瑕?MOC 澶勭悊
    config.hpp
    types.hpp
    logger.hpp
    preprocessor.hpp
    postprocessor.hpp
    yolo_trt_engine.hpp
    inference_runner.hpp
)

# ============ 鐢熸垚鍙墽琛屾枃浠?============
add_executable(yolo11_2 WIN32 ${SOURCES} ${HEADERS})

# ============ 澶存枃浠惰矾寰?============
target_include_directories(yolo11_2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}          # 椤圭洰鏍圭洰褰?鑷韩澶存枃浠?
    ${OpenCV_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    ${TENSORRT_INCLUDE_DIR}
)

# ============ 閾炬帴搴?============
# 鎸囧畾 TensorRT 搴撴悳绱㈣矾寰?璁╅摼鎺ュ櫒鑳芥壘鍒?nvinfer_10.lib)
target_link_directories(yolo11_2 PRIVATE ${TENSORRT_LIB_DIR})

target_link_libraries(yolo11_2 PRIVATE
    Qt5::Widgets
    Qt5::WebSockets
    ${OpenCV_LIBS}
    ${CUDA_LIBRARIES}
    nvinfer_10          # TensorRT 鎺ㄧ悊搴?
    nvonnxparser_10     # TensorRT ONNX 瑙ｆ瀽鍣?
)

# ============ 杈撳嚭淇℃伅 ============
message(STATUS "Qt5 found: ${Qt5Widgets_INCLUDE_DIRS}")
message(STATUS "OpenCV found: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "CUDA found: ${CUDA_INCLUDE_DIRS}")
message(STATUS "TensorRT found: ${TENSORRT_INCLUDE_DIR}")
