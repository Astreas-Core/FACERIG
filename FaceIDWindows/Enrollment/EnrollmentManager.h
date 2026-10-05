#pragma once

#include "../FaceEngine/FaceDetector.h"
#include "../FaceEngine/FaceRecognizer.h"
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>

namespace FaceID {

enum class EnrollmentState {
    CAPTURING,
    DONE
};

enum class EnrollmentStage {
    STRAIGHT_1,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    STRAIGHT_2
};

class EnrollmentManager {
public:
    EnrollmentManager(FaceRecognizer& recognizer);
    ~EnrollmentManager();

    void StartEnrollment();
    bool ProcessFrame(const cv::Mat& frame, const Face& face, std::string& outInstruction);
    EnrollmentState GetState() const { return currentState_; }
    int GetProgress() const { return static_cast<int>(collectedFeatures_.size()); }
    int GetMaxSamples() const { return MAX_SAMPLES; }
    bool SaveProfile(const std::string& username);

private:
    FaceRecognizer& recognizer_;
    EnrollmentState currentState_;
    EnrollmentStage currentStage_;
    std::vector<cv::Mat> collectedFeatures_;
    
    int captureFramesToWait_;
    const int MAX_SAMPLES = 30;
    
    bool EncryptAndSave(const std::vector<cv::Mat>& features, const std::string& path);
};

} // namespace FaceID
