#include "Provider.h"
#include "Credential.h"

IFACEMETHODIMP CProvider::QueryInterface(REFIID riid, void** ppv) {
    static const QITAB qit[] = {
        QITABENT(CProvider, ICredentialProvider),
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) CProvider::AddRef() { return InterlockedIncrement(&_cRef); }
IFACEMETHODIMP_(ULONG) CProvider::Release() {
    long cRef = InterlockedDecrement(&_cRef);
    if (!cRef) delete this;
    return cRef;
}

IFACEMETHODIMP CProvider::SetUsageScenario(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, DWORD dwFlags) {
    if (cpus == CPUS_LOGON || cpus == CPUS_UNLOCK_WORKSTATION) {
        _cpus = cpus;
        return S_OK;
    }
    return E_NOTIMPL;
}

IFACEMETHODIMP CProvider::SetSerialization(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs) {
    return E_NOTIMPL;
}

IFACEMETHODIMP CProvider::Advise(ICredentialProviderEvents* pcpe, UINT_PTR upAdviseContext) {
    if (_pcpe) return E_UNEXPECTED;
    _pcpe = pcpe;
    _pcpe->AddRef();
    _upAdviseContext = upAdviseContext;
    return S_OK;
}

IFACEMETHODIMP CProvider::UnAdvise() {
    if (_pcpe) {
        _pcpe->Release();
        _pcpe = nullptr;
    }
    _upAdviseContext = 0;
    return S_OK;
}

IFACEMETHODIMP CProvider::GetFieldDescriptorCount(DWORD* pdwCount) {
    *pdwCount = 1; // Just one text field for status
    return S_OK;
}

IFACEMETHODIMP CProvider::GetFieldDescriptorAt(DWORD dwIndex, CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd) {
    if (dwIndex != 0) return E_INVALIDARG;
    
    *ppcpfd = (CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR*)CoTaskMemAlloc(sizeof(CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR));
    if (!*ppcpfd) return E_OUTOFMEMORY;
    
    (*ppcpfd)->dwFieldID = 0;
    (*ppcpfd)->cpft = CPFT_LARGE_TEXT;
    (*ppcpfd)->pszLabel = nullptr;
    (*ppcpfd)->guidFieldType = GUID_NULL;
    
    return S_OK;
}

IFACEMETHODIMP CProvider::GetCredentialCount(DWORD* pdwCount, DWORD* pdwDefault, BOOL* pbAutoLogonWithDefault) {
    *pdwCount = 1;
    *pdwDefault = 0;
    *pbAutoLogonWithDefault = FALSE;
    
    if (_pCredential && _pCredential->IsAuthenticated()) {
        *pbAutoLogonWithDefault = TRUE;
    }
    
    FILE* f;
    if (fopen_s(&f, "C:\\FaceID\\cp_debug.log", "a") == 0) {
        fprintf(f, "Provider::GetCredentialCount. AutoLogon=%d\n", *pbAutoLogonWithDefault);
        fclose(f);
    }
    
    return S_OK;
}

IFACEMETHODIMP CProvider::GetCredentialAt(DWORD dwIndex, ICredentialProviderCredential** ppcpc) {
    if (dwIndex != 0) return E_INVALIDARG;
    
    if (!_pCredential) {
        _pCredential = new CCredential(_pcpe, _upAdviseContext);
        if (!_pCredential) return E_OUTOFMEMORY;
    }
    
    FILE* f;
    if (fopen_s(&f, "C:\\FaceID\\cp_debug.log", "a") == 0) {
        fprintf(f, "Provider::GetCredentialAt(Index=0)\n");
        fclose(f);
    }
    
    return _pCredential->QueryInterface(IID_PPV_ARGS(ppcpc));
}
