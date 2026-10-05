#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <string>
#include <vector>

namespace FaceID {

struct Face {
    cv::Rect2f bbox;
    std::vector<cv::Point2f> landmarks;
    float confidence;
    cv::Mat alignedFace; // Optional, populated later
};

class FaceDetector {
public:
    FaceDetector();
    ~FaceDetector();

    // Initialize the detector with the YuNet ONNX model
    bool Initialize(const std::string& modelPath, const cv::Size& inputSize = cv::Size(320, 320));

    // Detect faces in an image
    // Returns a list of detected faces
    std::vector<Face> Detect(const cv::Mat& image);

    // Set the input size dynamically based on the frame size
    void SetInputSize(const cv::Size& size);

    // Set thresholds
    void SetScoreThreshold(float score);
    void SetNMSThreshold(float nms);

private:
    cv::Ptr<cv::FaceDetectorYN> detector_;
    float scoreThreshold_;
    float nmsThreshold_;
};

} // namespace FaceID
