#include <opencv2/opencv.hpp>
#include <iostream>
#include <exception>
#include <windows.h>
#include "FaceEngine/FaceDetector.h"
#include "FaceEngine/FaceRecognizer.h"
#include "FaceEngine/LivenessDetector.h"
#include "Enrollment/EnrollmentManager.h"
#include "Security/SecureStorage.h"

static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string base64_encode(const unsigned char* bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for(i = 0; (i <4) ; i++) ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        for(j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; (j < i + 1); j++) ret += base64_chars[char_array_4[j]];
        while((i++ < 3)) ret += '=';
    }
    return ret;
}

void log_to_file(const std::string& msg) {
    FILE* f;
    if (fopen_s(&f, "C:\\Users\\DHANVESH\\Documents\\PROJECTS\\facereg\\FaceIDWindows\\faceid_debug.log", "a") == 0) {
        fprintf(f, "%s\n", msg.c_str());
        fclose(f);
    }
}
void log_app(const std::string& msg) {
    FILE* f;
    if (fopen_s(&f, "C:\\Users\\DHANVESH\\Documents\\PROJECTS\\facereg\\FaceIDWindows\\faceid_debug.log", "a") == 0) {
        fprintf(f, "%s\n", msg.c_str());
        fclose(f);
    }
}

int main(int argc, char* argv[]) {
    log_to_file("FaceIDApp started.");
    bool forceEnroll = false;
    bool forceTest = false;
    bool headless = false;
    bool requireLiveness = true;
    bool streamUI = false;
    float strictnessThreshold = 0.363f; // Default SFace threshold
    std::string passwordToSave = "";

    std::string enrollSlot = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--enroll" && i + 1 < argc) {
            forceEnroll = true;
            enrollSlot = argv[++i];
        }
        if (arg == "--test") forceTest = true;
        if (arg == "--headless") headless = true;
        if (arg == "--stream") streamUI = true;
        if (arg == "--no-liveness") requireLiveness = false;
        if (arg == "--delete-profile" && i + 2 < argc) {
            std::string slot = argv[++i];
            std::string tryPass = argv[++i];
            std::vector<unsigned char> pwData;
            if (FaceID::Security::SecureStorage::LoadMachineEncryptedFile("password_DefaultUser.bin", pwData)) {
                std::wstring password(reinterpret_cast<wchar_t*>(pwData.data()), pwData.size() / sizeof(wchar_t));
                std::string actualPass(password.begin(), password.end());
                if (actualPass.find(tryPass) == 0) { // password matched
                    std::string profileFile = "profile_" + slot + ".bin";
                    DeleteFileA(profileFile.c_str());
                    return 0; // success
                }
            }
            return 1; // fail
        }
        if (arg == "--verify-password" && i + 1 < argc) {
            std::string tryPass = argv[++i];
            std::vector<unsigned char> pwData;
            if (FaceID::Security::SecureStorage::LoadMachineEncryptedFile("password_DefaultUser.bin", pwData)) {
                std::wstring password(reinterpret_cast<wchar_t*>(pwData.data()), pwData.size() / sizeof(wchar_t));
                std::string actualPass(password.begin(), password.end());
                if (actualPass.find(tryPass) == 0) {
                    return 0; // success
                }
            }
            return 1; // fail
        }
        if (arg == "--strictness" && i + 1 < argc) {
            strictnessThreshold = std::stof(argv[++i]);
        }
        if (arg == "--save-password" && i + 1 < argc) {
            passwordToSave = argv[++i];
        }
    }

    if (!passwordToSave.empty()) {
        std::vector<unsigned char> data(passwordToSave.begin(), passwordToSave.end());
        // Append null terminator for WCHAR conversion later
        data.push_back(0); 
        data.push_back(0);
        
        // Use wstring to ensure UTF-16 LE required by CredPackAuthenticationBufferW
        std::wstring wpass(passwordToSave.begin(), passwordToSave.end());
        std::vector<unsigned char> wdata(reinterpret_cast<unsigned char*>(&wpass[0]), reinterpret_cast<unsigned char*>(&wpass[0]) + (wpass.size() * sizeof(wchar_t)) + sizeof(wchar_t));

        if (FaceID::Security::SecureStorage::SaveMachineEncryptedFile("password_DefaultUser.bin", wdata)) {
            return 0; // Success
        }
        return 1; // Failed
    }

    try {
        log_app("Initializing models...");
        
        FaceID::FaceDetector detector;
        if (!detector.Initialize("Models/face_detection_yunet_2023mar.onnx")) {
            log_app("Failed to load FaceDetector model!");
            return 1;
        }

        FaceID::FaceRecognizer recognizer;
        if (!recognizer.Initialize("Models/face_recognition_sface_2021dec.onnx")) {
            log_app("Failed to load FaceRecognizer model!");
            return 1;
        }
        
        FaceID::LivenessDetector liveness;

        std::map<std::string, cv::Mat> storedFeatures;
        
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA("profile_*.bin", &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::string filename = findData.cFileName;
                if (filename.length() > 12) {
                    std::string slot = filename.substr(8, filename.length() - 12);
                    std::vector<unsigned char> profileData;
                    if (FaceID::Security::SecureStorage::LoadMachineEncryptedFile(filename, profileData) && !profileData.empty()) {
                        storedFeatures[slot] = cv::Mat(1, 128, CV_32FC1, profileData.data()).clone();
                    }
                }
            } while (FindNextFileA(hFind, &findData));
            FindClose(hFind);
        }

        FaceID::EnrollmentManager enrollment(recognizer);
        if (forceEnroll && !enrollSlot.empty()) {
            enrollment.StartEnrollment();
        }

        log_app("Opening camera...");
        cv::VideoCapture cap(0);
        if (!cap.isOpened()) {
            log_app("Error: Could not open the webcam.");
            return 1;
        }
        
        cv::Mat frame;
        cap >> frame;
        if (!frame.empty()) {
            detector.SetInputSize(cv::Size(frame.cols, frame.rows));
        } else {
            log_app("Error: First frame was empty.");
        }

        const std::string windowName = "FaceID Windows";
        if (!headless) {
            cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
        }

        int headlessFrameCount = 0;
        const int MAX_HEADLESS_FRAMES = streamUI ? 3000 : 900; // allow more time for headless auth
        bool successExit = false;

        while (true) {
            if (headless && !streamUI) {
                headlessFrameCount++;
                if (headlessFrameCount > MAX_HEADLESS_FRAMES) {
                    log_app("Timeout after MAX_HEADLESS_FRAMES");
                    return 1; // Failed to authenticate in time
                }
            }
            cap >> frame;
            if (frame.empty()) {
                log_app("Warning: cap >> frame returned empty frame, retrying...");
                Sleep(33); // Wait ~1 frame at 30fps
                continue;
            }
            cv::flip(frame, frame, 1);
            
            std::vector<FaceID::Face> faces = detector.Detect(frame);
            std::string uiText = "";
            cv::Scalar textColor(255, 255, 255);

            if (forceEnroll && !enrollSlot.empty()) {
                if (faces.size() == 1) {
                    bool captured = enrollment.ProcessFrame(frame, faces[0], uiText);
                    if (captured) {
                        textColor = cv::Scalar(0, 255, 0);
                    }
                    if (enrollment.GetState() == FaceID::EnrollmentState::DONE) {
                        if (streamUI) {
                            std::cout << "STATE:PROGRESS:" << static_cast<int>(enrollment.GetState()) << "," << enrollment.GetProgress() << "," << enrollment.GetMaxSamples() << "\n";
                            std::cout << "STATE:UI:Creating your Face ID profile...\n" << std::flush;
                        }

                        if (enrollment.SaveProfile(enrollSlot)) {
                            std::string profileFile = "profile_" + enrollSlot + ".bin";
                            std::vector<unsigned char> profileData;
                            FaceID::Security::SecureStorage::LoadMachineEncryptedFile(profileFile, profileData);
                            storedFeatures[enrollSlot] = cv::Mat(1, 128, CV_32FC1, profileData.data()).clone();
                            liveness.Reset();
                            
                            if (streamUI) {
                                std::cout << "STATE:DONE\n" << std::flush;
                            } else if (!headless) {
                                cv::putText(frame, "Enrollment Saved! Closing...", cv::Point(22, 42), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);
                                cv::putText(frame, "Enrollment Saved! Closing...", cv::Point(20, 40), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 0), 2);
                                cv::imshow(windowName, frame);
                                cv::waitKey(1500);
                            }
                            successExit = true;
                            break;
                        } else {
                            uiText = "Failed to save profile!";
                            textColor = cv::Scalar(0, 0, 255);
                        }
                    }
                } else if (faces.empty()) {
                    uiText = "No face detected.";
                    textColor = cv::Scalar(0, 0, 255);
                } else {
                    uiText = "Multiple faces detected. Please be alone.";
                    textColor = cv::Scalar(0, 0, 255);
                }
            } else {
                if (storedFeatures.empty()) {
                    uiText = "No faces enrolled.";
                    textColor = cv::Scalar(0, 0, 255);
                } else if (faces.size() == 1) {
                    if (!requireLiveness || liveness.Update(faces[0])) {
                        cv::Mat liveFeature = recognizer.ExtractFeatures(frame, faces[0]);
                        if (!liveFeature.empty()) {
                            float bestScore = -1.0f;
                            std::string bestMatch = "";
                            for (const auto& pair : storedFeatures) {
                                float sim = recognizer.Compare(liveFeature, pair.second);
                                if (sim > bestScore) {
                                    bestScore = sim;
                                    bestMatch = pair.first;
                                }
                            }
                            
                            if (recognizer.IsSamePerson(bestScore, strictnessThreshold)) {
                                uiText = cv::format("AUTHENTICATED: %s", bestMatch.c_str());
                                textColor = cv::Scalar(0, 255, 0);
                                if (headless) {
                                    std::cout << "MATCH:" << bestMatch << std::endl;
                                    return 0;
                                }
                            } else {
                                uiText = cv::format("UNKNOWN PERSON (%.2f)", bestScore);
                                textColor = cv::Scalar(0, 0, 255);
                            }
                        }
                    } else {
                        uiText = "Checking Liveness... Please move your head slightly.";
                        textColor = cv::Scalar(0, 255, 255);
                    }
                } else {
                    uiText = "Looking for Face...";
                    liveness.Reset();
                }
            }
            
            if (streamUI) {
                std::cout << "STATE:UI:" << uiText << "\n";
                if (faces.size() == 1) {
                    std::cout << "STATE:FACE:" << faces[0].bbox.x << "," << faces[0].bbox.y << "," << faces[0].bbox.width << "," << faces[0].bbox.height << "\n";
                    if (forceEnroll) {
                        std::cout << "STATE:PROGRESS:" << static_cast<int>(enrollment.GetState()) << "," << enrollment.GetProgress() << "," << enrollment.GetMaxSamples() << "\n";
                    }
                } else {
                    std::cout << "STATE:NOFACE\n";
                }
                
                cv::Mat smallFrame;
                cv::resize(frame, smallFrame, cv::Size(640, 480));
                std::vector<uchar> buf;
                std::vector<int> compression_params;
                compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
                compression_params.push_back(60);
                cv::imencode(".jpg", smallFrame, buf, compression_params);
                std::cout << "FRAME:" << base64_encode(buf.data(), buf.size()) << "\n" << std::flush;
            } else if (!headless) {
                // Draw UI (with a black shadow so it's readable on bright backgrounds)
                cv::putText(frame, uiText, cv::Point(22, 42), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);
                cv::putText(frame, uiText, cv::Point(20, 40), cv::FONT_HERSHEY_SIMPLEX, 0.8, textColor, 2);
                
                for (auto& face : faces) {
                    cv::rectangle(frame, face.bbox, cv::Scalar(255, 0, 0), 2);
                    for (const auto& pt : face.landmarks) {
                        cv::circle(frame, pt, 2, cv::Scalar(0, 0, 255), -1);
                    }
                }
                
                cv::imshow(windowName, frame);
                char key = (char)cv::waitKey(30);
                if (key == 'q' || key == 'Q' || key == 27) break;
            }
        }
        
        cap.release();
        cv::destroyAllWindows();
        
        return successExit ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
}
