#!/bin/bash
# ============================================================
# YOLO11 PPE Detection - Linux Deployment Packaging Script
# Collects Release build + all dependency .so files into dist/
# Target PC needs: NVIDIA GPU + latest driver (CUDA requirement)
# ============================================================

set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT/build"
DIST_DIR="$ROOT/dist"

echo "========================================"
echo "  Packaging YOLO11 PPE Detection System"
echo "========================================"

# [1/5] Build
echo ""
echo "[1/5] Building Release..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j"$(nproc)"
echo "  Build OK"

# [2/5] Create dist directory
echo ""
echo "[2/5] Creating output directory..."
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"/{model,output}

# [3/5] Copy executable
echo ""
echo "[3/5] Copying executable..."
if [ -f "$BUILD_DIR/yolo11_2" ]; then
    cp "$BUILD_DIR/yolo11_2" "$DIST_DIR/"
    echo "  yolo11_2"
else
    echo "  [WARNING] yolo11_2 not found in build output"
    echo "  Please build the project first, then copy the binary manually"
fi

# [4/5] Deploy Qt runtime libraries
echo ""
echo "[4/5] Deploying Qt runtime libraries..."
if command -v linuxdeployqt &>/dev/null; then
    linuxdeployqt "$DIST_DIR/yolo11_2" -bundle-non-qt-libs
    echo "  Qt deployed via linuxdeployqt"
elif command -v windeployqt &>/dev/null; then
    # Fallback for cross-compiled scenarios
    echo "  [SKIP] windeployqt not useful on Linux"
else
    echo "  [NOTE] Install linuxdeployqt for bundling, or set LD_LIBRARY_PATH"
    echo "         For now, copying essential libs..."
    # Copy known TensorRT libs
    TENSORRT_DIR="${TENSORRT_DIR:-/usr/local/tensorrt}"
    if [ -d "$TENSORRT_DIR/lib" ]; then
        cp -a "$TENSORRT_DIR/lib"/*.so* "$DIST_DIR/" 2>/dev/null || true
    fi
fi

# [5/5] Copy model file
echo ""
echo "[5/5] Copying model..."
if ls "$ROOT/model"/*.engine 2>/dev/null; then
    cp "$ROOT/model"/*.engine "$DIST_DIR/model/"
    echo "  Model file copied to dist/model/"
else
    echo "  [NOTE] No .engine model found in model/"
    echo "         Place your TensorRT engine file in dist/model/"
fi

# Generate README
echo ""
cat > "$DIST_DIR/README.txt" << 'README'
YOLO11 PPE Detection System - Standalone Package
================================================

Requirements:
  - Linux x86_64
  - NVIDIA GPU with latest driver (provides CUDA runtime)
  - TensorRT runtime libraries

How to use:
  1. Place your .engine model file in model/
  2. Run ./yolo11_2

Directory layout:
  yolo11_2          - Main executable
  model/            - TensorRT engine model (.engine)
  output/           - Detection results
  *.so*             - Runtime dependencies
README

echo ""
echo "========================================"
echo "  Packaging complete!"
echo "  Output: $DIST_DIR"
echo "========================================"
