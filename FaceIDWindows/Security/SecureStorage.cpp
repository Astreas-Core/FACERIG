#include "SecureStorage.h"
#include <fstream>
#include <iostream>

#pragma comment(lib, "Crypt32.lib")

namespace FaceID {
namespace Security {

bool SecureStorage::EncryptData(const std::vector<unsigned char>& plainText, std::vector<unsigned char>& cipherText) {
    DATA_BLOB dataIn;
    DATA_BLOB dataOut;

    dataIn.pbData = const_cast<BYTE*>(plainText.data());
    dataIn.cbData = static_cast<DWORD>(plainText.size());

    // CRYPTPROTECT_UI_FORBIDDEN prevents DPAPI from displaying a UI
    if (CryptProtectData(&dataIn, L"FaceID_Template", NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &dataOut)) {
        cipherText.assign(dataOut.pbData, dataOut.pbData + dataOut.cbData);
        LocalFree(dataOut.pbData);
        return true;
    }
    return false;
}

bool SecureStorage::DecryptData(const std::vector<unsigned char>& cipherText, std::vector<unsigned char>& plainText) {
    DATA_BLOB dataIn;
    DATA_BLOB dataOut;

    dataIn.pbData = const_cast<BYTE*>(cipherText.data());
    dataIn.cbData = static_cast<DWORD>(cipherText.size());

    if (CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, CRYPTPROTECT_UI_FORBIDDEN, &dataOut)) {
        plainText.assign(dataOut.pbData, dataOut.pbData + dataOut.cbData);
        LocalFree(dataOut.pbData);
        return true;
    }
    return false;
}

bool SecureStorage::SaveEncryptedFile(const std::string& filePath, const std::vector<unsigned char>& plainText) {
    std::vector<unsigned char> cipherText;
    if (!EncryptData(plainText, cipherText)) {
        std::cerr << "Failed to encrypt data before saving.\n";
        return false;
    }

    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) return false;

    file.write(reinterpret_cast<const char*>(cipherText.data()), cipherText.size());
    return true;
}

bool SecureStorage::LoadEncryptedFile(const std::string& filePath, std::vector<unsigned char>& plainText) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> cipherText(size);
    if (file.read(reinterpret_cast<char*>(cipherText.data()), size)) {
        return DecryptData(cipherText, plainText);
    }
    return false;
}

bool SecureStorage::SaveMachineEncryptedFile(const std::string& filePath, const std::vector<unsigned char>& plainText) {
    if (plainText.empty()) return false;

    DATA_BLOB dataIn;
    dataIn.pbData = const_cast<BYTE*>(plainText.data());
    dataIn.cbData = static_cast<DWORD>(plainText.size());

    DATA_BLOB dataOut;
    if (CryptProtectData(&dataIn, L"FaceID Machine Data", NULL, NULL, NULL, CRYPTPROTECT_LOCAL_MACHINE | CRYPTPROTECT_UI_FORBIDDEN, &dataOut)) {
        std::ofstream outFile(filePath, std::ios::binary);
        if (outFile) {
            outFile.write(reinterpret_cast<char*>(dataOut.pbData), dataOut.cbData);
            outFile.close();
            LocalFree(dataOut.pbData);
            return true;
        }
        LocalFree(dataOut.pbData);
    }
    return false;
}

bool SecureStorage::LoadMachineEncryptedFile(const std::string& filePath, std::vector<unsigned char>& plainText) {
    std::ifstream inFile(filePath, std::ios::binary | std::ios::ate);
    if (!inFile) return false;

    std::streamsize size = inFile.tellg();
    inFile.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!inFile.read(buffer.data(), size)) return false;

    DATA_BLOB dataIn;
    dataIn.pbData = reinterpret_cast<BYTE*>(buffer.data());
    dataIn.cbData = static_cast<DWORD>(size);

    DATA_BLOB dataOut;
    if (CryptUnprotectData(&dataIn, NULL, NULL, NULL, NULL, CRYPTPROTECT_LOCAL_MACHINE | CRYPTPROTECT_UI_FORBIDDEN, &dataOut)) {
        plainText.assign(dataOut.pbData, dataOut.pbData + dataOut.cbData);
        LocalFree(dataOut.pbData);
        return true;
    }
    return false;
}

} // namespace Security
} // namespace FaceID
