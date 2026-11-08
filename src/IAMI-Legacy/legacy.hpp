#pragma once

#include <vector>

extern const unsigned char ek[];
extern const unsigned long ek_size;
extern const unsigned long crc_seed;
extern const unsigned long crc_add;

void protection(char* buf, size_t buf_size, uintmax_t start_offset);
void protection(std::vector<uint8_t>& buf, size_t buf_size, size_t start_offset);
uint32_t decursed(uint32_t value, uintmax_t start_offset);
uint8_t decursed(uint8_t value, uint64_t start_offset);