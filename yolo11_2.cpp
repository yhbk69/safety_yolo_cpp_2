/**
 * @file yolo11_2.cpp
 * @brief YOLO11 TensorRT 鎺ㄧ悊绯荤粺 - 涓诲叆鍙ｆ枃浠?
 *
 * 鏈▼搴忔槸涓€涓熀浜嶵ensorRT鐨刌OLO11鐩爣妫€娴嬫帹鐞嗙郴缁?
 * 鐢ㄤ簬瀹炴椂妫€娴嬩釜浜洪槻鎶よ澶?PPE)鐨勪僵鎴存儏鍐?
 *
 * 浠ｇ爜閲囩敤妯″潡鍖栬璁? 鍚勬枃浠惰亴璐ｅ涓?
 * config.hpp           - 閰嶇疆鍙傛暟(闃堝€? 绫诲埆, 璺緞绛?
 * logger.hpp           - TensorRT鏃ュ織璁板綍鍣?
 * types.hpp            - 鏁版嵁缁撴瀯瀹氫箟(Detection)
 * preprocessor.hpp     - 鍥惧儚棰勫鐞?Letterbox, 褰掍竴鍖? CHW杞崲)
 * postprocessor.hpp    - 鍚庡鐞?瑙ｇ爜, NMS, 缁樺埗缁撴灉)
 * yolo_trt_engine.hpp  - TensorRT鎺ㄧ悊寮曟搸(GPU鍐呭瓨绠＄悊, 鎺ㄧ悊)
 * inference_runner.hpp - 鎺ㄧ悊鎵ц鍣?鍥剧墖/瑙嗛/鎽勫儚澶?鏂囦欢澶?
 * yolo11_2.cpp         - 涓诲叆鍙?鍛戒护琛岃В鏋? 璋冨害鎵ц)
 *
 * 浣跨敤鏂瑰紡:
 * yolo11_2.exe image.jpg              鎺ㄧ悊鍗曞紶鍥剧墖
 * yolo11_2.exe video.mp4              鎺ㄧ悊瑙嗛鏂囦欢
 * yolo11_2.exe camera                 瀹炴椂鎽勫儚澶存帹鐞?
 * yolo11_2.exe images <folder_path>   鎵归噺澶勭悊鏂囦欢澶?
 */

#include <iostream>
#include <string>
#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

#include "config.hpp"
#include "types.hpp"
#include "preprocessor.hpp"
#include "postprocessor.hpp"
#include "yolo_trt_engine.hpp"
#include "inference_runner.hpp"

namespace fs = std::filesystem;

// 鎵撳嵃绋嬪簭浣跨敤甯姪淇℃伅
void printUsage(const char* programName) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "          浣跨敤甯姪" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\n鏀寔鐨勮緭鍏ユā寮?" << std::endl;
    std::cout << "  1. 鍗曞紶鍥剧墖鎺ㄧ悊" << std::endl;
    std::cout << "     " << programName << " image.jpg" << std::endl;
    std::cout << "\n  2. 瑙嗛鏂囦欢鎺ㄧ悊" << std::endl;
    std::cout << "     " << programName << " video.mp4" << std::endl;
    std::cout << "\n  3. 瀹炴椂鎽勫儚澶存帹鐞? << std::endl;
    std::cout << "     " << programName << " camera" << std::endl;
    std::cout << "\n  4. 鎵归噺澶勭悊鏂囦欢澶? << std::endl;
    std::cout << "     " << programName << " images <folder_path>" << std::endl;
    std::cout << "\n鏀寔鐨勫浘鐗囨牸寮? .jpg, .jpeg, .png, .bmp" << std::endl;
    std::cout << "鏀寔鐨勮棰戞牸寮? .mp4, .avi, .mov, .mkv" << std::endl;
    std::cout << "\n鎸塃SC閿彲閫€鍑烘帹鐞嗙獥鍙? << std::endl;
}

// 鍒ゆ柇鏂囦欢鎵╁睍鍚嶆槸鍚﹀尮閰嶇洰鏍囬泦鍚?
bool hasExtension(const std::string& filePath, const std::vector<std::string>& extensions) {
    std::string ext = fs::path(filePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return std::find(extensions.begin(), extensions.end(), ext) != extensions.end();
}

// 绋嬪簭涓诲叆鍙?
int main(int argc, char** argv) {
    // 灏嗘帶鍒跺彴缂栫爜璁句负 UTF-8锛堣В鍐?Windows 涓嬩腑鏂囦贡鐮侊級
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // 鍚姩淇℃伅
    std::cout << "========================================" << std::endl;
    std::cout << "   YOLO11 TensorRT C++ 鎺ㄧ悊绯荤粺" << std::endl;
    std::cout << "   PPE 涓汉闃叉姢瑁呭妫€娴? << std::endl;
    std::cout << "========================================" << std::endl;

    // 鍒涘缓杈撳嚭鐩綍
    fs::create_directories(Config::OUTPUT_DIR);
    std::cout << "[Main] Output directory: " << Config::OUTPUT_DIR << std::endl;

    // 鍔犺浇妯″瀷
    std::cout << "[Main] Loading model: " << Config::MODEL_PATH << std::endl;
    InferenceRunner runner(Config::MODEL_PATH);
    std::cout << "[Main] Model loaded successfully!" << std::endl;

    // 鍛戒护琛屽弬鏁拌В鏋?
    if (argc < 2) {
        printUsage(argv[0]);
        return 0;
    }

    std::string mode = argv[1];

    // 妯″紡璋冨害
    try {
        if (mode == "camera") {
            std::cout << "\n[Main] Mode: Camera" << std::endl;
            runner.runCamera();

        } else if (mode == "images" && argc >= 3) {
            std::cout << "\n[Main] Mode: Folder batch - " << argv[2] << std::endl;
            runner.runFolder(argv[2]);

        } else {
            std::string path = argv[1];
            std::cout << "\n[Main] Mode: Single file - " << path << std::endl;

            std::vector<std::string> imageExts = {".jpg", ".jpeg", ".png", ".bmp"};
            std::vector<std::string> videoExts = {".mp4", ".avi", ".mov", ".mkv"};

            if (hasExtension(path, imageExts)) {
                runner.runImage(path);
            } else if (hasExtension(path, videoExts)) {
                runner.runVideo(path);
            } else {
                std::cerr << "[Main] Error: Unsupported format: "
                          << fs::path(path).extension().string() << std::endl;
                printUsage(argv[0]);
                return 1;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "\n[Main] Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\n[Main] Done!" << std::endl;
    return 0;
}
