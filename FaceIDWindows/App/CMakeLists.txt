add_executable(FaceIDApp main.cpp)

target_link_libraries(FaceIDApp PRIVATE 
    FaceEngine
    Enrollment
    Security
    ${OpenCV_LIBS}
    onnxruntime::onnxruntime
)

target_include_directories(FaceIDApp PRIVATE ${CMAKE_SOURCE_DIR})

# For Windows, sometimes you need to copy DLLs to the executable folder,
# but vcpkg usually handles this automatically for CMake builds.
