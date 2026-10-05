#pragma once

#include "FaceDetector.h"
#include <deque>

namespace FaceID {

class LivenessDetector {
public:
    LivenessDetector();
    ~LivenessDetector();

    // Returns true if liveness is confirmed
    // Returns false if still checking or detected spoof
    bool Update(const Face& face);
    
    // Reset the liveness state (e.g. when a new face enters the frame)
    void Reset();

    // Get the current progress/status for UI
    float GetProgress() const;
    bool IsSpoof() const { return isSpoof_; }

private:
    std::deque<float> poseRatios_;
    const size_t HISTORY_SIZE = 15; // Requires ~15 frames (0.5s) to verify
    bool isLive_;
    bool isSpoof_;
};

} // namespace FaceID
