#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "crypt32.lib")

int main() {
    FILE* fp;
    if (fopen_s(&fp, "C:\\Users\\DHANVESH\\Documents\\PROJECTS\\facereg\\FaceIDWindows\\password_DefaultUser.bin", "rb") != 0) {
        std::cout << "Failed to open password file\n";
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::vector<BYTE> encrypted(size);
    fread(encrypted.data(), 1, size, fp);
    fclose(fp);

    DATA_BLOB in, out;
    in.pbData = encrypted.data();
    in.cbData = encrypted.size();

    if (CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &out)) {
        std::wstring pwd(reinterpret_cast<wchar_t*>(out.pbData), out.cbData / sizeof(wchar_t));
        
        while (!pwd.empty() && pwd.back() == L'\0') {
            pwd.pop_back();
        }

        std::cout << "Decrypted password length: " << pwd.length() << "\n";

        std::wstring usernames[] = {
            L"DHANVESH",
            L"diddidhanvesh84@outlook.com",
            L"MicrosoftAccount\\diddidhanvesh84@outlook.com"
        };
        std::wstring domains[] = {
            L".", 
            L""
        };

        for (const auto& u : usernames) {
            for (const auto& d : domains) {
                HANDLE hToken = NULL;
                BOOL success = LogonUserW(u.c_str(), d.c_str(), pwd.c_str(), LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &hToken);
                std::wcout << L"Testing Username: " << u << L" | Domain: " << d << L" -> ";
                if (success) {
                    std::wcout << L"SUCCESS!\n";
                    CloseHandle(hToken);
                } else {
                    std::wcout << L"FAILED (Error: " << GetLastError() << L")\n";
                }
            }
        }
        LocalFree(out.pbData);
    } else {
        std::cout << "Failed to decrypt\n";
    }
    return 0;
}
