#ifndef _Functions_h_
#define _Functions_h_

#include <string>
#include <vector>
#include <fstream>

namespace Func {//Functions
	//Use either std::string with CP_ACP = 0 codepage or std::wstring for file paths
	std::string Wide2UTF(const std::wstring& wstr, unsigned int codepage = 65001);
	std::wstring UTF2Wide(const std::string& str, unsigned int codepage = 65001);
};

void BetterCallPause();
std::string str_xor(std::string str);
std::string int_to_str(uint32_t a, int T);
std::vector<std::string> Separate(std::string s);
std::vector<std::string> SeparateEx(std::string s, const char sep, int counter);

/*
* These are OLD WinAPI functions, introduced before Vista.
* TODO: Check if char* == char []
*/
std::string OpenFileWindow(const char formats[]);
std::string SaveFileWindow(const char formats[], const char def_format[]);

unsigned int CRC32(char* buf, unsigned long len);
char* protection(char* buf, uint64_t buf_size, char mode);
void IamiProtection(char* buf, uint64_t buf_size, uint64_t start_offset);
std::vector<std::string> txt2vec(std::ifstream& f);
uint32_t decursed(uint32_t value, uint64_t start_offset);
std::vector<std::string> get_paths(std::string& path);
unsigned long djb2_hash(unsigned char* str);


#endif