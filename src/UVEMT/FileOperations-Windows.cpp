#include "FileOperations.hpp"

#define NOMINMAX
#include <Windows.h>
#undef NOMINMAX
#include "Functions.hpp"
#include <iostream>

namespace FileOperations {

std::wstring OpenFile(const _COMDLG_FILTERSPEC* filter, size_t filter_size, const wchar_t* title) {
    HRESULT olehr = OleInitialize(NULL);

    IFileOpenDialog* pFileOpenDialog = NULL;
    IShellItem* pShellItem = NULL;
    LPWSTR fullpath = NULL;

    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pFileOpenDialog));
    if (SUCCEEDED(hr)) {
        // Flags and commands
        // https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/ne-shobjidl_core-_fileopendialogoptions
        // https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ifiledialog
        hr = pFileOpenDialog->SetOptions(FOS_NOCHANGEDIR | FOS_FORCEFILESYSTEM);

        if (filter != nullptr) { hr = pFileOpenDialog->SetFileTypes(filter_size, filter); }
        if (title != nullptr) { hr = pFileOpenDialog->SetTitle(title); }

        hr = pFileOpenDialog->Show(NULL);
        if (SUCCEEDED(hr)) {
            hr = pFileOpenDialog->GetResult(&pShellItem);
            pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &fullpath);
        }
        pFileOpenDialog->Release();
    }
    CoTaskMemFree(fullpath);
    if (olehr == S_OK) { OleUninitialize(); }
    return fullpath != NULL ? std::wstring(fullpath) : L"";
}

std::wstring OpenFolder(const wchar_t* title, const wchar_t* default_folder) {
    HRESULT olehr = OleInitialize(NULL);

    IFileOpenDialog* pFileOpenDialog = NULL;
    IShellItem* pShellItem = NULL;
    LPWSTR fullpath = NULL;

    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pFileOpenDialog));
    if (SUCCEEDED(hr)) {
        hr = pFileOpenDialog->SetOptions(FOS_NOCHANGEDIR | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

        if (default_folder != nullptr) {
            IShellItem* defFolder = NULL;
            hr = SHCreateItemFromParsingName(default_folder, NULL, IID_PPV_ARGS(&defFolder));
            if (SUCCEEDED(hr)) { hr = pFileOpenDialog->SetDefaultFolder(defFolder); }
            defFolder->Release();
        }

        if (title != nullptr) { hr = pFileOpenDialog->SetTitle(title); }

        hr = pFileOpenDialog->Show(NULL);
        if (SUCCEEDED(hr)) {
            hr = pFileOpenDialog->GetResult(&pShellItem);
            pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &fullpath);
        }
        pFileOpenDialog->Release();
    }
    CoTaskMemFree(fullpath);
    if (olehr == S_OK) { OleUninitialize(); }
    return fullpath != NULL ? std::wstring(fullpath) : L"";
}

std::wstring SaveFile(const _COMDLG_FILTERSPEC* filter, size_t filter_size, const wchar_t* def_ext,
                      const wchar_t* file_name, const wchar_t* title) {
    HRESULT olehr = OleInitialize(NULL);

    IFileSaveDialog* pFileSaveDialog = NULL;
    IShellItem* pShellItem = NULL;
    LPWSTR fullpath = NULL;

    HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pFileSaveDialog));
    if (SUCCEEDED(hr)) {
        hr = pFileSaveDialog->SetOptions(FOS_NOCHANGEDIR | FOS_FORCEFILESYSTEM);

        if (filter != nullptr) { hr = pFileSaveDialog->SetFileTypes(filter_size, filter); }
        if (def_ext != nullptr) { hr = pFileSaveDialog->SetDefaultExtension(def_ext); }
        if (file_name != nullptr) { hr = pFileSaveDialog->SetFileName(file_name); }
        if (title != nullptr) { hr = pFileSaveDialog->SetTitle(title); }

        hr = pFileSaveDialog->Show(NULL);
        if (SUCCEEDED(hr)) {
            hr = pFileSaveDialog->GetResult(&pShellItem);
            hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &fullpath);
        }
        hr = pFileSaveDialog->Release();
    }
    CoTaskMemFree(fullpath);
    if (olehr == S_OK) { OleUninitialize(); }
    return fullpath != NULL ? std::wstring(fullpath) : L"";
}

} // namespace FileOperations

static int CALLBACK BrowseFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
    if (uMsg == BFFM_INITIALIZED) {
        LPCSTR path = reinterpret_cast<LPCSTR>(lpData);
        ::SendMessageA(hwnd, BFFM_SETSELECTIONA, true, (LPARAM)path);
    }
    return 0;
}

std::string BrowseFolder(std::string saved_path) {
    char path[MAX_PATH];

    const char* path_param = saved_path.c_str();

    BROWSEINFOA bi = {0};
    bi.lpszTitle = "Browse for folder...";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = BrowseFolderCallback;
    bi.lParam = (LPARAM)path_param;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);

    if (pidl != 0) {
        // get the name of the folder and put it in path
        SHGetPathFromIDListA(pidl, path);

        // free memory used
        IMalloc* imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }
        return path;
    }
    return "";
}

std::string BrowseFolderNEW(std::string saved_path) {
    return Func::Wide2UTF(
        FileOperations::OpenFolder(nullptr, Func::UTF2Wide(saved_path, 0).c_str()), 0);
}

// owocek, thank you again for this code
std::string OpenFileWindow(const char formats[]) {
    char filename[MAX_PATH];
    OPENFILENAMEA ofn;
    ZeroMemory(&filename, sizeof(filename));
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // If you have a window to center over, put its HANDLE here
    ofn.lpstrFilter =
        formats; // "Patapon Archive
                 // Format\0*.BND;*.LBND;*.MDLL;*.AMDL;*.TEXLS;*.BNS;*.LAYL;*.MDLN;*.BNS\0All
                 // Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Open a file";
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (filename[MAX_PATH - 1]) { filename[MAX_PATH - 1] = 0; }
    if (GetOpenFileNameA(&ofn)) { std::cout << "You chose the file \"" << filename << "\"\n"; }
    return std::string(filename);
}

std::string SaveFileWindow(const char formats[], const char def_format[]) {
    char filename[MAX_PATH];
    OPENFILENAMEA ofn;
    ZeroMemory(&filename, sizeof(filename));
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL; // If you have a window to center over, put its HANDLE here
    ofn.lpstrFilter =
        formats; // "Patapon Archive
                 // Format\0*.BND;*.LBND;*.MDLL;*.AMDL;*.TEXLS;*.BNS;*.LAYL;*.MDLN;*.BNS\0All
                 // Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Save file as...";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = def_format;

    if (filename[MAX_PATH - 1]) filename[MAX_PATH - 1] = 0;
    if (GetSaveFileNameA(&ofn)) { std::cout << "You chose the file \"" << filename << "\"\n"; }
    return std::string(filename);
}