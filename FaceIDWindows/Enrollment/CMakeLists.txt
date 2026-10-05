add_library(Enrollment STATIC
    EnrollmentManager.cpp
)

target_include_directories(Enrollment PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Enrollment PUBLIC 
    FaceEngine
    Security
    ${OpenCV_LIBS}
)
