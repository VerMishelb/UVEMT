#include "Functions.h"

#define NOMINMAX
//#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <commdlg.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace Func {
	std::string Wide2UTF(const std::wstring& wstr, unsigned int codepage) {
		if (wstr.empty()) { return std::string(); }
		//Codepage, flags, pointer to source [don't use .data() because NULL string would cause something bad], char amount of source, pointer to dest, size (bytes) of dest, NULL, NULL
		//Returns the required size, in bytes, for the destination buffer indicated by lpMultiByteStr
		int size_needed = WideCharToMultiByte(codepage, 0, wstr.data(), (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);
		WideCharToMultiByte(codepage, 0, wstr.data(), (int)wstr.size(), strTo.data(), size_needed, NULL, NULL);
		return strTo;
	}

	std::wstring UTF2Wide(const std::string& str, unsigned int codepage) {
		if (str.empty()) { return std::wstring(); }
		int size_needed = MultiByteToWideChar(codepage, 0, str.data(), (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(codepage, 0, str.data(), (int)str.size(), wstrTo.data(), size_needed);
		return wstrTo;
	}
}

void BetterCallPause() {
	std::cout << "Press Enter to continue...\n";
	int c;
	while ((c = getchar()) != '\n' && c != EOF) {}
	c = getchar();
}

std::string str_xor(std::string a) {
	std::string res = a;
	for (uint32_t i = 0; i < a.length(); i++) {
		res[i] ^= 0xCC;
	}
	return res;
}

std::string int_to_str(uint32_t a, int T) {
	std::ostringstream b;
	b << "0x"
		<< std::setfill('0') << std::setw(uint64_t(T) * 2)
		<< std::hex << a;
	return b.str();
}

std::vector<std::string> Separate(std::string s)
{
	std::vector<std::string> line;
	std::string token;
	std::istringstream iss(s);
	while (getline(iss, token, '/')) {
		line.push_back(token);
	}
	return line;
}
std::vector<std::string> SeparateEx(std::string s, const char sep, int counter)
{
	std::vector<std::string> line;
	std::string token;
	int c = 0;
	std::istringstream iss(s);
	while (getline(iss, token, sep) && c < counter) {
		line.push_back(token);
		c++;
	}
	return line;
}

//crc check thing to put it's crc in the end of mod file and iami table
unsigned int CRC32(char* buf, unsigned long len) {
	unsigned long crc_table[256];
	unsigned long crc;
	for (int i = 0; i < 256; i++) {
		crc = i;
		for (int j = 0; j < 8; j++) {
			crc = crc & 1 ? (crc >> 1) ^ 0xEDB88320UL : crc >> 1;
		}
		crc_table[i] = crc;
	}
	crc = 0xFFFFFFFFUL;
	while (len--) {
		crc = crc_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
	}
	return crc ^ 0xFFFFFFFFUL;
}
char* protection(char* buf, uint64_t buf_size, char mode) {
	std::vector<uint8_t> ek = { 67, 177, 172, 181, 38, 215, 76, 33, 89, 213, 93, 29, 211, 51, 27, 116, 238, 50, 240, 169, 25, 228, 143, 124, 9, 243, 184, 129, 32 };
	if (mode == 'e')//'d'/'e'
	{
		for (uint64_t i = 0; i < buf_size; i++) {
			buf[i] += ek[i % 29];
		}
	}
	else if (mode == 'd') {
		for (uint64_t i = 0; i < buf_size; i++) {
			buf[i] -= ek[i % 29];
		}
	}
	return buf;
}
void IamiProtection(char* buf, uint64_t buf_size, uint64_t start_offset) {
	std::vector<uint8_t> ek = { 67, 177, 172, 181, 38, 215, 76, 33, 89, 213, 93, 29, 211, 51, 27, 116, 238, 50, 240, 169, 25, 228, 143, 124, 9, 243, 184, 129, 32 };
	for (uint64_t counter = 0; counter < buf_size; counter++) {
		buf[counter] -= ek[start_offset % ek.size()];
		start_offset++;
	}
}
std::vector<std::string> txt2vec(std::ifstream &f) {
	std::vector<std::string> v;
	std::string s;
	f.seekg(0);
	while (f) {
		getline(f, s);
		v.push_back(s);
	}
	v.pop_back();
	f.clear();
	return v;
}
uint32_t decursed(uint32_t value, uint64_t start_offset) {
	char arr[4];
	uint32_t r_value;
	memcpy(arr, &value, sizeof(uint32_t));
	IamiProtection(arr, 4, start_offset);
	memcpy(&r_value, arr, sizeof(uint32_t));
	return r_value;
}


//path has NO '/' at the end
std::vector<std::string> get_paths(std::string& path) {
	/*
	20.03.21
	returns paths as a vector of:
	[0] sfx/quiet.ogg
	[1] <full_path>/sfx/quiet.ogg
	*/

	std::vector<std::string> res;
	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(path)) {
		if (entry.is_regular_file()) {
			std::string s = entry.path().string();
			std::size_t path_size = path.size();
			std::string s_cut = s.substr(path_size + 1);
			std::size_t pos = 0;
			for (;;) {
				pos = s_cut.find('\\', pos);
				if (pos == std::string::npos) {
					break;
				}
				s_cut.replace(pos, size_t(1), "/");
				pos++;
			}

			res.push_back(s_cut);
			res.push_back(s);
		}
	}
	return res;
}

unsigned long djb2_hash(unsigned char* str) {
	unsigned long hash = 5381;
	int c;

	while (c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}



