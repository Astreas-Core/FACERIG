cmake_minimum_required(VERSION 3.20)
project(FaceIDWindows VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenCV
find_package(OpenCV REQUIRED)
message(STATUS "Found OpenCV: ${OpenCV_VERSION}")

# Find ONNX Runtime (vcpkg port provides unofficially supported target)
find_package(onnxruntime CONFIG REQUIRED)

add_subdirectory(FaceEngine)
add_subdirectory(Security)
add_subdirectory(Enrollment)
add_subdirectory(CredentialProvider)
add_subdirectory(App)
add_executable(TestLogon test_logon.cpp)