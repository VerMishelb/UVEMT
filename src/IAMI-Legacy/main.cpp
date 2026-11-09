/*
Copyright 2020-2026 VerMishelb <vermishelb535@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

Icon 2 (icon1.ico) made by IceIris
Icon 1 (booka.ico) made by Francis

InterAction Mod Installer, 16.09.20

Installs mod files created with InterAction Mod Tool. Uses IAMT code as the base.
*/

//PLATFORM
//#define B_LINUX
#define B_WIN

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#include "Crossplatform.h"
#include <lmcons.h>


//#define DEBUG_OUTPUT
//#define USE_PATH_LOGGING
//#define EXCLUDE_4


#define IAMI_VERSION "v1.3.9.0 (/MD)"

#ifdef DEBUG_OUTPUT
#define DLOG(s) DLOGfunc((s), __LINE__)
#else
#define DLOG(s) ;
#endif

/*
CHANGELOG
1.3.9.0
* File buffer size can no longer be changed manually and is now hardcoded to 32 KB (was previously 10 MB by default).
* File buffer is also no longer allocated in the heap.
* Removed unnecessary write operations when copying files and calculating CRCs which only slowed things down.
* Corrected credits a bit.
* Added summer WADs to the open file dialog filter.
* Reduced the file size by stripping unnecessary stuff.
* The file dialogs should not be blurry on high DPI monitors, I hope.
* Changed the file description in resources to prevent it being long in the task scheduler. And bumped up the version there, as I forgot it's defined there since 1.3.6.3.
* Reworked the progress bar to reduce redraws.
* When a missing file is found, you can now type "all" instead of "y" to skip all missing files. Works until iami is closed.

1.3.8.1
Fixed the WAD file corruption if no files were found during the mod installation.

1.3.8.0
Added -wad <path> command line argument for setting the WAD (222x) file (use if the file selection dialog has issues).
Now prints out the path it searches for CIU.iami and config.cfg files.

1.3.7.0


1.3.6.3
Administrator privileges are no longer forced
Fixed "Open with..." action (bug introduced in 1.3.6). Now you can assign IAMI for .ciumod files and the like to make installation easier.
Fixed completion percentage output (on Windows 10 for sure, idk whether Win7's command line treats \\r escape sequence).

1.3.6.2
a

1.3.6.1
Added a resource file for version display.
Added *.???x to FILTERS_DAT to make 222x visible by default.

1.3.6
Removed pause call in credits list because it was kind of useless
No longer quits after successful installation to let user install mods one after another
Fixed (maybe) a minor IShellItem memory leak
Added (a bit broken?) ability to choose more than one mod file. Requires testing, use with caution.
Added file completion percentage

1.3.5
modded_file_path is now cleared correctly upon skipping a file for non-CIU games

1.3.4
Applied encoding fix for tool_path and for argv[] use. Fixes "Open with..." issues and hopefully .iami issues too.
Path logging now saves more paths (debug change)

1.3.3
Reworked open file dialog to avoid incorrect paths
Debug output is now correctly coloured (if present)
Fixed 1 typo :з
*/

const size_t BUF_SIZE = 32 * 1024;
char buffer[BUF_SIZE] = {0};

struct TableEntry {
	std::string path;
	std::uint32_t hash_djb2;
};

struct WAD4_Entry {
	std::uint32_t hash_djb2;
	std::uint32_t file_ptr;
	std::uint32_t size_uncompressed;
	std::uint32_t size;
};

void DLOGfunc(std::string s, int line_num) {
#if (defined _WIN32) || (defined _WIN64)
	HANDLE hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO attributes_old;
	GetConsoleScreenBufferInfo(hConsole, &attributes_old);
	unsigned short old_console_colour = attributes_old.wAttributes;

	SetConsoleTextAttribute(hConsole,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY |
		BACKGROUND_RED
	);
	std::cout << "[DEBUG]:" << line_num;
	SetConsoleTextAttribute(hConsole, old_console_colour & ~(FOREGROUND_BLUE) |
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	std::cout << " " << s << "\n";
	SetConsoleTextAttribute(hConsole, old_console_colour);
#else
	std::cout << "\x1b[48;5;22m" "\x1b[38;5;118m"
		"[DEBUG]:" << line_num <<
		"\x1b[0m" "\x1b[38;5;10m " << s << "\033[0m\n";
#endif
}

const unsigned char ek[] = { 67, 177, 172, 181, 38, 215, 76, 33, 89, 213, 93, 29, 211, 51, 27, 116, 238, 50, 240, 169, 25, 228, 143, 124, 9, 243, 184, 129, 32 };

#define FILTERS_MOD_SIZE 2
#define FILTERS_DAT_SIZE 2

const _COMDLG_FILTERSPEC FILTERS_MOD[] = {
	{L"IAMI Mod", L"*.ciumod;*.ci5mod;*.ci4mod;*.ci3mod;*.ci3oldmod;*.ci2mod;*.ci2rmod;*.ci1mod;*.unimod;*.svmod;*.pimod"},
	{L"All Files", L"*.*"},
};

const _COMDLG_FILTERSPEC FILTERS_DAT[] = {
	{L"WAD Files", L"*.dat;*.???x;*.easter;*.halloween;*.standard;*.summer;*.thanksgiving;*.xmas"},
	{L"All Files", L"*.*"},
};
/*


"IAMI Mod (*.CIUMod;*.CI5Mod;*.CI4Mod;*.CI3Mod;*.CI3OldMod;*.CI2Mod;*.CI2RMod;*.CI1Mod;*.UNIMod;*.SVMod;*.PIMod)
*.CIUMod;*.CI5Mod;*.CI4Mod;*.CI3Mod;*.CI3OldMod;*.CI2Mod;*.CI2RMod;*.CI1Mod;*.UNIMod;*.SVMod;*.PIMod"
*
"IAMI CIU Mod (*.CIUMod)  *.CIUMod\0"
"IAMI CI5 Mod (*.CI5Mod)  *.CI5Mod\0"
"IAMI CI4 Mod (*.CI4Mod)  *.CI4Mod\0"
"IAMI CI3 Mod (*.CI3Mod)  *.CI3Mod\0"
"IAMI Old CI3 Mod (*.CI3OldMod)  *.CI3OldMod\0"
"IAMI CI2 Mod (*.CI2Mod)  *.CI2Mod\0"
"IAMI CI2 Remastered Mod (*.CI2RMod)\0*.CI2RMod\0"
"IAMI CI1 Mod (*.CI1Mod)  *.CI1Mod\0"
"IAMI UNI-Mod (*.UNIMod)  *.UNIMod\0"
"IAMI Smileyville Mod (*.SVMod)  *.SVMod\0"
"IAMI Piggly Mod (*.PIMod)  *.PIMod\0"
"All Files  *.*


*/


//"Select.dat.hq2x in the game folder(if there is no, select.dat)" }

// Returns a xor-ed string of the same size, 0xCC terminated
std::string str_xor(std::string a) {
	std::string res;
	res.reserve(a.size());
	for (uint32_t i = 0; i < a.length(); i++)
		res += a[i] ^ 0xCC;
	return res;
}

//Use either std::string with CP_ACP codepage or std::wstring for file paths
std::string Wide2UTF(const std::wstring& wstr, unsigned int codepage = CP_UTF8) {
	if (wstr.empty()) { return std::string(); }
	//Codepage, flags, pointer to source [don't use .data() because NULL string would cause something bad], char amount of source, pointer to dest, size (bytes) of dest, NULL, NULL
	//Returns the required size, in bytes, for the destination buffer indicated by lpMultiByteStr
	int size_needed = WideCharToMultiByte(codepage, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(codepage, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

std::wstring UTF2Wide(const std::string& str, unsigned int codepage = CP_UTF8) {
	if (str.empty()) { return std::wstring(); }
	int size_needed = MultiByteToWideChar(codepage, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(codepage, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

std::wstring CommonItemOpenFile(const wchar_t* title, const _COMDLG_FILTERSPEC* filter, unsigned int filter_size) {
	std::wstring fullpath_wstr;

	HRESULT hr = OleInitialize(0);
	if (hr != S_OK) { fprintf_s(stderr, "[Error] OleInitialize() failed: %l\n", hr); }
	IFileOpenDialog* pFileOpenDialog = nullptr;
	IShellItem* pShellItem = nullptr;
	LPWSTR fullpath = nullptr;

	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpenDialog));
	if (SUCCEEDED(hr)) {
		//Flags and commands
		//https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/ne-shobjidl_core-_fileopendialogoptions
		//https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ifiledialog
		//NOCHANGEDIR prevents changing the working directory (!)
		hr = pFileOpenDialog->SetOptions(FOS_NOCHANGEDIR | FOS_FORCEFILESYSTEM);
		hr = pFileOpenDialog->SetTitle(title);
		hr = pFileOpenDialog->SetFileTypes(filter_size, filter);

		hr = pFileOpenDialog->Show(NULL);
		if (SUCCEEDED(hr)) {
			hr = pFileOpenDialog->GetResult(&pShellItem);
#ifdef DEBUG_OUTPUT
			if (!SUCCEEDED(hr)) {
				DLOG("GetResult() failed. hr = " + std::to_string((int)hr));
			}
#endif
			pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &fullpath);

#ifdef USE_PATH_LOGGING
			FILE* path_output_test = nullptr;
			fopen_s(&path_output_test, "IAMI_path_output_test_UTF16.txt", "ab");
			if (path_output_test != nullptr) {
				for (int i = 0; fullpath[i] != L'\0'; ++i) {
					fwrite(fullpath + i, sizeof(WCHAR), 1, path_output_test);
				}
				fputwc(L'\n', path_output_test);
				fclose(path_output_test);
			}
#endif
			fullpath_wstr = (fullpath == 0) ? L"" : fullpath;
			CoTaskMemFree(fullpath);
			pShellItem->Release();
		}
		pFileOpenDialog->Release();
	}
	OleUninitialize();
	return fullpath_wstr;
}

std::vector<std::wstring> CommonItemOpenFiles(const wchar_t* title, const _COMDLG_FILTERSPEC* filter, unsigned int filter_size) {
	std::wstring fullpath_wstr;
	std::vector<std::wstring> wstringsVec;

	HRESULT hr = OleInitialize(0);
	if (hr != S_OK) { fprintf_s(stderr, "[Error] OleInitialize() failed: %l\n", hr); }
	IFileOpenDialog* pFileOpenDialog = nullptr;
	IShellItemArray* pShellItemArray = nullptr;
	LPWSTR fullpath = nullptr;

	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpenDialog));
	if (SUCCEEDED(hr)) {
		//Flags and commands
		//https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/ne-shobjidl_core-_fileopendialogoptions
		//https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ifiledialog
		//NOCHANGEDIR prevents changing the working directory (!)
		hr = pFileOpenDialog->SetOptions(FOS_NOCHANGEDIR | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT);
		hr = pFileOpenDialog->SetTitle(title);
		hr = pFileOpenDialog->SetFileTypes(filter_size, filter);

		hr = pFileOpenDialog->Show(NULL);
		if (SUCCEEDED(hr)) {
			hr = pFileOpenDialog->GetResults(&pShellItemArray);
#ifdef DEBUG_OUTPUT
			if (!SUCCEEDED(hr)) {
				DLOG("GetResult() failed. hr = " + std::to_string((int)hr));
			}
#endif
			DWORD tmpCount = 0;
			pShellItemArray->GetCount(&tmpCount);
			wstringsVec.reserve(tmpCount);
			for (uint32_t i = 0; i < tmpCount; ++i) {
				IShellItem* item = nullptr;
				pShellItemArray->GetItemAt(i, &item);
				item->GetDisplayName(SIGDN_FILESYSPATH, &fullpath);
				fullpath_wstr = (fullpath == 0) ? L"" : fullpath;
				wstringsVec.emplace_back(fullpath_wstr);
				CoTaskMemFree(fullpath);
				fullpath = 0;
				fullpath_wstr = L"";
				item->Release();
			}
		}
		pFileOpenDialog->Release();
	}
	OleUninitialize();
	return wstringsVec;
}

//string OpenFileWindow(const char formats[], const char title[])//owocek, thank you again for this code
//{
//	char filename[MAX_PATH];
//
//	OPENFILENAME ofn;
//	ZeroMemory(&filename, sizeof(filename));
//	ZeroMemory(&ofn, sizeof(ofn));
//	ofn.lStructSize = sizeof(ofn);
//	ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
//	ofn.lpstrFilter = formats;// "Patapon Archive Format\0*.BND;*.LBND;*.MDLL;*.AMDL;*.TEXLS;*.BNS;*.LAYL;*.MDLN;*.BNS\0All Files\0*.*\0";
//	ofn.lpstrFile = filename;
//	ofn.nMaxFile = MAX_PATH;
//	ofn.lpstrTitle = title;
//	ofn.Flags = OFN_FILEMUSTEXIST;
//
//	if (GetOpenFileNameA(&ofn))
//	{
//		std::cout << '\"' << filename << "\"\n";
//	}
//	std::string return_str = filename;
//	return return_str;
//}

uint32_t CRC32(char* buf, size_t len) {
	uint32_t crc_table[256];
	uint32_t crc;
	for (int i = 0; i < 256; i++) {
		crc = i;
		for (int j = 0; j < 8; j++)
			crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;
		crc_table[i] = crc;
	}
	crc = 0xFFFFFFFFUL;

	while (len--)
		crc = crc_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
	return crc ^ 0xFFFFFFFFUL;
}

uint32_t CRC32(std::fstream& file, size_t len, size_t start_from = 0) {
	//Generate CRC table (?)
	uint32_t crc_table[256];
	uint32_t crc;
	for (int i = 0; i < 256; i++) {
		crc = i;
		for (int j = 0; j < 8; j++)
			crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;
		crc_table[i] = crc;
	}
	crc = 0xFFFFFFFFUL;

	// char* buffer = new char[BUF_SIZE];
	char* buffer_ptr = buffer;
	// memset(buffer, 0, BUF_SIZE);
	std::streampos old_file_pos = file.tellg();

	size_t current_pos = 0;
	file.seekg(start_from);
	while (current_pos < len) {
		size_t bytes_amount;
		if (len - current_pos >= BUF_SIZE) {
			bytes_amount = BUF_SIZE;
		}
		else {
			bytes_amount = len - current_pos;
		}
		current_pos += bytes_amount;

		file.read(buffer_ptr, bytes_amount);
		char* buf_pos = buffer_ptr;
		while (bytes_amount--)
			crc = crc_table[(crc ^ *buffer_ptr++) & 0xFF] ^ (crc >> 8);
		buffer_ptr = buf_pos;
		DLOG("[CRC] Bytes " + std::to_string(current_pos) + " of " + std::to_string(len));
	}
	file.seekg(old_file_pos);
	// delete[] buffer;

	return crc ^ 0xFFFFFFFFUL;
}

uint32_t CRC32(const std::vector<uint8_t>& buf, size_t bufSize) {
	uint32_t crc_table[256];
	uint32_t crc;
	for (uint16_t i = 0; i < 256; i++) {
		crc = i;
		for (int8_t j = 0; j < 8; j++)
			crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;
		crc_table[i] = crc;
	}
	crc = 0xFFFFFFFFUL;

	for (size_t i = 0; i < bufSize; i++)
		crc = crc_table[(crc & 0xFF) ^ buf[i]] ^ (crc >> 8);

	return crc ^ 0xFFFFFFFFUL;
}

void protection(char* buf, size_t buf_size, uintmax_t start_offset) {
	for (size_t counter = 0; counter < buf_size; counter++) {
		buf[counter] -= ek[start_offset % 29];
		start_offset++;
	}
}

void protection(std::vector<uint8_t>& buf, size_t buf_size, size_t start_offset) {
	for (size_t counter = 0; counter < buf_size; ++counter) {
		buf[counter] -= ek[start_offset % 29];
		++start_offset;
	}
}

uint32_t decursed(uint32_t value, uintmax_t start_offset) {
	char arr[4];
	uint32_t r_value;
	memcpy(arr, &value, sizeof(uint32_t));
	protection(arr, 4, start_offset);
	memcpy(&r_value, arr, sizeof(uint32_t));
	return r_value;
}

uint8_t decursed(uint8_t value, uint64_t start_offset) {
	uint8_t ek[] = { 67, 177, 172, 181, 38, 215, 76, 33, 89, 213, 93, 29, 211, 51, 27, 116, 238, 50, 240, 169, 25, 228, 143, 124, 9, 243, 184, 129, 32 };
	value -= ek[start_offset % 29];
	return value;
}


//Internal stuff
void VecWrite(std::vector<uint8_t>& destination, rsize_t dOffset, const std::vector<uint8_t>& source, rsize_t sSize) {
	memcpy_s(destination.data() + dOffset, destination.size() - dOffset, source.data(), sSize);
}

template <typename T>
void VecWrite(std::vector<uint8_t>& destination, rsize_t dOffset, T* val) {
	memcpy_s(destination.data() + dOffset, destination.size() - dOffset, val, sizeof(T));
}

void VecRead(uint16_t& to, const std::vector<uint8_t>& from, rsize_t offs) {
	if (to != 0) to = 0;
	for (int i = offs + sizeof(to) - 1; i >= offs && i >= 0 && i < from.size(); --i)
		to = (to << 8) + from[i];
}

void VecRead(uint32_t& to, const std::vector<uint8_t>& from, rsize_t offs) {
	if (to != 0) to = 0;
	for (int i = offs + sizeof(to) - 1; i >= offs && i >= 0 && i < from.size(); --i)
		to = (to << 8) + from[i];
}

void VecRead(std::string& to, const std::vector<uint8_t>& from, rsize_t offs, size_t estimated_length = 0, uint8_t terminator = '\0', size_t limit = MAXSIZE_T) {
	size_t written = 0;
	if (estimated_length) {
		to.reserve(estimated_length);
	}
	for (; from[offs] != terminator && offs < from.size() && written < limit; ++offs) {
		to += from[offs];
		++written;
	}
	to += terminator;
	to.shrink_to_fit();
}

void FReadStr(std::string& to, std::fstream& from, size_t estimated_length, char terminator = '\0') {
	char* str = new char[estimated_length + 1];
	from.get(str, estimated_length + 1, terminator);
	if (from.fail()) {//If no characters extracted
		from.clear();
	}
	to.reserve(estimated_length);
	for (size_t i = 0; i < estimated_length && str[i] != '\0'; ++i) {
		to += str[i];
	}
	delete[] str;
	to += terminator;
	to.shrink_to_fit();
}

char PROGRESS_BAR_LINE[] = "[---------------]";
const int PROGRESS_BAR_WIDTH = 15;
int PROGRESS_BAR_last_progress = 0;
static bool ProgressBarReset() {
	if (PROGRESS_BAR_last_progress) {
		for (int i = 1; i < PROGRESS_BAR_WIDTH + 1; ++i) {
			switch (i % 5) {
			case 1: PROGRESS_BAR_LINE[i] = '.'; break;
			case 2: PROGRESS_BAR_LINE[i] = '_'; break;
			case 3: PROGRESS_BAR_LINE[i] = ' '; break;
			case 4: PROGRESS_BAR_LINE[i] = '.'; break;
			case 0: PROGRESS_BAR_LINE[i] = ' '; break;
			}
		}
		PROGRESS_BAR_last_progress = 0;
		return true;
	}
	else {
		return false;
	}
}
// Progress must be 0-PROGRESS_BAR_WIDTH.
static bool ProgressBarSet(int progress) {
	if (progress == PROGRESS_BAR_last_progress) return false;
	if (progress == 0) { return ProgressBarReset(); }

	for (int i = 1+PROGRESS_BAR_last_progress; i <= progress; ++i ) {
		PROGRESS_BAR_LINE[i] = '#';
	}
	PROGRESS_BAR_last_progress = progress;
	return true;
}

void CopyFilePart(std::fstream& f_from, std::fstream& f_to, size_t part_size, size_t decurse_pos = -1) {
	// char* buffer = new char[BUF_SIZE];
	// memset(buffer, 0, BUF_SIZE);

	size_t current_pos = 0;
	putchar('\n');
	printf("Copying files\n");
	ProgressBarReset();
	while (current_pos < part_size) {
		size_t bytes_amount;
		if (part_size - current_pos >= BUF_SIZE) {
			bytes_amount = BUF_SIZE;
		}
		else {
			bytes_amount = part_size - current_pos;
		}

		//mumbo jumbo yed
		f_from.read(buffer, bytes_amount);
		if (decurse_pos != -1) {
			protection(buffer, bytes_amount, current_pos + decurse_pos);
		}
		f_to.write(buffer, bytes_amount);
		current_pos += bytes_amount;
		if (ProgressBarSet(current_pos / (part_size / PROGRESS_BAR_WIDTH))) {
			printf("\r%s", PROGRESS_BAR_LINE);
		}
		DLOG("Bytes " + std::to_string(current_pos) + " of " + std::to_string(part_size));
	}
	putchar('\n');
	// delete[] buffer;
}

void CopyFilePart(std::ifstream& f_from, std::fstream& f_to, size_t part_size, size_t decurse_pos = -1) {
	// char* buffer = new char[BUF_SIZE];
	// memset(buffer, 0, BUF_SIZE);

	size_t current_pos = 0;
	while (current_pos < part_size) {
		size_t bytes_amount;
		if (part_size - current_pos >= BUF_SIZE) {
			bytes_amount = BUF_SIZE;
		}
		else {
			bytes_amount = part_size - current_pos;
		}

		//mumbo jumbo yed
		f_from.read(buffer, bytes_amount);
		if (decurse_pos != -1) {
			protection(buffer, bytes_amount, current_pos + decurse_pos);
		}
		f_to.write(buffer, bytes_amount);
		current_pos += bytes_amount;
		DLOG("Bytes " + std::to_string(current_pos) + " of " + std::to_string(part_size));
	}
	// delete[] buffer;
}

void callPause(const char* message = "Press Enter to continue...") {
	std::cout << message;
	std::cin.ignore(10000, '\n');
}

void deleteAllShit(const std::wstring& dir, const std::wstring ext = L".iamitemp") {
	for (const auto& p : std::filesystem::directory_iterator(dir)) {
		if (p.path().extension() == ext.c_str()) {
			printf("Removing temp files...\n");
#ifdef  DEBUG_OUTPUT
			std::cout << "\nRemove: " << Wide2UTF(p.path().c_str()) << '\n';
#endif //  DEBUG_OUTPUT
			std::filesystem::remove(p.path());
		}
	}
}

int main(int argc, char* argv[]) {
	setlocale(LC_CTYPE, ".UTF8");
	std::cout << "InterAction Mod Installer " IAMI_VERSION " (" __DATE__ ") by VerMishelb\n\n"
		"If you found any bugs or crashes, please let me know in Chicken Invaders Discord server or in direct messages (@vmlb)\n"
		"Please note that InterAction studios IS NOT related to IAMT or IAMI development and won't help you if you corrupted your game files while doing mods."
		"\n\t\t- Mishelb\n\n";
	std::string str;
	std::string tool_path;
	ProgressBarReset();

	{
		std::string strargv = Wide2UTF(UTF2Wide(argv[0], CP_ACP));
		tool_path = strargv.substr(0, strargv.find_last_of(u8"\\/") + 1);
	}

	std::wstring TEMP_PATH = std::filesystem::temp_directory_path().c_str() /*+ L'/'*/;

	//Delete temp files if they somehow lived
	deleteAllShit(TEMP_PATH);

	char mode = '0';
	char hq2x_type = '?';//1-CI1 2-CI2 3-CI345 4-CIU

	bool mode_selected = false;
	bool wait_for_files = true;
	bool full_backup = true;
	bool last_backup = false;
	bool skip_missing_paths = false;

	std::fstream ifile;
	std::fstream ofile;

	std::ifstream mod_file_f;
	std::cout << "Tool folder: " << tool_path << std::endl;
	std::ifstream config(tool_path + "config.cfg");
	if (!config) { std::cerr << "[Warning] Can't find/open config.cfg in the tool folder. Get one from the tool archive. The tool uses default values instead.\n\n"; }

	//Read config data
	else {
		std::cout << "Reading config.\n";
		std::string config_lines[] = { "full_backup", "last_backup", "buffer_size_kb" }; //lines in config to search
		while (getline(config, str)) {
			if (str.empty() == true) {
				continue;
			}
			else if (str.at(0) == '#' || str.at(0) == ';') { //Comment
				continue;
			}
			else if (str.find(config_lines[0]) != std::string::npos) { //Full backup
				str = str.substr(str.find('=', str.find(config_lines[0])) + 1);
				full_backup = stoi(str);
				continue;
			}
			else if (str.find(config_lines[1]) != std::string::npos) { //Last successful replacement backup
				str = str.substr(str.find('=', str.find(config_lines[1])) + 1);
				last_backup = stoi(str);
				continue;
			}
			else if (str.find(config_lines[2]) != std::string::npos) {
				std::cout << "[Config] Config parameter \"" << config_lines[2] << "\" is ignored since IAMI v1.3.9.0." << std::endl;
				// str = str.substr(str.find('=', str.find(config_lines[2])) + 1);
				// BUF_SIZE = std::stoull(str) * 1024;
			}
		}
		config.close();
		std::cout << "Done!\n\n";
	}

	//Mode setup via command line arguments
	if (argc > 1) {
		mode = 'i';

#ifdef DEBUG_OUTPUT
		DLOG("The following arguments are present:");
		for (int i = 0; i < argc; ++i) {
			DLOG(argv[i]);
		}
#endif
		// if (argc > 2) { wait_for_files = false; }
	}

LABEL_MENU:
	//Menu, mode selection
	if (mode == '0') {
		std::string inpt;

		while (mode_selected == false) {
			std::cout << "\rPlease, select an option (Write a number or a word and press \"Enter\"):\n"
				"C - Show the list of cool people (credits).\n"
				"0|exit - Close the tool.\n"
				"1|install - Install the mod.\n"
				">";

			std::getline(std::cin, inpt);

			if (inpt == "0" || inpt == "ciao" || inpt == "exit" || inpt == "quit") { std::cout << "Quitting..."; exit(0); }
			else if (inpt == "1" || inpt == "install" || inpt == "i" || inpt == "I") {
				mode = 'i';
				wait_for_files = true;
				mode_selected = true;
				break;
			}
			else if (inpt == "C" || inpt == "c") {
				std::cout <<
					"\n\nInterAction Mod Tool v3.5.1 (16 September 2020) (At the moment of IAMI release)\nInterAction Mod Installer " IAMI_VERSION " (" __DATE__ ")\n"
					"[List of cool people]\n"
					"VerMishelb (DC @vmlb) - \"Wow that iami code sucks\", coding, lack of testing, ._ ., bomkin.\n"
					"One-Winged Lunarian (DC @onewinglunarian) - Testing, features ideas, interface ideas, epic modding.\n"
					"Emerald (Jade) - Information about files format, testing, motivation to start working on this, \"oh\".\n"
					"owocek - Great help with coding, giving the author chocky milk \"because he is awesome\".\n"
					"BoggY - Testing, finding the most terrible bug.\n"
					"InterAction studios - Letting modding in their games be possible, best programmer ever (~~please don't update WAD I need to sleep ,_ ,~~).\n"
					"Okina (IceIris#7283) - Didn't bounce, made new icon, meow, wah.\n"
					"Hale - Testing from down under and selling australiums for tea.\n"
					"Francis (DC @lostwherethemountaingrows) - Made the tool OUR, design for old IAMI icon, didn't know what the tool did all along, oh ok, booka booka, best Barbie doll lover ever, sky-clad observer.\n"
					"Reggy - Why doesn't it work only for you?! (UPD: it now does but with pain), tung.\n"
					"Ramon - Stuck in the bowl of ramen.\n"
					"Galbatorix - Saving my sanity.\n\n"
					"And everyone who used IAMT before v3.0 release. Thanks to your feedback it became even more useful and handy than I expected (and mod installer exists).\n"
					"Take your :chocken:, you deserve it!\n\n"
					"Also special thanks to:\n"
					"UTF-8 for being not default codepage in Windows (now I hate Windows)\n"
					"Visual Studio for making up errors at error-impossible places and literally updating every hour >:(\n\n";
			}
		}
	}


	//Installation mode
	if (mode == 'i') {
		uint32_t target_crc;
		uint32_t files_amount;
		uint32_t mod_files_amount;
		uint32_t file_pointer;
		uint32_t file_size;
		uint32_t modded_file_pointer;
		uint32_t modded_file_size;
		uint32_t new_file_size;
		uint32_t mod_version;

		size_t h_file_offset;//file in the header offset
		size_t erase_point = 0;//Used for erasing residual bytes "if they are exist"
		size_t full_size = 0;//Usually =hq2x_size but for temp file

		std::string hq2x;//dat file path
		size_t hq2x_size;
		std::string mod_file_path;//Path of the MOD FILE
		std::string modded_file_path;//Path of the file INSIDE the mod
		std::string res;//Used for name search comparison

		std::string mod_name;
		std::string mod_description;

		std::vector<std::wstring> mod_file_paths_vector;

		int difference;
		bool found;
		bool allow_ciu = false;

		//if (wait_for_files == false) {
		//	mod_file_paths_vector.push_back(UTF2Wide(argv[1], CP_ACP));
		//	hq2x = Wide2UTF(UTF2Wide(argv[2], CP_ACP));
		//} else

		if (!hq2x.empty()) {
			hq2x.clear();
			hq2x.shrink_to_fit();
		}
		
		if (argc < 2) { // If there are no files dragged on IAMI
			std::cout << "\nSelect mod files: ";
			mod_file_paths_vector = CommonItemOpenFiles(L"Select mod files", FILTERS_MOD, FILTERS_MOD_SIZE);
			for (size_t boo = 0; boo < mod_file_paths_vector.size(); ++boo) {
				std::cout << Wide2UTF(mod_file_paths_vector[boo]) << '\n';
			}
		}
		else {
			std::cout << "Mods to be installed:" << std::endl;
			for (int i = 1; i < argc; ++i) {
				if (strcmp(u8"-wad", argv[i]) == 0 && i+1 < argc) {
					//std::cout << "WAD file: " << argv[i+1] << std::endl;
					hq2x = Wide2UTF(UTF2Wide(argv[i + 1], CP_ACP));
					i++;
					continue;
				}
				std::cout << argv[i] << std::endl;
				mod_file_paths_vector.push_back(UTF2Wide(argv[i], CP_ACP));
			}
		}
		if (hq2x.empty()) {
			std::cout << "Select the game's WAD file: ";
			hq2x = Wide2UTF(CommonItemOpenFile(L"Select a WAD file in the game folder", FILTERS_DAT, FILTERS_DAT_SIZE));
		}
		std::cout << "WAD File:" << hq2x << '\n';
		//hq2x = OpenFileWindow("Game data (*.dat;*.dat.hq2x)\0*.DAT;*.HQ2X\0All Files\0*.*\0", "Select .dat.hq2x in the game folder (if there is no, select .dat)");
		

		for (size_t processing_file = 0; processing_file < mod_file_paths_vector.size(); ++processing_file) {
			mod_file_path = Wide2UTF(mod_file_paths_vector[processing_file]);

			std::filesystem::path fs_path(UTF2Wide(mod_file_path));
#ifdef USE_PATH_LOGGING
			FILE* path_output_test = nullptr;
			fopen_s(&path_output_test, "IAMI_path_output_test_UTF16.txt", "ab");
			if (path_output_test != nullptr) {
				for (int i = 0; fs_path.c_str()[i] != L'\0'; ++i) {
					fwrite(fs_path.c_str() + i, sizeof(WCHAR), 1, path_output_test);
				}
				fputwc(L'\n', path_output_test);
				fclose(path_output_test);
			}
#endif
			mod_file_f.open(fs_path, std::ios::binary);
			if (!mod_file_f) {
				std::cerr << "[Error] Can't open \"" << mod_file_path << "\".\nTry running the tool as administrator or move the file to different directory.\n";
				callPause();
				exit(1);
			}
			std::cout << "Reading mod information...\n";
			mod_file_f.read(reinterpret_cast<char*>(&mod_version), sizeof(uint32_t));
			if (mod_version == 0xB501FA86) {//CIU 4cc
				allow_ciu = true;
			}
			mod_file_f.seekg(4);
			mod_file_f.read(reinterpret_cast<char*>(&mod_version), sizeof(uint32_t));
			if (mod_version != 0x6799186F) {//IAMI 4сс
				std::cerr << "[Error] Mod file is incorrect.\n";
				callPause();
				exit(2);
			}

			uintmax_t mod_file_size = std::filesystem::file_size(fs_path);

			//Create temp decursed file
			std::cout << "Reading content...\n";
			mod_file_f.seekg(-4, std::ios::end);
			mod_file_f.read(reinterpret_cast<char*>(&target_crc), sizeof(uint32_t));
			target_crc = decursed(target_crc, mod_file_size - 4) - 0x93E56B02;
			DLOG("Decursing mod_file");
			DLOG("Creating temp file");
			std::fstream temp_mod_file(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp", std::ios::out | std::ios::in | std::ios::trunc | std::ios::binary);
			if (!temp_mod_file) {
				std::cout << "Can't create temporary file: either %TEMP% is unavailable or requires administrator permission.\n";
				callPause("Press Enter to exit...");
				exit(1);
			}

			mod_file_f.seekg(0);
			CopyFilePart(mod_file_f, temp_mod_file, mod_file_size, 0);

			DLOG("Decursed!");

#ifdef DEBUG_OUTPUT
			temp_mod_file.flush();//Juuuuust in case I have to check it with HxD
			//callPause();
#endif
			mod_file_f.close();
			DLOG("Temp file flushed, mod file closed.");


			//Check CRC [NEW]
			DLOG("target_crc = " + std::to_string(target_crc));
			uint32_t mod_crc = CRC32(temp_mod_file, mod_file_size - 4);
			DLOG("mod_crc = " + std::to_string(mod_crc));

			if (target_crc != mod_crc) {
				std::cerr << "[Error] This mod file is corrupted.\n";
				callPause();
				temp_mod_file.close();
				std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
				std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
				std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");
				exit(0);
			}

			//Display data [NEW]
			temp_mod_file.seekg(0x8);
			temp_mod_file.read(reinterpret_cast<char*>(&mod_version), sizeof(uint32_t));
			temp_mod_file.read(reinterpret_cast<char*>(&mod_files_amount), sizeof(uint32_t));
			mod_name.clear();
			temp_mod_file.seekg(0x10);
			FReadStr(mod_name, temp_mod_file, 0x50);
			mod_description.clear();
			temp_mod_file.seekg(0x60);
			FReadStr(mod_description, temp_mod_file, 0x100);

			std::cout << "\n\n[Mod information]\n"
				"Name: " << mod_name <<
				"\nVersion: " << mod_version <<
				"\n\n[Description]\n" << mod_description <<
				"\n\n";

			for (;;) {
				std::string inpt;
				std::cout << "Do you want to install this mod? (y/n): ";
				std::getline(std::cin, inpt);
				
				if (inpt == "n") {
					std::cout << "\nInstallation cancelled.\n";
					callPause();
					temp_mod_file.close();
					std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
					std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
					std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");
					exit(0);
				}
				if (inpt == "y") { break; }
				std::cout << "\r";
			}

			//Open hq2x in read mode
			std::cout << "\nOpening the \"" << hq2x << "\".\n";
			fs_path = UTF2Wide(hq2x);
			ifile.open(fs_path, std::ios::in | std::ios::binary);
			if (!ifile) {
				std::cerr <<
					"\n[Error] Can't open \"" << hq2x << "\".\n"
					"Try running the tool as administrator or move the file to different directory.\n";
				callPause();
				temp_mod_file.close();
				std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
				std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
				std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");
				exit(1);
			}
			hq2x_size = std::filesystem::file_size(fs_path);

			std::fstream temp_hq2x_file(TEMP_PATH + L"DAT.iamitemp", std::ios::out | std::ios::in | std::ios::trunc | std::ios::binary);
			CopyFilePart(ifile, temp_hq2x_file, hq2x_size);
			ifile.close();


			//Which hq2x type [NEW]
			temp_hq2x_file.seekg(0);
			temp_hq2x_file.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
			if (files_amount == 0x20455655) {//CIU v35+ WAD4 "UVE " line
				hq2x_type = '4';
			}
			else if (files_amount >= 0 && files_amount <= 10) {//old CI4-all HD versions + old CIU
				hq2x_type = '3';
			}
			else {
				temp_hq2x_file.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
				if (files_amount == 0x2F786667 ||
					files_amount == 0x2F786673 ||
					files_amount == 0x6973756D ||
					files_amount == 0x74737973) {// "gfx/" "sfx/" "musi" "syst"
					hq2x_type = '1';
				}
				else {
					hq2x_type = '2';
				}
			}

			//Read files amount
			if (hq2x_type == '3') {
				temp_hq2x_file.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
			}
			else if (hq2x_type == '4') {
				if (allow_ciu == false) {
					std::cerr << "[Error] This mod is not compatible with Chicken Invaders Universe.\n";
					callPause();
					temp_hq2x_file.close();
					temp_mod_file.close();
					std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
					std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
					std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");
					exit(5);
				}
				temp_hq2x_file.seekg(0xC);
				temp_hq2x_file.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
			}
			else {
				temp_hq2x_file.seekg(0);
				temp_hq2x_file.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
			}

			if (full_backup) {
				std::cout << "Creating a backup file (Will be saved near selected .dat/.dat.hq2x).\n";
				std::filesystem::path fs_path_backup(fs_path);
				fs_path_backup += ".backup";
				ofile.open(fs_path_backup, std::ios::out | std::ios::trunc | std::ios::binary);
				if (!ofile) {
					std::cerr <<
						"[Error] Can't create backup file.\n"
						"Try running the tool as administrator.\n";
					callPause();
					temp_hq2x_file.close();
					temp_mod_file.close();
					std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
					std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
					std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");
					exit(1);
				}
				std::cout << "Saving...\n";
				temp_hq2x_file.seekg(0);
				CopyFilePart(temp_hq2x_file, ofile, hq2x_size);
				ofile.close();
				std::cout << "Done!\n";
			}
			else {
				std::cout << "Full backup is disabled.\n\n";
			}


			if (hq2x_type == '3' || hq2x_type == '2' || hq2x_type == '1') {
				for (size_t queue = 0; queue < mod_files_amount; ++queue) {
					printf("____\n");
					temp_mod_file.seekg(0x160 + 0x60 * queue);
					modded_file_path.clear();
					FReadStr(modded_file_path, temp_mod_file, 0x58);
					temp_mod_file.seekg(0x1B8 + 0x60 * queue);
					temp_mod_file.read(reinterpret_cast<char*>(&modded_file_pointer), sizeof(uint32_t));
					temp_mod_file.read(reinterpret_cast<char*>(&modded_file_size), sizeof(uint32_t));

					DLOG(
						"modded_file_path: " + modded_file_path +
						"\nmodded_file_pointer: " + std::to_string(modded_file_pointer) +
						"\nmodded_file_size: " + std::to_string(modded_file_size));

					found = false;
					str = modded_file_path;
					res.clear();
					if (hq2x_type == '1') {
						res = str;
					}
					else {
						res = str_xor(str);
					}
					DLOG("Looking for: " + str);

					uint32_t f_num = 0;//number of the file we need, in the header
					while (f_num < files_amount) {
						if (hq2x_type == '3')
							h_file_offset = 0x8 + 0x5c * size_t(f_num);//file in the header offset
						else
							h_file_offset = 0x4 + 0x58 * size_t(f_num);//file in the header offset
						str.clear();
						temp_hq2x_file.seekg(h_file_offset);
						if (hq2x_type == '1') {
							FReadStr(str, temp_hq2x_file, 0x50);
						}
						else {
							FReadStr(str, temp_hq2x_file, 0x50, 0xCC);
						}
						if (res == str) {
							DLOG("Found!");
							found = true;
							break;
						}
						f_num++;
					}

					if (found) {
						temp_hq2x_file.seekg(h_file_offset + 0x50);
						temp_hq2x_file.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));
						temp_hq2x_file.read(reinterpret_cast<char*>(&file_size), sizeof(uint32_t));
						DLOG("file_pointer: " + std::to_string(file_pointer) +
							"\nfile_size: " + std::to_string(file_size));
					}
					else {
						std::cerr << "\n[Error] Can't find file \"" << modded_file_path << "\" in the game.\n";
						for (;;) {
							if (skip_missing_paths) {
								std::cout << "Skipping...\n";
								break;
							}
							std::string inpt;
							std::cout << "Do you want to continue the installation? (y/all/n): ";
							std::getline(std::cin, inpt);
							if (inpt == "y") {
								break;
							}
							if (inpt == "n") {
								std::cout << "\nThe installation was cancelled.\n";
								callPause();
								temp_hq2x_file.close();
								temp_mod_file.close();
								std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
								std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
								std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");
								exit(0);
							}
							if (inpt == "all") {
								skip_missing_paths = true;
								break;
							}
							else {
								std::cout << "\r";
							}
						}
						std::cout << "\n\n";
						continue;
					}

					//temp_hq2x_file.flush();
					if (!full_size) {
						temp_hq2x_file.seekg(0, std::ios::end);
						full_size = temp_hq2x_file.tellg();//full hq2x size
					}
					if (!erase_point)
						erase_point = full_size;
					new_file_size = modded_file_size;


					//SAME FILE CHECK
					if (file_size == new_file_size) {
						bool same = false;
						uint32_t CRC_old = CRC32(temp_hq2x_file, file_size, file_pointer);
						uint32_t CRC_new = CRC32(temp_mod_file, new_file_size, modded_file_pointer);
						if (CRC_new == CRC_old) {
							std::cout << "Files are the same. Skipping...\n";
							same = true;
						}
						if (same) {
							modded_file_path.clear();
							continue;
						}
					}

					difference = new_file_size - file_size;
					DLOG("Difference: " + std::to_string(difference));
					//erase_point = new_file_size + erase_point - file_size;
					//erase_point += difference;

					temp_hq2x_file.seekp(h_file_offset + 0x50 + 4);
					temp_hq2x_file.write(reinterpret_cast<char*>(&new_file_size), sizeof(uint32_t));
					if (hq2x_type == '3') {
						temp_hq2x_file.write(reinterpret_cast<char*>(&new_file_size), sizeof(uint32_t));
					}
					std::fstream temp_hq2x_therest_file(TEMP_PATH + L"DAT2.iamitemp", std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
					temp_hq2x_file.seekg(file_pointer + file_size);
					CopyFilePart(temp_hq2x_file, temp_hq2x_therest_file, full_size - file_pointer - file_size);

					temp_mod_file.seekg(modded_file_pointer);
					temp_hq2x_file.seekp(file_pointer);
					CopyFilePart(temp_mod_file, temp_hq2x_file, new_file_size);

					temp_hq2x_therest_file.seekg(0);
					CopyFilePart(temp_hq2x_therest_file, temp_hq2x_file, full_size - file_pointer - file_size);
					temp_hq2x_therest_file.close();
					erase_point = temp_hq2x_file.tellp();
					DLOG("Erase point: " + std::to_string(erase_point));

					for (size_t i = size_t(f_num) + 1; i < files_amount; i++) {
						if (hq2x_type == '3') {
							temp_hq2x_file.seekg(0x5C * i + 0x58);
						}
						else {
							temp_hq2x_file.seekg(0x58 * i + 0x54);
						}
						temp_hq2x_file.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));
						file_pointer += difference;
						if (hq2x_type == '3') {
							temp_hq2x_file.seekp(0x5C * i + 0x58);
						}
						else {
							temp_hq2x_file.seekp(0x58 * i + 0x54);
						}
						temp_hq2x_file.write(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));
					}

					if (last_backup == true) {
						std::cout << "Creating last backup.\n";
						std::filesystem::path fs_path_last(fs_path);
						fs_path_last += ".last";
						ofile.open(fs_path_last, std::ios::out | std::ios::trunc | std::ios::binary);
						if (!ofile) {
							std::cerr << "[Error] Can't create backup of the last successful replacement.\n"
								"If you're trying to create the file in Program Files folder, run the tool as administrator.\n";
							callPause();
							temp_hq2x_file.close();
							temp_mod_file.close();
							std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
							std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
							std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");
							exit(2);
						}
						temp_hq2x_file.seekg(0);
						CopyFilePart(temp_hq2x_file, ofile, erase_point);
						ofile.close();
						std::cout << "Saved as " << hq2x << ".last\n";
					}
					else {
						std::cout << "Last backup is disabled.\n";
					}
					modded_file_path.clear();
					full_size = erase_point;
					std::cout << "Files replaced: " << queue + 1 << " of " << mod_files_amount << ".\n";
				}
				std::cout << "Saving changes.\n";
				ofile.open(fs_path, std::ios::out | std::ios::trunc | std::ios::binary);
				if (!ofile) {
					std::cerr << "[Error] Can't overwrite the file.\n"
						"If you're trying to edit the file in Program Files folder, run the tool as administrator.\n";
					callPause();
					temp_hq2x_file.close();
					temp_mod_file.close();
					std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
					std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
					std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");
					exit(2);
				}
				//temp_hq2x_file.flush();
				temp_hq2x_file.seekg(0);
				CopyFilePart(temp_hq2x_file, ofile, erase_point);
				ofile.close();

				//Remove temp shit
				temp_hq2x_file.close();
				temp_mod_file.close();

				std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
				std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
				std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");

				std::cout << "Done!\n";
			}

#ifndef EXCLUDE_4
			if (hq2x_type == '4') {
				uint64_t format_check;
				uint32_t ver;//crc or version of table
				uint32_t crc;//crc from hq2x
				uint32_t table_files_amount = 0;
				bool name_exists;

				std::vector<uint8_t> ciu_table_v;
				std::filesystem::path fs_table_path(UTF2Wide(tool_path) + L"CIU.iami");
#ifdef DEBUG_OUTPUT

#endif
				std::ifstream ciu_table_f(fs_table_path, std::ios::binary);
				if (!ciu_table_f) {
					std::cerr << "[Error] Can't open " << tool_path + "CIU.iami" << ". Check the tool folder.\n";
					callPause();
					exit(1);
				}

				ciu_table_f.read(reinterpret_cast<char*>(&format_check), sizeof(format_check));
				if (format_check != 0x76951A46FEF9F28C) {//IAMI CIU
					std::cerr << "[Error] CIU.iami is corrupted. Please, get a new one or check if you are using the right one.\n";
					callPause();
					exit(2);
				}

				size_t table_size = std::filesystem::file_size(fs_table_path);
				ciu_table_f.seekg(0);
				ciu_table_v.resize(table_size);
				ciu_table_f.read(reinterpret_cast<char*>(ciu_table_v.data()), table_size);
				ciu_table_f.close();
				protection(ciu_table_v, ciu_table_v.size(), 0);

				VecRead(ver, ciu_table_v, 0);
				VecRead(table_files_amount, ciu_table_v, 0xC);

				// Read all iamt table entries
				std::vector<TableEntry> ciu_ram_table = {};
				for (int i = 0; i < table_files_amount; ++i) {
					size_t table_offset = 0x10 + 0x60 * i;
					str.clear();
					ver = 0;
					VecRead(str, ciu_table_v, table_offset, 0x5C, 0, 0x5C);
					VecRead(ver, ciu_table_v, table_offset + 0x5C);
					TableEntry entry = { str, ver };
					ciu_ram_table.emplace_back(entry);
				}
				ciu_table_v.clear();
				ciu_table_v.shrink_to_fit();

				// Read all WAD header entries
				std::vector<WAD4_Entry> ciu_ram_wad_table = {};
				for (int i = 0; i < files_amount; ++i) {
					std::uint32_t offset, size_uncompressed, size, hash;
					offset = 0x10 + 0x10 * i;//file in the header offset
					temp_hq2x_file.seekg(offset);
					temp_hq2x_file.read(reinterpret_cast<char*>(&hash), sizeof(uint32_t));
					temp_hq2x_file.read(reinterpret_cast<char*>(&offset), sizeof(uint32_t));
					temp_hq2x_file.read(reinterpret_cast<char*>(&size_uncompressed), sizeof(uint32_t));
					temp_hq2x_file.read(reinterpret_cast<char*>(&size), sizeof(uint32_t));
					WAD4_Entry entry = { hash, offset, size_uncompressed, size };
					ciu_ram_wad_table.emplace_back(entry);
				}

				int h_file_idx = -1;
				for (size_t queue = 0; queue < mod_files_amount; ++queue) {
					printf("____\n");
					modded_file_path.clear();

					temp_mod_file.seekg(0x160 + 0x60 * queue);
					FReadStr(modded_file_path, temp_mod_file, 0x58);
					temp_mod_file.seekg(0x1B8 + 0x60 * queue);
					temp_mod_file.read(reinterpret_cast<char*>(&modded_file_pointer), sizeof(uint32_t));
					temp_mod_file.read(reinterpret_cast<char*>(&modded_file_size), sizeof(uint32_t));
					DLOG(
						"modded_file_path: " + modded_file_path +
						"\nmodded_file_pointer: " + std::to_string(modded_file_pointer) +
						"\nmodded_file_size: " + std::to_string(modded_file_size));

					found = false;
					name_exists = false;//in the table
					res.clear();
					res = modded_file_path;//res = path
					uint32_t f_num = 0;//number of the file which is needed in the header

					DLOG("Looking for: " + res);

					//find name in the table [NEW]
					//size_t table_offset = 0;
					//for (size_t i = 0; i < table_files_amount; ++i) {
					//	table_offset = 0x10 + 0x60 * i;
					//	str.clear();
					//	VecRead(str, ciu_table_v, table_offset, 0x5C, 0, 0x5C);
					//	if (res == str) {
					//		name_exists = true;
					//		break;
					//	}
					//}

					// Find name in the table [The NEWER]
#ifdef DEBUG_OUTPUT
					DLOG("Hi! I am the debug thing. If I don't get a \"Bye\" message right after, I was not able to find the modded_file_path in the CIU.iami.\n");
#endif
					for (size_t i = 0; i < table_files_amount; ++i) {
#ifdef DEBUG_OUTPUT
						printf("[DEBUG] @%s ?= #%s\n", ciu_ram_table[i].path.c_str(), res.c_str());
#endif
						if (ciu_ram_table[i].path == res) {
							name_exists = true;
							ver = ciu_ram_table[i].hash_djb2;
#ifdef DEBUG_OUTPUT
							DLOG("Bye! I found the path!\n");
#endif
						}
					}

					//find name in the table
					//for (uint64_t i = 0; i < table_files_amount; i++) {
					//	str.clear();
					//	temp_off = 0;
					//	do {
					//		ciu_table.seekg(0x10 + 0x60 * i + temp_off);
					//		c = ciu_table.get();
					//		c = decursed(c, 0x10 + 0x60 * i + temp_off);
					//		temp_off++;
					//		str += c;
					//	} while (c == res[temp_off - 1] && c != '\0' && temp_off < res.size());
					//	if (str == res) {
					//		ciu_table.seekg(0x10 + 0x60 * i);
					//		name_exists = true;
					//		break;
					//	}
					//}

					if (name_exists == false) {
						DLOG("Could not find the name. Reading as a hash...\n");
						ver = strtoul(res.c_str(), NULL, 16);
					}

					//find crc(ver) in the hq2x
					while (f_num < files_amount) {
						//h_file_offset = 0x10 + 0x10 * size_t(f_num);//file in the header offset

						//temp_hq2x_file.seekg(h_file_offset);
						//temp_hq2x_file.read(reinterpret_cast<char*>(&crc), sizeof(uint32_t));


						//VecRead(crc, hq2x_v, h_file_offset);

						//ifile.seekg(h_file_offset);
						////Read the crc of file
						//ifile.read(reinterpret_cast<char*>(&crc), sizeof(crc));

						if (ciu_ram_wad_table[f_num].hash_djb2 == ver) {
							DLOG("Found!");
							found = true;
							h_file_offset = 0x10 + 0x10 * size_t(f_num);//file in the header offset
							h_file_idx = f_num;
							break;
						}
						f_num++;
					}
					if (found) {
						//temp_hq2x_file.seekg(h_file_offset + 4);
						//temp_hq2x_file.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));
						//temp_hq2x_file.read(reinterpret_cast<char*>(&file_size), sizeof(uint32_t));
						file_pointer = ciu_ram_wad_table[h_file_idx].file_ptr;
						file_size = ciu_ram_wad_table[h_file_idx].size;

						//VecRead(file_pointer, hq2x_v, h_file_offset + 4);
						//VecRead(file_size, hq2x_v, h_file_offset + 8);
						DLOG("file_pointer: " + std::to_string(file_pointer) +
							"\nfile_size: " + std::to_string(file_size));

						//ifile.seekg(h_file_offset + 0x4);
						//ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));//poiner value
						//ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));//size value
					}
					else {
						std::cerr << "\n[Error] Can't find file \"" << modded_file_path << "\" in the header.\n";
						for (;;) {
							if (skip_missing_paths) {
								std::cout << "Skipping...\n";
								break;
							}
							std::string inpt;
							std::cout << "Do you want to continue the installation? (y/all/n): ";
							std::getline(std::cin, inpt);
							if (inpt == "y")
								break;
							if (inpt == "n") {
								std::cout << "\nThe installation was cancelled.\n";
								callPause();
								temp_hq2x_file.close();
								temp_mod_file.close();
								std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
								std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
								std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");
								exit(0);
							}
							if (inpt == "all") {
								skip_missing_paths = true;
								break;
							}
							else {
								std::cout << "\r";
							}
						}
						std::cout << "\n\n";
						continue;
					}


					if (!full_size) {
						temp_hq2x_file.seekg(0, std::ios::end);
						full_size = temp_hq2x_file.tellg();//full hq2x size
					}
					if (!erase_point) {
						erase_point = full_size;
					}
					new_file_size = modded_file_size;
					DLOG("new_file_size="+std::to_string(new_file_size));

					//std::vector<uint8_t> part_b(hq2x_v.begin() + file_pointer + file_size, hq2x_v.end());//go to the end of the replaceable file
					//std::vector<uint8_t> part_a(mod_file.begin() + modded_file_pointer, mod_file.begin() + modded_file_pointer + new_file_size);//new file

					//ifile.seekg(0, std::ios::end);
					//full_size = ifile.tellg();//full hq2x size


					//Check if files are the same
					//if (file_size == new_file_size) {
					//	bool same = false;
					//	for (uint64_t i = 0; i < file_size; ++i) {
					//		if (mod_file[modded_file_pointer + i] != hq2x_v[file_pointer + i]) { break; }
					//		if ((i == file_size - 1) && (mod_file[modded_file_pointer + i] == hq2x_v[file_pointer + i])) {
					//			std::cout << "Files are the same, skipping...\n";
					//			same = true;
					//		}
					//	}
					//	if (same) {
					//		part_a.clear();
					//		part_a.shrink_to_fit();
					//		part_b.clear();
					//		part_b.shrink_to_fit();
					//		modded_file_path.clear();
					//		continue;
					//	}
					//}

					//SAME FILE CHECK
					if (file_size == new_file_size) {
						bool same = false;
						uint32_t CRC_old = CRC32(temp_hq2x_file, file_size, file_pointer);
						uint32_t CRC_new = CRC32(temp_mod_file, new_file_size, modded_file_pointer);
						if (CRC_new == CRC_old) {
							std::cout << "Files are the same. Skipping...\n";
							same = true;
						}
						if (same) {
							modded_file_path.clear();
							continue;
						}
					}
					difference = new_file_size - file_size;
					DLOG("Difference: " + std::to_string(difference));
					//erase_point = (file_pointer + new_file_size) + (full_size - file_pointer - file_size);
					DLOG("Erase point: " + std::to_string(erase_point));

					temp_hq2x_file.seekp(h_file_offset + 8);
					temp_hq2x_file.write(reinterpret_cast<char*>(&new_file_size), sizeof(uint32_t));
					temp_hq2x_file.write(reinterpret_cast<char*>(&new_file_size), sizeof(uint32_t));
					ciu_ram_wad_table[h_file_idx].size = new_file_size;
					ciu_ram_wad_table[h_file_idx].size_uncompressed = new_file_size;
					//VecWrite(hq2x_v, h_file_offset + 0x8, &new_file_size);
					//VecWrite(hq2x_v, h_file_offset + 0xC, &new_file_size);

					std::fstream temp_hq2x_therest_file(TEMP_PATH + L"DAT2.iamitemp", std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
					temp_hq2x_file.seekg(file_pointer + file_size);
					CopyFilePart(temp_hq2x_file, temp_hq2x_therest_file, full_size - file_pointer - file_size);

					temp_mod_file.seekg(modded_file_pointer);
					temp_hq2x_file.seekp(file_pointer);
					CopyFilePart(temp_mod_file, temp_hq2x_file, new_file_size);
					//VecWrite(hq2x_v, file_pointer, part_a, new_file_size);

					temp_hq2x_therest_file.seekg(0);
					CopyFilePart(temp_hq2x_therest_file, temp_hq2x_file, full_size - file_pointer - file_size);
					//VecWrite(hq2x_v, file_pointer + new_file_size, part_b, full_size - file_pointer - file_size);
					temp_hq2x_therest_file.close();
					erase_point = temp_hq2x_file.tellp();
					DLOG("Erase point: " + std::to_string(erase_point));

					//ifile.seekg(file_pointer);
					//ifile.write(part_a, new_file_size);//put new file
					//ifile.write(part_b, full_size - file_pointer - file_size);

					for (size_t i = size_t(f_num) + 1; i < files_amount; ++i) {
						//VecRead(file_pointer, hq2x_v, 0x10 * i + 0x14);

						temp_hq2x_file.seekg(0x10 * i + 0x14);
						temp_hq2x_file.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));
						file_pointer += difference;

						//VecWrite(hq2x_v, 0x10 * i + 0x14, &file_pointer);
						temp_hq2x_file.seekp(0x10 * i + 0x14);
						temp_hq2x_file.write(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));

						ciu_ram_wad_table[i].file_ptr += difference;
						//ifile.seekg(-4, std::ios::cur);
						//ifile.write((char*)&file_pointer, 4);
					}

					if (last_backup) {
						std::cout << "Creating last backup.\n";
						std::filesystem::path fs_path_last(fs_path);
						fs_path_last += ".last";
						ofile.open(fs_path_last, std::ios::out | std::ios::trunc | std::ios::binary);
						if (!ofile) {
							std::cerr <<
								"[Error] Can't create backup of the last successful replacement.\n"
								"If you're trying to create the file in Program Files folder, run the tool as administrator.\n";
							callPause();
							temp_hq2x_file.close();
							temp_mod_file.close();
							std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
							std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
							std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");
							exit(2);
						}
						temp_hq2x_file.seekg(0);
						CopyFilePart(temp_hq2x_file, ofile, erase_point);
						ofile.close();
						std::cout << "Saved as " << hq2x << ".last\n";
					}
					else {
						std::cout << "Last backup is turned off by config.\n";
					}
					modded_file_path.clear();
					full_size = erase_point;
					std::cout << "Files replaced: " << queue + 1 << " of " << mod_files_amount << ".\n";
				}

				DLOG("erase_point=" + std::to_string(erase_point));
				if (erase_point != 0) {
					std::cout << "Saving changes.\n";
					ofile.open(fs_path, std::ios::out | std::ios::trunc | std::ios::binary);
					if (!ofile) {
						std::cerr << "[Error] Can't overwrite the file.\n"
							"If you're trying to edit the file in Program Files folder, run the tool as administrator.\n";
						callPause();
						temp_hq2x_file.close();
						temp_mod_file.close();
						std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
						std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
						std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");
						exit(2);
					}
					temp_hq2x_file.seekg(0);
					CopyFilePart(temp_hq2x_file, ofile, erase_point);
					ofile.close();
					std::cout << "Done!\n";
				}
				else {
					std::cout << "No changes were made.\n";
				}

				//Remove temp shit
				temp_hq2x_file.close();
				temp_mod_file.close();

				std::filesystem::remove(TEMP_PATH + std::to_wstring(target_crc) + L".iamitemp");
				std::filesystem::remove(TEMP_PATH + L"DAT.iamitemp");
				std::filesystem::remove(TEMP_PATH + L"DAT2.iamitemp");
			}
		}

#endif
		std::cout << "\nMod installation is finished :)\a\n";
	}

	//The following is required for label to work correctly
	mode = '0';
	mode_selected = false;
	wait_for_files = true;
	goto LABEL_MENU;
	return 0;
}