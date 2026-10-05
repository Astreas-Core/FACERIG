#pragma once
#include <credentialprovider.h>
#include <windows.h>
#include <shlwapi.h>

extern void DllAddRef();
extern void DllRelease();

#include "Credential.h"

class CProvider : public ICredentialProvider {
public:
    CProvider() : _cRef(1) { DllAddRef(); }
    ~CProvider() { 
        if (_pCredential) _pCredential->Release();
        DllRelease(); 
    }

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv);
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();

    // ICredentialProvider
    IFACEMETHODIMP SetUsageScenario(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, DWORD dwFlags);
    IFACEMETHODIMP SetSerialization(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs);
    IFACEMETHODIMP Advise(ICredentialProviderEvents* pcpe, UINT_PTR upAdviseContext);
    IFACEMETHODIMP UnAdvise();
    IFACEMETHODIMP GetFieldDescriptorCount(DWORD* pdwCount);
    IFACEMETHODIMP GetFieldDescriptorAt(DWORD dwIndex, CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd);
    IFACEMETHODIMP GetCredentialCount(DWORD* pdwCount, DWORD* pdwDefault, BOOL* pbAutoLogonWithDefault);
    IFACEMETHODIMP GetCredentialAt(DWORD dwIndex, ICredentialProviderCredential** ppcpc);

private:
    long _cRef;
    ICredentialProviderEvents* _pcpe = nullptr;
    UINT_PTR _upAdviseContext = 0;
    CREDENTIAL_PROVIDER_USAGE_SCENARIO _cpus;
    CCredential* _pCredential = nullptr;
};
