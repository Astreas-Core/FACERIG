add_library(Security STATIC
    SecureStorage.cpp
)

target_include_directories(Security PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Crypt32 is required for DPAPI
target_link_libraries(Security PUBLIC crypt32)
