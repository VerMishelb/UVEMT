#ifndef _FileOperations_h_
#define _FileOperations_h_

#include <string>
#include <ShlObj.h>

namespace FileOperations {
	std::wstring OpenFile(const _COMDLG_FILTERSPEC* filter = nullptr, size_t filter_size = 0, const wchar_t* title = L"");
	std::wstring OpenFolder(const wchar_t* title = nullptr, const wchar_t* def_folder = nullptr);
	std::wstring SaveFile(const _COMDLG_FILTERSPEC* filter = nullptr, size_t filter_size = 0, const wchar_t* def_ext = nullptr, const wchar_t* file_name = nullptr, const wchar_t* title = nullptr);
}

std::string BrowseFolder(std::string saved_path);
std::string BrowseFolderNEW(std::string saved_path);

#endif