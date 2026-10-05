#include "FaceRecognizer.h"
#include <iostream>

namespace FaceID {

FaceRecognizer::FaceRecognizer() {}

FaceRecognizer::~FaceRecognizer() {}

bool FaceRecognizer::Initialize(const std::string& modelPath) {
    try {
        // Create the SFace recognizer
        recognizer_ = cv::FaceRecognizerSF::create(modelPath, "");
        return recognizer_ != nullptr;
    } catch (const cv::Exception& e) {
        std::cerr << "FaceRecognizer Init Error: " << e.what() << "\n";
        return false;
    }
}

cv::Mat FaceRecognizer::ExtractFeatures(const cv::Mat& image, Face& face) {
    cv::Mat feature;
    if (!recognizer_ || image.empty() || face.landmarks.empty()) return feature;

    try {
        // We need to format the face detection result back to OpenCV's format for alignment
        // The format is [x, y, w, h, x_re, y_re, x_le, y_le, x_nt, y_nt, x_rcm, y_rcm, x_lcm, y_lcm, score]
        cv::Mat faceDet = cv::Mat::zeros(1, 15, CV_32FC1);
        faceDet.at<float>(0, 0) = face.bbox.x;
        faceDet.at<float>(0, 1) = face.bbox.y;
        faceDet.at<float>(0, 2) = face.bbox.width;
        faceDet.at<float>(0, 3) = face.bbox.height;
        
        for (int i = 0; i < 5; ++i) {
            faceDet.at<float>(0, 4 + i * 2) = face.landmarks[i].x;
            faceDet.at<float>(0, 5 + i * 2) = face.landmarks[i].y;
        }
        faceDet.at<float>(0, 14) = face.confidence;

        // Align the face
        recognizer_->alignCrop(image, faceDet, face.alignedFace);
        
        // Extract embedding
        recognizer_->feature(face.alignedFace, feature);
        
        return feature;
    } catch (const cv::Exception& e) {
        std::cerr << "Feature extraction error: " << e.what() << "\n";
        return cv::Mat();
    }
}

float FaceRecognizer::Compare(const cv::Mat& feature1, const cv::Mat& feature2) {
    if (!recognizer_ || feature1.empty() || feature2.empty()) return 0.0f;
    
    // Use cosine similarity (FR_COSINE)
    return recognizer_->match(feature1, feature2, cv::FaceRecognizerSF::FR_COSINE);
}

bool FaceRecognizer::IsSamePerson(float similarity, float threshold) {
    return similarity >= threshold;
}

} // namespace FaceID
