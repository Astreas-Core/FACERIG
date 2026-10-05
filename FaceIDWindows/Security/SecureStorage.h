#pragma once

#include <vector>
#include <string>
#include <windows.h>
#include <wincrypt.h>

namespace FaceID {
namespace Security {

class SecureStorage {
public:
    // Encrypts data using Windows DPAPI (CryptProtectData)
    // The data is tied to the current Windows user's login credential.
    static bool EncryptData(const std::vector<unsigned char>& plainText, std::vector<unsigned char>& cipherText);

    // Decrypts data using Windows DPAPI (CryptUnprotectData)
    static bool DecryptData(const std::vector<unsigned char>& cipherText, std::vector<unsigned char>& plainText);

    // Save encrypted data to a file
    static bool SaveEncryptedFile(const std::string& filePath, const std::vector<unsigned char>& plainText);

    // Load and decrypt data from a file
    static bool LoadEncryptedFile(const std::string& filePath, std::vector<unsigned char>& plainText);
    // Machine-level encryption (Can be decrypted by SYSTEM / LogonUI)
    static bool SaveMachineEncryptedFile(const std::string& filename, const std::vector<unsigned char>& data);
    static bool LoadMachineEncryptedFile(const std::string& filename, std::vector<unsigned char>& data);
};

} // namespace Security
} // namespace FaceID
