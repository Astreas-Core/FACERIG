#include "EnrollmentManager.h"
#include "../Security/SecureStorage.h"
#include <iostream>

namespace FaceID {

EnrollmentManager::EnrollmentManager(FaceRecognizer& recognizer) 
    : recognizer_(recognizer), currentState_(EnrollmentState::CAPTURING), currentStage_(EnrollmentStage::STRAIGHT_1), captureFramesToWait_(0) {}

EnrollmentManager::~EnrollmentManager() {}

void EnrollmentManager::StartEnrollment() {
    currentState_ = EnrollmentState::CAPTURING;
    currentStage_ = EnrollmentStage::STRAIGHT_1;
    collectedFeatures_.clear();
    captureFramesToWait_ = 10; // wait a few frames before capturing first frame
}

bool EnrollmentManager::ProcessFrame(const cv::Mat& frame, const Face& face, std::string& outInstruction) {
    if (currentState_ == EnrollmentState::DONE) {
        outInstruction = "Enrollment Complete!";
        return false;
    }

    // Ensure the face is somewhat straight and centered
    if (face.landmarks.size() < 5) {
        outInstruction = "Face landmarks not clear";
        return false;
    }

    if (captureFramesToWait_ > 0) {
        captureFramesToWait_--;
        outInstruction = cv::format("Capturing... (%d/%d)", collectedFeatures_.size(), MAX_SAMPLES);
        return false;
    }

    // Pose estimation using landmarks
    cv::Point2f rightEye = face.landmarks[0]; // Image left
    cv::Point2f leftEye = face.landmarks[1]; // Image right
    cv::Point2f nose = face.landmarks[2];
    cv::Point2f rightMouth = face.landmarks[3];
    cv::Point2f leftMouth = face.landmarks[4];
    
    float eyeDist = std::abs(rightEye.x - leftEye.x);
    if (eyeDist < 30) {
        outInstruction = "Please move closer to the camera";
        return false;
    }

    float yawRatio = (nose.x - rightEye.x) / eyeDist;
    float mouthY = (rightMouth.y + leftMouth.y) / 2.0f;
    float eyeY = (rightEye.y + leftEye.y) / 2.0f;
    float faceHeight = mouthY - eyeY;
    
    if (faceHeight < 10) return false;
    
    float noseYRatio = (nose.y - eyeY) / faceHeight;
    
    int count = collectedFeatures_.size();
    if (count < 6) currentStage_ = EnrollmentStage::STRAIGHT_1;
    else if (count < 11) currentStage_ = EnrollmentStage::LEFT;
    else if (count < 16) currentStage_ = EnrollmentStage::RIGHT;
    else if (count < 20) currentStage_ = EnrollmentStage::UP;
    else if (count < 24) currentStage_ = EnrollmentStage::DOWN;
    else currentStage_ = EnrollmentStage::STRAIGHT_2;

    bool poseValid = false;
    switch(currentStage_) {
        case EnrollmentStage::STRAIGHT_1:
        case EnrollmentStage::STRAIGHT_2:
            outInstruction = "Look straight ahead";
            if (std::abs(yawRatio - 0.5f) < 0.25f && std::abs(noseYRatio - 0.55f) < 0.2f) poseValid = true;
            else outInstruction = "Center your head";
            break;
        case EnrollmentStage::LEFT:
            outInstruction = "Slowly turn your head LEFT";
            if (yawRatio < 0.35f) poseValid = true;
            break;
        case EnrollmentStage::RIGHT:
            outInstruction = "Slowly turn your head RIGHT";
            if (yawRatio > 0.65f) poseValid = true;
            break;
        case EnrollmentStage::UP:
            outInstruction = "Tilt your head UP";
            if (noseYRatio < 0.45f) poseValid = true;
            break;
        case EnrollmentStage::DOWN:
            outInstruction = "Tilt your head DOWN";
            if (noseYRatio > 0.65f) poseValid = true;
            break;
    }

    if (!poseValid) return false;

    // Valid frame! Extract features.
    cv::Mat features = recognizer_.ExtractFeatures(frame, const_cast<Face&>(face));
    if (!features.empty()) {
        collectedFeatures_.push_back(features.clone());
        outInstruction = cv::format("Capturing... (%d/%d)", collectedFeatures_.size(), MAX_SAMPLES);
        
        if (collectedFeatures_.size() >= MAX_SAMPLES) {
            currentState_ = EnrollmentState::DONE;
        } else {
            captureFramesToWait_ = 4; // small delay between captures
        }
        return true;
    }
    
    return false;
}

bool EnrollmentManager::SaveProfile(const std::string& username) {
    if (collectedFeatures_.empty()) return false;

    // Average the features for a robust single template
    cv::Mat avgFeature = cv::Mat::zeros(collectedFeatures_[0].size(), collectedFeatures_[0].type());
    for (const auto& feat : collectedFeatures_) {
        avgFeature += feat;
    }
    avgFeature /= (float)collectedFeatures_.size();
    
    // Convert Mat to byte array
    std::vector<unsigned char> plainText(avgFeature.total() * avgFeature.elemSize());
    memcpy(plainText.data(), avgFeature.data, plainText.size());

    // Securely save
    std::string filename = "profile_" + username + ".bin";
    return Security::SecureStorage::SaveMachineEncryptedFile(filename, plainText);
}

} // namespace FaceID
