add_library(FaceEngine STATIC
    FaceDetector.cpp
    FaceRecognizer.cpp
    LivenessDetector.cpp
)

target_include_directories(FaceEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(FaceEngine PUBLIC 
    ${OpenCV_LIBS}
)
