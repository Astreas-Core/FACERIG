#include "LivenessDetector.h"
#include <cmath>
#include <numeric>

namespace FaceID {

LivenessDetector::LivenessDetector() : isLive_(false), isSpoof_(false) {}

LivenessDetector::~LivenessDetector() {}

void LivenessDetector::Reset() {
    poseRatios_.clear();
    isLive_ = false;
    isSpoof_ = false;
}

bool LivenessDetector::Update(const Face& face) {
    if (isLive_) return true; // Already verified
    
    if (face.landmarks.size() < 5) return false;

    // Calculate a 3D parallax ratio: Nose to Right Eye / Eye Distance
    cv::Point2f rightEye = face.landmarks[0];
    cv::Point2f leftEye = face.landmarks[1];
    cv::Point2f nose = face.landmarks[2];

    float eyeDist = std::abs(rightEye.x - leftEye.x);
    if (eyeDist < 20) return false; // Too far

    float noseToRightEye = std::abs(nose.x - rightEye.x);
    float ratio = noseToRightEye / eyeDist;

    poseRatios_.push_back(ratio);
    if (poseRatios_.size() > HISTORY_SIZE) {
        poseRatios_.pop_front();
    }

    if (poseRatios_.size() == HISTORY_SIZE) {
        // Calculate variance
        float sum = std::accumulate(poseRatios_.begin(), poseRatios_.end(), 0.0f);
        float mean = sum / HISTORY_SIZE;
        float sq_sum = std::inner_product(poseRatios_.begin(), poseRatios_.end(), poseRatios_.begin(), 0.0f);
        float variance = (sq_sum / HISTORY_SIZE) - (mean * mean);

        // A static photo has near zero variance (< 0.0001).
        // A real face breathing/moving slightly has higher variance.
        // If variance is completely zero over 15 frames, it's a static image (spoof).
        if (variance > 0.0002f) {
            isLive_ = true;
        } else {
            // Keep checking. If it stays rigid for a long time, flag spoof.
            // But since this is a simple demo, we will just say it's live once it hits the threshold.
            // If they are perfectly still, it will just make them wait until they move slightly.
        }
    }

    return isLive_;
}

float LivenessDetector::GetProgress() const {
    if (isLive_) return 1.0f;
    return static_cast<float>(poseRatios_.size()) / HISTORY_SIZE;
}

} // namespace FaceID
