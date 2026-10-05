add_library(FaceIDProvider SHARED
    Dll.cpp
    Provider.cpp
    Credential.cpp
    FaceIDProvider.def
)

target_link_libraries(FaceIDProvider PRIVATE
    shlwapi
    credui
    secur32
    Security
    Crypt32
)

# For a real provider, we would link FaceEngine and OpenCV here,
# but for the stub we just build the basic COM DLL.
