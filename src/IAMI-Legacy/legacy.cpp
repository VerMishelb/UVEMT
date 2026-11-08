#include "legacy.hpp"

const unsigned char ek[] = { 67, 177, 172, 181, 38, 215, 76, 33, 89, 213, 93, 29, 211, 51, 27, 116, 238, 50, 240, 169, 25, 228, 143, 124, 9, 243, 184, 129, 32 };
const unsigned long ek_size = sizeof(ek);
const unsigned long crc_seed = 0xEDB88320UL;
const unsigned long crc_add = 0x93E56B02;

void protection(char* buf, size_t buf_size, uintmax_t start_offset) {
	for (size_t counter = 0; counter < buf_size; counter++) {
		buf[counter] -= ek[start_offset % ek_size];
		start_offset++;
	}
}

void protection(std::vector<uint8_t>& buf, size_t buf_size, size_t start_offset) {
	for (size_t counter = 0; counter < buf_size; ++counter) {
		buf[counter] -= ek[start_offset % ek_size];
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
	value -= reinterpret_cast<uint8_t>(ek[start_offset % ek_size]);
	return value;
}