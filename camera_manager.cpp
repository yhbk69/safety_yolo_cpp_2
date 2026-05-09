/**
 * @file camera_manager.cpp
 * @brief 摄像头生命周期管理模块实现
 */

#include "camera_manager.hpp"

CameraManager::CameraManager() {
}

void CameraManager::add(int cameraId, QThread* thread, InferenceWorker* worker) {
    if (!thread || !worker) return;
    entries_[cameraId] = {thread, worker};
}

InferenceWorker* CameraManager::worker(int cameraId) const {
    auto it = entries_.find(cameraId);
    return it != entries_.end() ? it->worker : nullptr;
}

bool CameraManager::contains(int cameraId) const {
    return entries_.contains(cameraId);
}

QList<int> CameraManager::cameraIds() const {
    return entries_.keys();
}

void CameraManager::stop(int cameraId) {
    auto it = entries_.find(cameraId);
    if (it == entries_.end()) return;

    if (it->worker) it->worker->stop();

    if (it->thread) {
        if (!it->thread->wait(2000)) {
            it->thread->terminate();
            it->thread->wait(500);
        }
    }

    entries_.erase(it);
}

void CameraManager::stopAll() {
    auto ids = entries_.keys();
    for (int id : ids) {
        stop(id);
    }
}

void CameraManager::setBatchInference(bool enabled) {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->worker) it->worker->setBatchInference(enabled);
    }
}
