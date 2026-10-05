#pragma once

#include "FaceDetector.h"
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <string>
#include <vector>

namespace FaceID {

class FaceRecognizer {
public:
    FaceRecognizer();
    ~FaceRecognizer();

    // Initialize the recognizer with the SFace ONNX model
    bool Initialize(const std::string& modelPath);

    // Extract facial features (embedding) from a detected face
    // Returns a 1D feature vector (Mat)
    cv::Mat ExtractFeatures(const cv::Mat& image, Face& face);

    // Compare two feature vectors and return a similarity score
    // Higher score means higher similarity
    float Compare(const cv::Mat& feature1, const cv::Mat& feature2);

    // Check if the similarity meets the threshold for authentication
    bool IsSamePerson(float similarity, float threshold = 0.363f); // Cosine similarity default for SFace

private:
    cv::Ptr<cv::FaceRecognizerSF> recognizer_;
};

} // namespace FaceID
