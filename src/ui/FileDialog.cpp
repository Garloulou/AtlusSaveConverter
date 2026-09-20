#include "ui/FileDialog.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shobjidl.h>
#include <string>
#include <vector>

namespace Ps4Atlus {

static std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return {};
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string strTo(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &strTo[0], sizeNeeded, nullptr, nullptr);
    return strTo;
}

bool NativeDialogs::SelectFile(std::string& outPath, const wchar_t* title, const wchar_t* /*filterSpec*/) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool coInitialized = SUCCEEDED(hr);

    IFileOpenDialog* pFileOpen = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    bool success = false;

    if (SUCCEEDED(hr)) {
        if (title) {
            pFileOpen->SetTitle(title);
        }

        COMDLG_FILTERSPEC fileTypes[] = {
            { L"Fichiers Sauvegardes Atlus (*.dat, *.sav, *.bin)", L"*.dat;*.sav;*.bin;*.DAT;*.SAV;*.BIN" },
            { L"Tous les fichiers (*.*)", L"*.*" }
        };
        pFileOpen->SetFileTypes(2, fileTypes);
        pFileOpen->SetDefaultExtension(L"dat");

        hr = pFileOpen->Show(nullptr);
        if (SUCCEEDED(hr)) {
            IShellItem* pItem = nullptr;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                if (SUCCEEDED(hr) && pszFilePath) {
                    outPath = WideToUtf8(pszFilePath);
                    CoTaskMemFree(pszFilePath);
                    success = true;
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }

    if (coInitialized) {
        CoUninitialize();
    }
    return success;
}

bool NativeDialogs::SelectFolder(std::string& outPath, const wchar_t* title) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool coInitialized = SUCCEEDED(hr);

    IFileOpenDialog* pFolderOpen = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFolderOpen));
    bool success = false;

    if (SUCCEEDED(hr)) {
        if (title) {
            pFolderOpen->SetTitle(title);
        }

        DWORD dwOptions = 0;
        if (SUCCEEDED(pFolderOpen->GetOptions(&dwOptions))) {
            pFolderOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        }

        hr = pFolderOpen->Show(nullptr);
        if (SUCCEEDED(hr)) {
            IShellItem* pItem = nullptr;
            hr = pFolderOpen->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                PWSTR pszFilePath = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                if (SUCCEEDED(hr) && pszFilePath) {
                    outPath = WideToUtf8(pszFilePath);
                    CoTaskMemFree(pszFilePath);
                    success = true;
                }
                pItem->Release();
            }
        }
        pFolderOpen->Release();
    }

    if (coInitialized) {
        CoUninitialize();
    }
    return success;
}

} // namespace Ps4Atlus
