#include "Credential.h"
#include "../Security/SecureStorage.h"
#include <strsafe.h>
#include <wincred.h>
#include <string>
#include <vector>

#pragma comment(lib, "Credui.lib")

IFACEMETHODIMP CCredential::QueryInterface(REFIID riid, void** ppv) {
    static const QITAB qit[] = {
        QITABENT(CCredential, ICredentialProviderCredential),
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) CCredential::AddRef() { return InterlockedIncrement(&_cRef); }
IFACEMETHODIMP_(ULONG) CCredential::Release() {
    long cRef = InterlockedDecrement(&_cRef);
    if (!cRef) delete this;
    return cRef;
}

IFACEMETHODIMP CCredential::Advise(ICredentialProviderCredentialEvents* pcpce) {
    if (_pcpce) return E_UNEXPECTED;
    _pcpce = pcpce;
    _pcpce->AddRef();
    
    FILE* f;
    if (fopen_s(&f, "C:\\FaceID\\cp_debug.log", "a") == 0) {
        fprintf(f, "Credential::Advise starting thread\n");
        fclose(f);
    }

    // Start background facial scan
    std::thread([this]() {
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        std::wstring cmd = L"C:\\FaceID\\FaceIDApp.exe --test --headless --no-liveness --strictness 0.25";
        std::vector<wchar_t> cmdBuffer(cmd.begin(), cmd.end());
        cmdBuffer.push_back(0);

        if (CreateProcessW(NULL, cmdBuffer.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, L"C:\\FaceID", &si, &pi)) {
            WaitForSingleObject(pi.hProcess, 30000); // Wait up to 30s while lock screen is active
            DWORD exitCode = 1;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            FILE* f2;
            if (fopen_s(&f2, "C:\\FaceID\\cp_debug.log", "a") == 0) {
                fprintf(f2, "Credential thread finished. ExitCode=%lu\n", exitCode);
                fclose(f2);
            }

            if (exitCode == 0) {
                this->_isAuthenticated = true;
                if (this->_pcpe) {
                    this->_pcpe->CredentialsChanged(this->_upAdviseContext);
                }
            }
        }
    }).detach();

    return S_OK;
}

IFACEMETHODIMP CCredential::UnAdvise() {
    if (_pcpce) {
        _pcpce->Release();
        _pcpce = nullptr;
    }
    return S_OK;
}

IFACEMETHODIMP CCredential::SetSelected(BOOL* pbAutoLogon) {
    if (_isAuthenticated) {
        *pbAutoLogon = TRUE;
    } else {
        *pbAutoLogon = FALSE;
    }
    
    FILE* f;
    if (fopen_s(&f, "C:\\FaceID\\cp_debug.log", "a") == 0) {
        fprintf(f, "Credential::SetSelected. AutoLogon=%d\n", *pbAutoLogon);
        fclose(f);
    }
    
    return S_OK;
}

IFACEMETHODIMP CCredential::SetDeselected() {
    return S_OK;
}

IFACEMETHODIMP CCredential::SetDeserializedAuthenticationState(DWORD dwState) { return E_NOTIMPL; }

IFACEMETHODIMP CCredential::GetFieldState(DWORD dwFieldID, CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs, CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis) {
    if (dwFieldID != 0) return E_INVALIDARG;
    *pcpfs = CPFS_DISPLAY_IN_BOTH;
    *pcpfis = CPFIS_NONE;
    return S_OK;
}

IFACEMETHODIMP CCredential::GetStringValue(DWORD dwFieldID, LPWSTR* ppsz) {
    if (dwFieldID != 0) return E_INVALIDARG;
    SHStrDupW(L"FaceID Windows", ppsz);
    return S_OK;
}

IFACEMETHODIMP CCredential::GetBitmapValue(DWORD dwFieldID, HBITMAP* phbmp) { return E_NOTIMPL; }
IFACEMETHODIMP CCredential::GetCheckboxValue(DWORD dwFieldID, BOOL* pbChecked, LPWSTR* ppszLabel) { return E_NOTIMPL; }
IFACEMETHODIMP CCredential::GetSubmitButtonValue(DWORD dwFieldID, DWORD* pdwAdjacentTo) { return E_NOTIMPL; }
IFACEMETHODIMP CCredential::GetComboBoxValueCount(DWORD dwFieldID, DWORD* pcItems, DWORD* pdwComboBoxDefault) { return E_NOTIMPL; }
IFACEMETHODIMP CCredential::GetComboBoxValueAt(DWORD dwFieldID, DWORD dwItem, LPWSTR* ppszItem) { return E_NOTIMPL; }
IFACEMETHODIMP CCredential::SetStringValue(DWORD dwFieldID, LPCWSTR psz) { return E_NOTIMPL; }
IFACEMETHODIMP CCredential::SetCheckboxValue(DWORD dwFieldID, BOOL bChecked) { return E_NOTIMPL; }
IFACEMETHODIMP CCredential::SetComboBoxSelectedValue(DWORD dwFieldID, DWORD dwSelectedItem) { return E_NOTIMPL; }
// CLSID for FaceIDProvider
extern "C" const GUID CLSID_FaceIDProvider;

IFACEMETHODIMP CCredential::CommandLinkClicked(DWORD dwFieldID) {
    return E_NOTIMPL;
}

IFACEMETHODIMP CCredential::GetSerialization(CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr, CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs, LPWSTR* ppszOptionalStatusText, CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon) {
    if (!_isAuthenticated) {
        *pcpgsr = CPGSR_NO_CREDENTIAL_NOT_FINISHED;
        return S_OK;
    }

    // Load machine-encrypted password
    std::vector<unsigned char> pwData;
    std::string path = "C:\\FaceID\\password_DefaultUser.bin";
    
    if (!FaceID::Security::SecureStorage::LoadMachineEncryptedFile(path, pwData) || pwData.empty()) {
        *pcpgsr = CPGSR_NO_CREDENTIAL_NOT_FINISHED;
        return S_OK;
    }

    std::wstring password(reinterpret_cast<wchar_t*>(pwData.data()), pwData.size() / sizeof(wchar_t));
    std::wstring username = L"YOUR_USERNAME"; 

    DWORD cbAuthBuffer = 0;
    CredPackAuthenticationBufferW(0, const_cast<LPWSTR>(username.c_str()), const_cast<LPWSTR>(password.c_str()), nullptr, &cbAuthBuffer);
    
    if (cbAuthBuffer > 0) {
        pcpcs->rgbSerialization = (BYTE*)CoTaskMemAlloc(cbAuthBuffer);
        if (pcpcs->rgbSerialization) {
            pcpcs->cbSerialization = cbAuthBuffer;
            CredPackAuthenticationBufferW(0, const_cast<LPWSTR>(username.c_str()), const_cast<LPWSTR>(password.c_str()), pcpcs->rgbSerialization, &cbAuthBuffer);
            
            // Route the credential to the Built-in Password Provider
            const GUID CLSID_PasswordCredentialProvider = { 0x60b78e88, 0xead8, 0x445c, { 0x9c, 0xfd, 0x0b, 0x87, 0xf7, 0x4e, 0xa6, 0xcd } };
            pcpcs->clsidCredentialProvider = CLSID_PasswordCredentialProvider;
            
            FILE* f;
            if (fopen_s(&f, "C:\\FaceID\\cp_debug.log", "a") == 0) {
                fprintf(f, "GetSerialization SUCCESS! Packed %lu bytes. Auth=%d\n", cbAuthBuffer, _isAuthenticated.load());
                fclose(f);
            }

            *pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
            return S_OK;
        }
    }
    
    FILE* f;
    if (fopen_s(&f, "C:\\FaceID\\cp_debug.log", "a") == 0) {
        fprintf(f, "GetSerialization FAILED inside buffer prep. cbAuthBuffer=%lu\n", cbAuthBuffer);
        fclose(f);
    }
    *pcpgsr = CPGSR_NO_CREDENTIAL_NOT_FINISHED;
    return S_OK;
}

IFACEMETHODIMP CCredential::ReportResult(NTSTATUS ntsStatus, NTSTATUS ntsSubstatus, LPWSTR* ppszOptionalStatusText, CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon) {
    FILE* f;
    if (fopen_s(&f, "C:\\FaceID\\cp_debug.log", "a") == 0) {
        fprintf(f, "ReportResult Called! ntsStatus=0x%lx, ntsSubstatus=0x%lx\n", ntsStatus, ntsSubstatus);
        fclose(f);
    }
    return S_OK;
}
