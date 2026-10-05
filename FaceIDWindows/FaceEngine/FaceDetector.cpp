#include "FaceDetector.h"
#include <iostream>

namespace FaceID {

FaceDetector::FaceDetector() : scoreThreshold_(0.9f), nmsThreshold_(0.3f) {}

FaceDetector::~FaceDetector() {}

bool FaceDetector::Initialize(const std::string& modelPath, const cv::Size& inputSize) {
    try {
        // Create the YuNet face detector
        // Using CPU backend for maximum compatibility, can be changed to CUDA/OpenCL later
        detector_ = cv::FaceDetectorYN::create(
            modelPath,
            "", // Config file not needed for YuNet
            inputSize,
            scoreThreshold_,
            nmsThreshold_,
            5000 // top_k
        );
        return detector_ != nullptr;
    } catch (const cv::Exception& e) {
        std::cerr << "FaceDetector Init Error: " << e.what() << "\n";
        return false;
    }
}

void FaceDetector::SetInputSize(const cv::Size& size) {
    if (detector_) {
        detector_->setInputSize(size);
    }
}

void FaceDetector::SetScoreThreshold(float score) {
    scoreThreshold_ = score;
    if (detector_) detector_->setScoreThreshold(score);
}

void FaceDetector::SetNMSThreshold(float nms) {
    nmsThreshold_ = nms;
    if (detector_) detector_->setNMSThreshold(nms);
}

std::vector<Face> FaceDetector::Detect(const cv::Mat& image) {
    std::vector<Face> result;
    if (!detector_ || image.empty()) return result;

    cv::Mat faces;
    // YuNet expects a BGR image
    detector_->detect(image, faces);

    // faces is a CV_32FC1 matrix of shape (num_faces, 15)
    // format: [x, y, w, h, x_re, y_re, x_le, y_le, x_nt, y_nt, x_rcm, y_rcm, x_lcm, y_lcm, score]
    if (!faces.empty()) {
        for (int i = 0; i < faces.rows; i++) {
            Face f;
            f.bbox = cv::Rect2f(faces.at<float>(i, 0), faces.at<float>(i, 1), faces.at<float>(i, 2), faces.at<float>(i, 3));
            
            // 5 landmarks (Right eye, Left eye, Nose tip, Right mouth corner, Left mouth corner)
            f.landmarks.push_back(cv::Point2f(faces.at<float>(i, 4), faces.at<float>(i, 5)));
            f.landmarks.push_back(cv::Point2f(faces.at<float>(i, 6), faces.at<float>(i, 7)));
            f.landmarks.push_back(cv::Point2f(faces.at<float>(i, 8), faces.at<float>(i, 9)));
            f.landmarks.push_back(cv::Point2f(faces.at<float>(i, 10), faces.at<float>(i, 11)));
            f.landmarks.push_back(cv::Point2f(faces.at<float>(i, 12), faces.at<float>(i, 13)));
            
            f.confidence = faces.at<float>(i, 14);
            
            result.push_back(f);
        }
    }

    return result;
}

} // namespace FaceID
