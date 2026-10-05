#include <windows.h>
#include <credentialprovider.h>
#include <initguid.h>
#include "Provider.h"

// {B9E1C1F6-3199-4A94-98F1-4F9EE8B41D12}
DEFINE_GUID(CLSID_FaceIDProvider, 
0xb9e1c1f6, 0x3199, 0x4a94, 0x98, 0xf1, 0x4f, 0x9e, 0xe8, 0xb4, 0x1d, 0x12);

long g_cRefModule = 0;
HINSTANCE g_hinst = NULL;

void DllAddRef() { InterlockedIncrement(&g_cRefModule); }
void DllRelease() { InterlockedDecrement(&g_cRefModule); }

class CClassFactory : public IClassFactory {
public:
    CClassFactory() : _cRef(1) {}
    
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) {
        static const QITAB qit[] = {
            QITABENT(CClassFactory, IClassFactory),
            { 0 }
        };
        return QISearch(this, qit, riid, ppv);
    }
    
    IFACEMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&_cRef); }
    IFACEMETHODIMP_(ULONG) Release() {
        long cRef = InterlockedDecrement(&_cRef);
        if (!cRef) delete this;
        return cRef;
    }
    
    IFACEMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) {
        if (pUnkOuter) return CLASS_E_NOAGGREGATION;
        CProvider* pProvider = new CProvider();
        if (!pProvider) return E_OUTOFMEMORY;
        HRESULT hr = pProvider->QueryInterface(riid, ppv);
        pProvider->Release();
        return hr;
    }
    
    IFACEMETHODIMP LockServer(BOOL bLock) {
        if (bLock) DllAddRef();
        else DllRelease();
        return S_OK;
    }
private:
    long _cRef;
};

STDAPI DllGetClassObject(REFIID rclsid, REFIID riid, void** ppv) {
    if (rclsid != CLSID_FaceIDProvider) return CLASS_E_CLASSNOTAVAILABLE;
    CClassFactory* pFactory = new CClassFactory();
    if (!pFactory) return E_OUTOFMEMORY;
    HRESULT hr = pFactory->QueryInterface(riid, ppv);
    pFactory->Release();
    return hr;
}

STDAPI DllCanUnloadNow() {
    return g_cRefModule == 0 ? S_OK : S_FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        g_hinst = hinst;
        DisableThreadLibraryCalls(hinst);
    }
    return TRUE;
}
