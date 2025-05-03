#include "Modes.h"
#include "Functions.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
#include <codecvt> // Removed in C++26
#include "asciiDickClark.h"

IamtConfig g_iamt_config{};
int g_argc = 0;
char** g_argv = nullptr;
const std::string size_str[4] = { "B","KB","MB", "GB" };//Bytes, Kilobytes, Megabytes

// Done
void readIamtConfig() {
	g_iamt_config.tool_folder = std::string(g_argv[0]).substr(
		0, std::string(g_argv[0]).find_last_of("\\/") + 1
	);
	std::srand(static_cast<unsigned int>(time(0)));

	std::ifstream config(g_iamt_config.tool_folder + "config.cfg");
	if (!config) {
		std::cout << "[Warning] Can't find config.cfg in tool folder! Tool will use default values.\n"
			"Add a \'config.txt\' file next to IAMT.exe with the following:\n"
			"full_backup=1\n"
			"last_backup=0\n"
			"loading_timeout=10\n"
			;
	}
	else {
		std::cout << "Reading config.cfg\n";
		std::string config_lines[4] = {
			"full_backup",
			"last_backup",
			"loading_timeout",
			"make_tableless_mods"
		}; //lines in config to search

		std::string str{};
		while (std::getline(config, str)) {
			if (str.empty()) { continue; }

			// Not in the same line with str.empty() because it will try to get an impossible symbol
			// in case of empty string, which results in crash.
			else if (str.at(0) == '#' || str.at(0) == ';') { continue; }

			else if (str.find(config_lines[0]) != std::string::npos) {
				str = str.substr(str.find('=', str.find(config_lines[0])) + 1); // 0, 1
				g_iamt_config.fullBackup = stoi(str);
				continue;
			}
			else if (str.find(config_lines[1]) != std::string::npos) {
				str = str.substr(str.find('=', str.find(config_lines[1])) + 1); // 0, 1
				g_iamt_config.lastBackup = stoi(str);
				continue;
			}
			else if (str.find(config_lines[2]) != std::string::npos) {
				str = str.substr(str.find('=', str.find(config_lines[2])) + 1); // int
				g_iamt_config.loading_timeout = stoi(str);
				if (g_iamt_config.loadingTimeout < 1) { g_iamt_config.loading_timeout = 10; }
				continue;
			}
			else if (str.find(config_lines[3]) != std::string::npos) {
				str = str.substr(str.find('=', str.find(config_lines[1])) + 1); // 0, 1
				g_iamt_config.makeTablelessMods = stoi(str);
				continue;
			}
		}
	}
}

// Done
void readArguments() {
	if (g_argc > 1) {
		if (strcmp(g_argv[1], "-d") == 0) { modeDecode(); return; }
		if (strcmp(g_argv[1], "-e") == 0) { modeEncode(); return; }
		if (strcmp(g_argv[1], "-r") == 0 && g_argc > 4) { modeWadRepack(); return; }
		if (strcmp(g_argv[1], "-s") == 0 && g_argc > 2) { modeHeaderInfo(); return; }
		if (strcmp(g_argv[1], "-ex") == 0) {
			bool wait_for_files = (g_argc <= 2);
			modeWadExtract(wait_for_files);
			return;
		}
#ifndef IAMT_AUDIO_ONLY
		if (strcmp(g_argv[1], "-u") == 0) { modeTableUpdate(); return; }
		if (strcmp(g_argv[1], "-ts") == 0) { modeTableInfo(); return; }
		if (strcmp(g_argv[1], "-te") == 0) { modeTableEdit(); return; }
		if (strcmp(g_argv[1], "-ti") == 0) { modeTableConvert(); return; }
		if (strcmp(g_argv[1], "-me") == 0) { modeModEdit(); return; }
#endif
		if (strcmp(g_argv[1], "-mc") == 0) {
			bool wait_for_files = (g_argc <= 2);
			modeModCreate(wait_for_files);
			return;
		}
	}
}

// Done?
void modeSelection() {

	std::string inpt;
	bool exit_selection = false;
	while (!exit_selection) {
		system("cls");
		std::cout << "VerMishelb's UVE Mod Tool (InterAction Mod Tool) " IAMT_VERSION " (" __DATE__ ")\nIf you found any bugs or crashes, please let me know in Discord (@vmlb)\nPlease note that InterAction studios IS NOT related to this tool development and won't help you if you corrupted your game files while doing mods.\n"
			"If you want to share your mod with others, DO NOT share your modded game files (.dat, .dat.hq2x, .dat.222x etc). Use mod creation isntead.\n"
#ifndef IAMT_AUDIO_ONLY
			"================\n"
			"!!! ATTENTION !!!\n"
			"SINCE IAMT v2.0 THE NORMAL VERSION CAN ONLY BE OBTAINED BY SEVERAL PEOPLE WHO MAKE MODS FOR CI OR EXTRACT FILES FOR WIKI.\n"
			"IF YOU DIDN'T OBTAIN THIS TOOL FROM Emerald, VerMishelb, Francis OR One-Winged Lunarian, LET US KNOW BY DISCORD IMMEDIATELY.\n"
			"================\n"
#endif
			;
		std::cout << "\rPlease, select mode you want to work with (you can also use letters instead: type \"r\" to enter repack mode):\n"
			"C - Show credits\n"
			"0|exit - Close the tool\n"
			"1|d - Decode bytes to string (-d)\n"
			"2|e - Encode string to bytes (-e)\n"
			"3|s - Show info about the header (-s)\n"
			"4|r - Repack DAT with new files (-r)\n"
			"5|ex - Extract files from DAT (-ex)\n"
			"6|u - Update .iamt_table (-u)\n"
#ifndef  IAMT_AUDIO_ONLY
			"7|ts - Show information about .iamt_table file (-ts)\n"
			"8|te - Edit .iamt_table file (-te)\n"
#endif
			"9|ti - Create .iami from .iamt_table (-ti)\n"
			"10|mc - Create mod file (-mc)\n"
#ifndef IAMT_AUDIO_ONLY
			"11|me - Edit mod file (-me)\n"
#endif
			">";

		std::getline(std::cin, inpt);
		if (inpt == "0" || inpt == "ciao" || inpt == "exit") {
			exit_selection = true;
			continue;
		}
		else if (inpt == "1" || inpt == "decode" || inpt == "d") {
			modeDecode();
			continue;
		}
		else if (inpt == "2" || inpt == "encode" || inpt == "e") {
			modeEncode();
			continue;
		}
		else if (inpt == "3" || inpt == "show" || inpt == "s") {
			bool wait_for_files = (g_argc <= 2);
			modeHeaderInfo(wait_for_files);
			continue;
		}
		else if (inpt == "4" || inpt == "repack" || inpt == "r") {
			modeWadRepack();
			continue;
		}
		else if (inpt == "5" || inpt == "extract" || inpt == "ex") {
			modeWadExtract();
			continue;
		}
		else if (inpt == "6" || inpt == "update" || inpt == "u") {
			modeTableUpdate();
			continue;
		}
		else if (inpt == "9" || inpt == "tconv" || inpt == "tiami" || inpt == "ti") {
			modeTableConvert();
			continue;
		}
#ifndef IAMT_AUDIO_ONLY
		else if (inpt == "7" || inpt == "tshow" || inpt == "ts") {
			modeTableInfo();
			continue;
		}
		else if (inpt == "8" || inpt == "tedit" || inpt == "te") {
			modeTableEdit();
			continue;
		}
		else if (inpt == "11" || inpt == "medit" || inpt == "me") {
			modeModEdit();
			continue;
		}
#endif
		else if (inpt == "10" || inpt == "mod" || inpt == "mc") {
			modeModCreate();
			continue;
		}
		else if (inpt == "C" || inpt == "c" || inpt == "credits" || inpt == "Credits") {
			modeCredits();
			continue;
		}
		else if (inpt == "dickclark") {
			system("cls");
			std::cout << "wat he returneds eg ain ?|?|?!?!??!evelen!?\n\n\n";
			std::cout << asciiDickClark << '\n';
			BetterCallPause();
			continue;
		}
	}
}

// Done
void modeCredits() {
	std::string credits =
		"\nInterAction Mod Tool " IAMT_VERSION " (" __DATE__ ")\n\n"
		"VerMishelb (VerMishelb#6318) - Coding, testing, main idea, ._ ., bomkin.\n"
		"One-Winged Lunarian - Testing, features ideas, interface ideas, epic modding.\n"
		"Emerald - Information about DAT files format, testing, motivation to start working on this, \"oh\".\n"
		"owocek - Great help with coding, giving the author chocky milk \"because he is awesome\" (his nickname starts from small \"o\" don't blame me).\n"
		"BoggY - Testing, finding the most terrible and invisible bug with vcredist.\n"
		"InterAction studios - Letting modding in their games be possible, help with table (maybe they didn't even notice how), best POGrammer ever.\n"
		"Okina (IceIris#7283) - Didn't bounce, meow, wah.\n"
		"Hale - Testing from down under and selling australiums for tea.\n"
		"Francis - Made the tool not for local use but OUR tool, blame them for IAMI icon, there is one more line for them but it's in iami so bruj.\n"
		"Reggy - Why it doesn't work only for you?!\n"
		"Orandza - Reward for being the best orange of the year. 10/10 would eat.\n"
		"Ramon - Stuck in a bowl of ramen.\n\n"
		"And everyone who used the tool before v3.0 release. Thanks to your feedback it became even more useful and handy than I expected.\n"
		"Take your :chocken:, because you are awesome!\n\n";

	printf("%s\n", credits.c_str());
	BetterCallPause();
}

// Done
void modeDecode() {
	std::cout <<
		"Decode mode.\n"
		"Enter the byte array you would like to read (use RMB to paste value).\n>";

	std::string temp_str;
	std::getline(std::cin, temp_str);

	std::cout << "\nErasing spaces between bytes...\n";
	std::string str;
	for (size_t i = 0; i < size(temp_str); i++) {
		if (temp_str[i] != ' ') {
			str += temp_str[i];
		}
	}

	std::string res;
	for (uint32_t i = 0; i < size(str); i += 2) {
		res += strtoul((str.substr(i, 2)).c_str(), NULL, 16);
	}

	res = str_xor(res);
	std::cout << "Result:\n" << res << '\n';
	for (uint32_t i = 0; i < res.length(); ++i) {
		// + forces it to be converted to (un)signed int
		std::cout << std::hex << +static_cast<uint8_t>(res[i]) << ((i + 1 < res.length()) ? ' ' : '\n');
	}
}

// Done
void modeEncode() {
	std::cout << "Encode mode.\nEnter the string you want to encode.\n>";
	std::string str;
	std::getline(std::cin, str);
	std::cout << "\nEncoding the string...\n";
	str = str_xor(str);
	std::cout << "Result:\n" << str << "\n";
	for (uint32_t i = 0; i < str.length(); i++) {
		std::cout << std::hex << +static_cast<uint8_t>(str[i]) << " ";
	}
	std::cout << '\n';
}

// Done
void modeHeaderInfo(bool wait_for_files) {
	std::cout << "Header info mode.\n";

	std::string hq2x{};
	if (wait_for_files == false) {
		hq2x = std::string(g_argv[2]);
	}
	else//If there are no files given
	{
		hq2x = OpenFileWindow(
			"InterAction WAD Format (.dat; .hq2x; .222x)\0*.HQ2X;*.DAT;*.222X;*.222X.*\0All Files\0*.*\0");
	}
	std::fstream ifile(hq2x, std::ios::in | std::ios::binary);
	if (!ifile) {
		std::cerr << "\n[Error] Can't open \"" << hq2x << "\".\n"
			"If you're trying to open the file from Program Files folder, run the tool from administrator.\n";
		BetterCallPause();
		exit(1);
	}
	std::cout << "Opened \"" << hq2x << "\".\n\n";





	//Which hq2x type

	uint32_t files_amount;
	ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
	char hq2x_type = '\0';
	if (files_amount == 0x20455655)//CIU v35 WAD4 "UVE "
	{
		hq2x_type = '4';
	}
	else if (files_amount >= 0 && files_amount <= 10)//old CI4-all HD versions + old CIU
	{
		hq2x_type = '3';
	}
	else
	{
		ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));//Reads 5-8 bytes, NOT 1-4!!!
		if (files_amount == 0x2F786667 ||
			files_amount == 0x2F786673 ||
			files_amount == 0x6973756D ||
			files_amount == 0x74737973)// "gfx/" "sfx/" "musi" "syst"
		{
			hq2x_type = '1';
		}
		else {
			hq2x_type = '2';
		}
	}

	//Read files amount
	if (hq2x_type == '3') {
		ifile.seekg(0x4);
		ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
	}
	else if (hq2x_type == '4') {
		ifile.seekg(0xC);
		ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
	}
	else {
		ifile.seekg(0);
		ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
	}

	//Header size
	uint32_t header_size;
	if (hq2x_type == '4') {
		header_size = 0x10 * (files_amount + 1);
	}
	else if (hq2x_type == '3') {
		header_size = 0x5c * files_amount + 0x8;
	}
	else {
		header_size = 0x58 * files_amount + 0x4;
	}

	if (hq2x_type == '3') {
		std::cout << "Files amount: " << files_amount << "\nHeader size: 0x" << std::hex << header_size
			<< "\n[Header OFFS] [File OFFS] (size) path\n";
		uint32_t files_counter = 0;
		for (uint32_t i = 0; i < files_amount; i++)
		{
			uint32_t h_file_offset;//Offset of the file in header
			h_file_offset = 0x8 + 0x5c * i;
			ifile.seekg(h_file_offset);

			//Name
			std::string str{};
			unsigned char c = 0xFF;
			uint64_t temp_off = 0;
			while (uint8_t(c) != 0xCC) {
				ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
				temp_off++;
				c = ifile.get();
				if (uint8_t(c) != 0xcc)
					str += c;
			}

			//Decode name
			str = str_xor(str);

			//File pointer
			ifile.seekg(uint64_t(h_file_offset) + 0x50);
			uint32_t file_pointer;//Offset of the file itself
			uint32_t file_size;
			ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));

			//File size
			ifile.seekg(uint64_t(h_file_offset) + 0x58);
			ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));
			int size_multiplier = 1;
			int size_str_i = 0;
			while (file_size > uint32_t(1024 * size_multiplier) && size_str_i < 2) {
				size_multiplier *= 1024;
				++size_str_i;
			}
			float file_size_f = float(file_size) / float(size_multiplier);//File size to display
			std::ostringstream size_display;
			std::ostringstream header_offset_display;
			std::ostringstream file_offset_display;


			size_display << "(" << round(file_size_f * 10) / 10 << size_str[size_str_i] << ")";
			header_offset_display << "[0x" << std::hex << h_file_offset << "]";
			file_offset_display << "[0x" << std::hex << file_pointer << "]";
#ifdef IAMT_AUDIO_ONLY
			if (str.find(".tga") == std::string::npos &&
				str.find(".preload") == std::string::npos &&
				str.find(".jpg") == std::string::npos &&
				str.find(".hmap") == std::string::npos &&
				str.find(".jpeg") == std::string::npos &&
				str.find(".txt") == std::string::npos)
#endif
				std::cout << std::setw(5) << std::left << std::dec << ++files_counter
				<< std::setw(9) << std::left << header_offset_display.str()
				<< std::setw(12) << std::left << file_offset_display.str()
				<< std::setw(13) << std::left << size_display.str() << str << '\n';
			size_display.str("");
			header_offset_display.str("");
			file_offset_display.str("");
			size_display.clear();
			header_offset_display.clear();
			file_offset_display.clear();
		}
	}
	else if (hq2x_type == '2') {
		std::cout << "Files amount: " << files_amount << "\nHeader size: 0x" << std::hex << header_size
			<< "\n[Header OFFS] [File OFFS] (size) path\n";
		uint32_t files_counter = 0;
		for (uint32_t i = 0; i < files_amount; i++) {
			uint32_t h_file_offset = 0x4 + 0x58 * i;
			ifile.seekg(h_file_offset);

			//Name
			std::string str{};
			unsigned char c = 0xFF;
			uint64_t temp_off = 0;
			while (uint8_t(c) != 0xCC) {
				ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
				temp_off++;
				c = ifile.get();
				if (uint8_t(c) != 0xcc)
					str += c;
			}

			//Decode name
			str = str_xor(str);

			//File pointer
			ifile.seekg(uint64_t(h_file_offset) + 0x50);
			uint32_t file_pointer = 0;
			ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));

			//File size
			ifile.seekg(uint64_t(h_file_offset) + 0x54);
			uint32_t file_size = 0;
			ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));
			int size_multiplier = 1;
			int size_str_i = 0;
			while (file_size > uint32_t(1024 * size_multiplier) && size_str_i < 2)
			{
				size_multiplier *= 1024;
				size_str_i += 1;
			}
			float file_size_f = float(file_size) / float(size_multiplier);//File size to display

			std::ostringstream size_display;
			std::ostringstream header_offset_display;
			std::ostringstream file_offset_display;

			size_display << "(" << round(file_size_f * 10) / 10 << size_str[size_str_i] << ")";
			header_offset_display << "[0x" << std::hex << h_file_offset << "]";
			file_offset_display << "[0x" << std::hex << file_pointer << "]";
#ifdef IAMT_AUDIO_ONLY
			if (str.find(".tga") == std::string::npos &&
				str.find(".preload") == std::string::npos &&
				str.find(".jpg") == std::string::npos &&
				str.find(".hmap") == std::string::npos &&
				str.find(".jpeg") == std::string::npos &&
				str.find(".txt") == std::string::npos)
#endif
				std::cout << std::setw(5) << std::left << std::dec << ++files_counter
				<< std::setw(9) << std::left << header_offset_display.str()
				<< std::setw(12) << std::left << file_offset_display.str()
				<< std::setw(13) << std::left << size_display.str() << str << '\n';
			size_display.str("");
			header_offset_display.str("");
			file_offset_display.str("");
			size_display.clear();
			header_offset_display.clear();
			file_offset_display.clear();
		}
	}
	else if (hq2x_type == '1') {
		std::cout << "Files amount: " << files_amount << "\nHeader size: 0x" << std::hex << header_size
			<< "\n[Header OFFS] [File OFFS] (size) path\n";
		int files_counter = 0;
		for (uint32_t i = 0; i < files_amount; i++) {
			uint32_t h_file_offset = 0x4 + 0x58 * i;
			ifile.seekg(h_file_offset);

			//Name
			std::string str{};
			unsigned char c = 0xFF;
			uint32_t temp_off = 0;
			while (uint8_t(c) != 0x00) {
				ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
				temp_off++;
				c = ifile.get();
				if (uint8_t(c) != 0x00) {
					str += c;
				}
			}

			//File pointer
			ifile.seekg(uint64_t(h_file_offset) + 0x50);
			uint32_t file_pointer = 0;
			ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));

			//File size
			ifile.seekg(uint64_t(h_file_offset) + 0x54);
			uint32_t file_size = 0;
			ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));
			int size_multiplier = 1;
			int size_str_i = 0;
			while (file_size > uint32_t(1024 * size_multiplier) && size_str_i < 2) {
				size_multiplier *= 1024;
				size_str_i += 1;
			}
			float file_size_f = float(file_size) / float(size_multiplier); // File size to display

			std::ostringstream size_display;
			std::ostringstream header_offset_display;
			std::ostringstream file_offset_display;

			size_display << "(" << round(file_size_f * 10) / 10 << size_str[size_str_i] << ")";
			header_offset_display << "[0x" << std::hex << h_file_offset << "]";
			file_offset_display << "[0x" << std::hex << file_pointer << "]";

			std::cout << std::setw(5) << std::left << std::dec << ++files_counter
				<< std::setw(9) << std::left << header_offset_display.str()
				<< std::setw(12) << std::left << file_offset_display.str()
				<< std::setw(13) << std::left << size_display.str() << str << '\n';
			size_display.str("");
			header_offset_display.str("");
			file_offset_display.str("");
			size_display.clear();
			header_offset_display.clear();
			file_offset_display.clear();
		}
	}
	if (hq2x_type == '4') {
		std::cout << "Chicken Invaders Universe requires additional table to display results right way.\nOpening \"CIU.iamt_table\".\n";

		std::ifstream ciu_table(g_iamt_config.tool_folder + "CIU.iamt_table", std::ios::binary);
		if (!ciu_table) {
			std::cerr << "[Error] Can't open CIU.iamt_table. Check tool folder.\n";
			BetterCallPause();
			exit(1);
		}
		uint64_t format_check;
		ciu_table.read(reinterpret_cast<char*>(&format_check), sizeof(format_check));
		if (format_check != 0x55494320544D4149) {
			std::cerr << "[Error] CIU table is corrupted! Please, get a new one or check if you use the right one.\n";
			BetterCallPause();
			exit(2);
		}

		uint32_t ver;//crc or version of table
		ciu_table.read(reinterpret_cast<char*>(&ver), sizeof(ver));//read CIU version
		std::cout << "CIU table version: " << std::dec << ver << '\n';
		uint32_t table_files_amount;
		ciu_table.read(reinterpret_cast<char*>(&table_files_amount), sizeof(table_files_amount));
		if (files_amount != table_files_amount) {
			std::cout << "Warning: CIU version can be different to CIU table version. Some files won't show names.\n";
		}
		std::cout << "Files amount: " << files_amount << "\nHeader size: 0x" << std::hex << header_size
			<< "\n[Header OFFS] [File OFFS] (size) path\n";
		int files_counter = 0;
		for (uint64_t i = 0; i < files_amount; i++) {
			std::uint32_t h_file_offset = 0x10 * (i + 1);
			ifile.seekg(h_file_offset);

			//Name
			uint32_t crc; // (actually djb2 hash) from hq2x
			ifile.read(reinterpret_cast<char*>(&crc), sizeof(crc));
			std::string str{};
			for (uint64_t j = 0; j < table_files_amount; j++) {
				ciu_table.seekg(0x10 + 0x60 * j + 0x5C); // go read crc from table
				ciu_table.read(reinterpret_cast<char*>(&ver), sizeof(ver));

				if (crc == ver) {
					ciu_table.seekg(0x10 + 0x60 * j);
					unsigned char c = 0xFF;
					uint64_t temp_off = 0;
					while (uint8_t(c) != 0x00) {
						ciu_table.seekg(0x10 + 0x60 * j + (temp_off));
						temp_off++;
						c = ciu_table.get();
						if (uint8_t(c) != 0x00)
							str += c;
					}
					break;
				}
				str = int_to_str(crc, 4);//Reads CRC
				ciu_table.clear();
			}

			//File pointer
			ifile.seekg(uint64_t(h_file_offset) + 0x4);
			uint32_t file_pointer = 0;
			ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));

			//File size
			ifile.seekg(uint64_t(h_file_offset) + 0xC);
			uint32_t file_size = 0;
			ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));
			int size_multiplier = 1;
			int size_str_i = 0;
			while (file_size > uint32_t(1024 * size_multiplier) && size_str_i < 2) {
				size_multiplier *= 1024;
				size_str_i += 1;
			}
			float file_size_f = float(file_size) / float(size_multiplier);//File size to display

			std::ostringstream size_display;
			std::ostringstream header_offset_display;
			std::ostringstream file_offset_display;

			size_display << "(" << round(file_size_f * 10) / 10 << size_str[size_str_i] << ")";
			header_offset_display << "[0x" << std::hex << h_file_offset << "]";
			file_offset_display << "[0x" << std::hex << file_pointer << "]";
#ifdef IAMT_AUDIO_ONLY
			if (str.find(".tga") == std::string::npos &&
				str.find(".preload") == std::string::npos &&
				str.find(".jpg") == std::string::npos &&
				str.find(".hmap") == std::string::npos &&
				str.find(".jpeg") == std::string::npos &&
				str.find(".txt") == std::string::npos)
#endif
				std::cout << std::setw(5) << std::left << std::dec << ++files_counter
				<< std::setw(9) << std::left << header_offset_display.str()
				<< std::setw(12) << std::left << file_offset_display.str()
				<< std::setw(13) << std::left << size_display.str() << str << '\n';
			size_display.str("");
			header_offset_display.str("");
			file_offset_display.str("");
			size_display.clear();
			header_offset_display.clear();
			file_offset_display.clear();
		}
	}
	ifile.close();
	ifile.clear();
	/*cout << "CMD command:\n" << command << '\n';*/
}


void modeWadRepack(bool wait_for_files) {
	std::cout << "Repack mode.\n";//argv[2] - hq2x, argv[3, 5...] - path, argv[4, 6...] - file
	//command += " -r ";



	std::string hq2x;
	std::vector<std::string> files;
	//Read files and paths and put them into the vector
	if (wait_for_files == false) {
		for (int i = 3; i < g_argc; i++) {
			files.push_back(std::string(g_argv[i]));//0, 2 - path; 1, 3 - file, then repeats...?
		}
		hq2x = g_argv[2];
	}
	else//If there are no files given
	{
		if (g_argc < 3) {
			std::cout << "Enter the path to \"dat.hq2x\" file:\n";
			hq2x = OpenFileWindow(
				"InterAction WAD Format (.dat; .hq2x; .222x.*)\0"
				"*.DAT;*.HQ2X;*.222X.*\0"
				"All Files\0*.*\0");
			/*
			Old code (in case this won't work for someone and I have an option to revert changes)
			getline(cin, hq2x);
			while (hq2x.find('\"') != string::npos)
			{
				hq2x.erase(hq2x.find('\"'), 1);
			}
			*/
		}
		else {
			hq2x = g_argv[2];
		}
		std::cout << "\nEnter the file paths you want to work with. Put an empty line when you finish.\n(Click RMB to paste path from buffer)\n\n"
			"\"undo\" - re-enter last pair.\n\"edit X\" - re-enter Xth pair.\n\"txt\" - use text file instead (will clear the ones you wrote before)\n\"folder\" - use folder as the root of paths\n";
		int counter = 1;
		std::string inpt;
		for (;;) {
			std::cout << counter << ". Path in hq2x: ";
			getline(std::cin, inpt);
			while (inpt.find('\"') != std::string::npos) {
				inpt.erase(inpt.find('\"'), 1);//delete " symbols
			}
			if (inpt == "") {
				std::cout << "\n\nTyping finished.\n";
				break;
			}
			else if (inpt == "undo") {
				if (files.size() >= 2) {
					files.pop_back();
					files.pop_back();
					counter--;
					continue;
				}
				else {
					std::cout << "You've reached the beginning!\n";
					continue;
				}
			}
			else if (inpt.find("edit") == 0 && inpt.size() > 4 && inpt[4] == ' ')//edit NUM. In case of "edit 9"... This shit better work on the first try (UPD: Ok it doesn't work) (UPD2: Fixed a bug when 4. edit 4 caused crash)
			{
				uint32_t edit_goto = 0;
				try {
					edit_goto = std::stoi(inpt.substr(4));//...this thing will be = 9
				}
				catch (const std::invalid_argument&) {
					std::cout << "Can't find proper number in the line!\n";
					continue;
				}
				//go edit the given pair of files
				if (edit_goto >= 1 && edit_goto < counter) {
					std::cout << edit_goto << ". Path in hq2x: ";
					getline(std::cin, inpt);
					while (inpt.find('\"') != std::string::npos) {
						inpt.erase(inpt.find('\"'), 1);//delete " symbols
					}
					edit_goto = (edit_goto - 1) * 2;//1 = [0, 1], 2 = [2, 3]
					files[edit_goto] = inpt;
					std::cout << " | Replace with: ";
					inpt = OpenFileWindow("All Files\0*.*\0Audio Files (*.ogg; *.wav; *.mp3)\0*.OGG;*.WAV;*.MP3\0"
#ifndef IAMT_AUDIO_ONLY
						"Graphics Files (*.tga; *.png; *.jpg; *.jpeg; *.bmp)\0*.TGA;*.PNG;*.JPG;*.JPEG;*.BMP\0"
#endif
					);
					/*
					getline(cin, inpt);
					while (inpt.find('\"') != string::npos)
					{
						inpt.erase(inpt.find('\"'), 1);
					}
					*/
					files[edit_goto + 1] = inpt;
					edit_goto = 0;
					continue;
				}
				else {
					std::cout << "Input value is incorrect.\n";
					continue;
				}
			}
			else if (inpt == "txt" || inpt == "TXT") {
				inpt = OpenFileWindow("Text File (*.txt)\0*.TXT\0All Files\0*.*\0");
				std::ifstream txtf(inpt);
				if (!txtf) {
					std::cout << "[Error] Can't open the file!\n";
					txtf.clear();
					continue;
				}
				files.clear();
				files = txt2vec(txtf);
				std::cout << "Done.\n";
				continue;
			}
			else if (inpt == "folder" || inpt == "FOLDER") {
				inpt = BrowseFolder("C:\\");
				if (!inpt.empty()) {
					files.clear();
					files = get_paths(inpt);
					std::cout << "Done.\n";
				}
				continue;
			}
			files.push_back(inpt);
			std::cout << " | Replace with: ";
			inpt = OpenFileWindow("All Files\0*.*\0Audio Files (*.ogg; *.wav; *.mp3)\0*.OGG;*.WAV;*.MP3\0"
#ifndef IAMT_AUDIO_ONLY
				"Graphics Files (*.tga; *.png; *.jpg; *.jpeg; *.bmp)\0*.TGA;*.PNG;*.JPG;*.JPEG;*.BMP\0"
#endif
			);
			/*
			getline(cin, inpt);
			while (inpt.find('\"') != string::npos)
			{
				inpt.erase(inpt.find('\"'), 1);
			}
			*/
			files.push_back(inpt);
			counter++;
		}
	}
	if (int(files.size()) % 2 != 0)//if SOMEHOW you entered one more file
		files.pop_back();

	for (uint32_t i = 0; i < files.size(); i += 2)//Replace all \ with / in hq2x paths
	{
		while (files[i].find('\\') != std::string::npos) {
			files[i].at(files[i].find('\\')) = '/';
		}
	}
	//command += '\"' + hq2x + "\" ";
	//Display final result
	std::cout << "Changes to be made:\nDAT file: \"" << hq2x << "\"\n";
	for (uint32_t i = 0; i < files.size(); i += 2) {
		//command += '\"' + files[i] + "\" \"" + files[i + 1] + "\" ";
		std::cout << "@ " << files[i] << " -> " << files[i + 1] << '\n';
	}
	//cout << "\nCMD command:\n" << command << "\n\n";
	for (;;) {
		std::string inpt;
		std::cout << "Do you want to start the replacement? (y/n): ";
		getline(std::cin, inpt);
		if (inpt == "y")
			break;
		if (inpt == "n") {
			std::cout << "\nReplacement canceled.\n";
			BetterCallPause();
			exit(0);
		}
		else {
			std::cout << "\r";
		}
	}

	//Open hq2x in reading mode
	std::cout << "\nOpening the \"" << hq2x << "\".";
	std::fstream ifile(hq2x, std::ios::in | std::ios::binary);
	if (!ifile) {
		std::cerr << "\n[Error] Can't open \"" << hq2x << "\".\n"
			"If you're trying to open the file from Program Files folder, run the tool from administrator.\n";
		//"If the path contains non-latin symbols, try using CMD command instead:\n"
		//<< command << '\n';
		BetterCallPause();
		exit(1);
	}
	std::cout << " Done!\n";


	//Which hq2x type
	char hq2x_type = '\0';
	uint32_t files_amount;
	ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
	if (files_amount == 0x20455655)//CIU v35+ WAD4 "UVE " line
	{
		hq2x_type = '4';
	}
	else if (files_amount >= 0 && files_amount <= 10)//old CI4-all HD versions + old CIU
	{
		hq2x_type = '3';
	}
	else {
		ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		if (files_amount == 0x2F786667 || files_amount == 0x2F786673 || files_amount == 0x6973756D || files_amount == 0x74737973)// "gfx/" "sfx/" "musi" "syst"
		{
			hq2x_type = '1';
		}
		else {
			hq2x_type = '2';
		}
	}

	//Read files amount
	if (hq2x_type == '3') {
		ifile.seekg(0x4);
		ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
	}
	else if (hq2x_type == '4') {
		ifile.seekg(0xC);
		ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
	}
	else {
		ifile.seekg(0);
		ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
	}

	if (g_iamt_config.full_backup) {
		std::cout << "Creating a backup.\n";
		ifile.seekg(0, std::ios::end);
		uint32_t erase_point = ifile.tellg();
		char* buffer = new char[erase_point];
		ifile.seekg(0);
		ifile.read(buffer, erase_point);
		ifile.close();

		std::ofstream ofile(hq2x + ".backup", std::ios::out | std::ios::binary);
		if (!ofile) {
			std::cerr << "[Error] Can't create backup file.\n"
				"Try running the tool from administrator.\n";
			//"If the path contains non-latin symbols, try using CMD command instead:\n"
			//<< command << '\n';
			BetterCallPause();
			exit(1);
		}
		ofile.write(buffer, erase_point);
		ofile.close();
		delete[] buffer;
		std::cout << "Backup successfully created.\n\n";
	}
	else {
		std::cout << "Full backup is turned off by config.\n\n";
		ifile.close();
	}
	//Get header size
	//header_size = 0x5c * files_amount + 0x8;

	//queue = number of pair of files (file+0, path+1)
	if (hq2x_type == '3') {
		for (uint32_t queue = 0; queue < files.size(); queue += 2) {
			std::cout << "Now replacing: " << files[queue] << "\nOpening \"" << hq2x << "\".";
			ifile.open(hq2x, std::ios::in | std::ios::binary);
			if (!ifile) {
				std::cerr << "\n[Error] Can't open \"" << hq2x << "\".\n"
					"If you're trying to open the file from Program Files folder, run the tool from administrator.\n";
				/*"If the path contains non-latin symbols, try using CMD command instead:\n"
				<< command << '\n';*/
				BetterCallPause();
				exit(1);
			}
			std::cout << " Done!\nSearching \"" << files[queue] << "\" in the header.";
			bool found = false;
			std::string str(files[queue]);//Path

#ifdef IAMT_AUDIO_ONLY
			if (str.find(".tga") != std::string::npos) {
				ifile.close();
				continue;
			}
#endif
			//Encode file name. res = encoded name
			std::string res = str_xor(str);
			//Search in the header
			//str = "";
			uint32_t h_file_offset;
			uint32_t f_num = 0;//number of the file which is needed in the header

			while (f_num < files_amount) {
				uint32_t h_file_offset = 0x8 + 0x5c * f_num;//file in the header offset
				ifile.seekg(h_file_offset);
				//Read the name from file
				str.clear();
				unsigned char c = 0xFF;
				uint32_t temp_off = 0;
				while (uint8_t(c) != 0xCC) {
					ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
					temp_off++;
					c = ifile.get();
					if (uint8_t(c) != 0xcc) {
						str += c;
					}
				}
				if (res == str) {
					found = true;
					break;
				}
				f_num++;
			}
			uint32_t file_pointer;
			uint32_t file_size;
			uint32_t real_file_size;
			if (found == true) {
				ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x50));
				ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));//poiner value
				ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));//size value
				ifile.read(reinterpret_cast<char*>(&real_file_size), sizeof(real_file_size));
			}
			else {
				std::cerr << "\n[Error] Can't find given file path \"" << files[queue] << "\" in the header.\nTake a look in USAGE.txt\n";
				//"You can also try using CMD command instead:\n" << command << "\n\n";
				for (;;) {
					std::string inpt;
					std::cout << "Do you want to skip this file? (y/n): ";
					getline(std::cin, inpt);
					if (inpt == "y") {
						break;
					}
					if (inpt == "n") {
						std::cout << "\nReplacement canceled.\n";
						BetterCallPause();
						exit(0);
					}
					else {
						std::cout << "\r";
					}
				}
				ifile.clear();
				ifile.close();
				std::cout << "\n\n";
				continue;
			}
			std::cout << " Done!\n";
			//char* part_b;//the rest of the file
			//h_file_offset is still a beginning of the block related to our file
			ifile.seekg(0, std::ios::end);
			uint64_t full_size = ifile.tellg();//full hq2x size
			ifile.seekg(uint64_t(file_pointer));
			char* old_file = new char[real_file_size];//to test if they are the same
			ifile.read(old_file, uint64_t(real_file_size));//Old file to check if they are the same
			ifile.seekg(uint64_t(file_pointer) + uint64_t(real_file_size));//go to the end of the replaceable file
			char* part_b = new char[full_size - uint64_t(file_pointer) - uint64_t(real_file_size)];
			ifile.read(part_b, full_size - uint64_t(file_pointer) - uint64_t(real_file_size));//full_size - uint64_t(file_pointer) - uint64_t(file_size)+1
			ifile.clear();
			ifile.close();
			std::cout << "Opening \"" << files[queue + 1] << "\".";
			ifile.open(files[queue + 1], std::ios::in | std::ios::binary);//open file to be replaced with
			if (!ifile) {
				std::cerr << "\n[Error] Can't open \"" << files[queue + 1] << "\".\n"
					"Try running the tool from administrator.\n";
				/*"If the path contains non-latin symbols, try using CMD command instead:\n"
				<< command << "\n\n";*/
				for (;;) {
					std::string inpt;
					std::cout << "Do you want to skip this file? (y/n): ";
					getline(std::cin, inpt);
					if (inpt == "y") { break; }
					if (inpt == "n") {
						std::cout << "\nReplacement canceled.\n";
						BetterCallPause();
						exit(0);
					}
					else {
						std::cout << "\r";
					}
				}
				ifile.clear();
				delete[] old_file;
				delete[] part_b;
				std::cout << "\n\n";
				continue;
			}
			std::cout << " Done!\n";
#ifdef IAMT_AUDIO_ONLY
			//AUDIO CHECK
			uint32_t test_val = 0;
			ifile.read(reinterpret_cast<char*>(&test_val), sizeof(uint32_t));
			if (FOURCC_CHECK(test_val)) {/*OggS*//*RIFF*/
				std::cout << "Can't use. This file is not an audio file.\n";
				ifile.close();
				delete[] old_file;
				delete[] part_b;
				cout << "\n\n";
				continue;
			}
			ifile.seekg(0);
			//AUDIO CHECK
#endif
			ifile.seekg(0, std::ios::end);
			uint64_t new_file_size = ifile.tellg();
			char* part_a;//new file
			part_a = new char[new_file_size];
			ifile.seekg(0, std::ios::beg);
			ifile.read(part_a, new_file_size);
			ifile.clear();
			ifile.close();

			//Check if files are the same
			if (real_file_size == new_file_size) {
				bool same = false;
				for (int i = 0; i < real_file_size; i++) {
					if (part_a[i] != old_file[i]) {
						break;
					}
					if ((i == real_file_size - 1) && (part_a[i] == old_file[i])) {
						std::cout << "Files are the same, skipping...\n";
						same = true;
					}
				}
				if (same == true) {
					delete[] part_a;
					delete[] old_file;
					delete[] part_b;
					continue;
				}
			}
			delete[] old_file;

			std::cout << "Opening \"" << hq2x << "\".";
			ifile.open(hq2x, std::ios::in | std::ios::out | std::ios::binary);//Open hq2x in read&write mode
			if (!ifile) {
				std::cerr << "\n[Error] Can't open \"" << hq2x << "\".\n"
					"If you're trying to open the file from Program Files folder, run the tool from administrator.\n";
				/*"If the path contains non-latin symbols, try using CMD command instead:\n"
				<< command << '\n';*/
				BetterCallPause();
				exit(1);
			}
			std::cout << " Done!\nRewriting the header segment.";
			ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x54)); // write new file size
			ifile.write((char*)&new_file_size, sizeof(new_file_size));
			ifile.write((char*)&new_file_size, sizeof(new_file_size));
			std::cout << " Done!\nRewriting the rest of the file.";
			ifile.seekg(file_pointer);
			ifile.write(part_a, new_file_size);
			ifile.write(part_b, full_size - uint64_t(file_pointer) - uint64_t(real_file_size));

			uint64_t erase_point = ifile.tellg();
			uint64_t difference = new_file_size - real_file_size;
			std::cout << " Done!\nFile difference is " << std::dec << difference << " bytes. Rewriting the header.";
			for (uint32_t i = f_num + 1; i < files_amount; i++) {
				ifile.seekg(uint64_t(0x5c) * i + uint64_t(0x58));
				ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));
				file_pointer += difference;
				ifile.seekg(-4, std::ios::cur);
				ifile.write((char*)&file_pointer, 4);
			}
			std::cout << " Done!\nErasing residual bytes if they exist.\n";
			delete[] part_a;
			delete[] part_b;
			char* buffer = new char[unsigned int(erase_point)];
			ifile.seekg(0);
			ifile.read(buffer, erase_point);
			ifile.close();
			std::ofstream ofile(hq2x, std::ios::binary);
			ofile.write(buffer, erase_point);
			ofile.close();

			if (g_iamt_config.last_backup) {
				std::cout << "Creating last backup.\n";
				ofile.open(hq2x + ".last", std::ios::binary);
				if (!ofile) {
					std::cerr << "[Error] Can't create backup of the last successful replacement.\n"
						"If you're trying to create the file in Program Files folder, run the tool from administrator.\n";
					/*"If the path contains non-latin symbols, try using CMD command instead:\n"
					<< command << '\n';*/
					BetterCallPause();
					exit(2);
				}
				ofile.write(buffer, erase_point);
				ofile.close();
				std::cout << "Done! Saved as " << hq2x << ".last\n";
			}
			else {
				std::cout << "Last backup is turned off by config.\n";
			}
			delete[] buffer;
			std::cout << "File replacement is finished.\n\n";
		}
	}
	if (hq2x_type == '2') {
		for (uint32_t queue = 0; queue < files.size(); queue += 2) {
			std::cout << "Now replacing: " << files[queue] << "\nOpening \"" << hq2x << "\".";
			ifile.open(hq2x, std::ios::in | std::ios::binary);
			if (!ifile) {
				std::cerr << "\n[Error] Can't open \"" << hq2x << "\".\n"
					"If you're trying to open the file from Program Files folder, run the tool from administrator.\n";
				/*"If the path contains non-latin symbols, try using CMD command instead:\n"
				<< command << '\n';*/
				BetterCallPause();
				exit(1);
			}

			std::cout << " Done!\nSearching \"" << files[queue] << "\" in the header.";
			std::string str{ files[queue] };//Path

#ifdef IAMT_AUDIO_ONLY
			if (str.find(".tga") != std::string::npos) {
				ifile.close();
				continue;
			}
#endif
			//Encode file name. res = encoded given name
			std::string res = str_xor(str);
			//Search in the header
			//str = "";
			uint32_t f_num = 0;//number of the needed file in the header
			bool found = false;
			uint32_t h_file_offset;
			uint32_t file_pointer;
			uint32_t file_size;
			while (f_num < files_amount) {
				h_file_offset = 0x4 + 0x58 * f_num;//offset of the file in the header
				ifile.seekg(h_file_offset);

				//Read the name from file
				str.clear();
				unsigned char c = 0xFF;
				uint64_t temp_off = 0;
				while (uint8_t(c) != 0xCC) {
					ifile.seekg(uint64_t(h_file_offset) + temp_off);
					temp_off++;
					c = ifile.get();
					if (uint8_t(c) != 0xcc) {
						str += c;
					}
				}
				if (res == str) {
					found = true;
					break;
				}
				f_num++;
			}
			if (found == true) {
				ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x50));
				ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));//poiner value
				ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));//size value
			}
			else {
				std::cerr << "\n[Error] Can't find given file path \"" << files[queue] << "\" in the header.\nTake a look in USAGE.txt\n";
				//"You can also try using CMD command instead:\n" << command << "\n\n";
				for (;;) {
					std::string inpt;
					std::cout << "Do you want to skip this file? (y/n): ";
					getline(std::cin, inpt);
					if (inpt == "y") { break; }
					if (inpt == "n") {
						std::cout << "\nReplacement canceled.\n";
						BetterCallPause();
						exit(0);
					}
					else {
						std::cout << "\r";
					}
				}
				ifile.clear();
				ifile.close();
				std::cout << "\n\n";
				continue;
			}
			std::cout << " Done!\n";

			//char* part_b;//the rest of the file
			//h_file_offset is still a beginning of the block related to our file
			ifile.seekg(0, std::ios::end);
			uint64_t full_size = ifile.tellg();//full hq2x size
			ifile.seekg(uint64_t(file_pointer));
			char* old_file = new char[file_size];
			ifile.read(old_file, uint64_t(file_size));
			ifile.seekg(uint64_t(file_pointer) + uint64_t(file_size));//go to the end of the replaceable file
			char* part_b = new char[full_size - uint64_t(file_pointer) - uint64_t(file_size)];
			ifile.read(part_b, full_size - uint64_t(file_pointer) - uint64_t(file_size));//full_size - uint64_t(file_pointer) - uint64_t(file_size)+1
			ifile.clear();
			ifile.close();
			std::cout << "Opening \"" << files[queue + 1] << "\".";
			ifile.open(files[queue + 1], std::ios::in | std::ios::binary);//open file to be replaced with
			if (!ifile) {
				std::cerr << "\n[Error] Can't open \"" << files[queue + 1] << "\".\n"
					"Try running the tool from administrator.\n";
				//"If the path contains non-latin symbols, try using CMD command instead:\n" << command << "\n\n";
				for (;;) {
					std::string inpt;
					std::cout << "Do you want to skip this file? (y/n): ";
					getline(std::cin, inpt);
					if (inpt == "y") { break; }
					if (inpt == "n") {
						std::cout << "\nReplacement canceled.\n";
						BetterCallPause();
						exit(0);
					}
					else {
						std::cout << "\r";
					}
				}
				ifile.clear();
				delete[] old_file;
				delete[] part_b;
				std::cout << "\n\n";
				continue;
			}
			std::cout << " Done!\n";
#ifdef IAMT_AUDIO_ONLY
			//AUDIO CHECK
			uint32_t test_val = 0;
			ifile.read(reinterpret_cast<char*>(&test_val), sizeof(uint32_t));
			if (FOURCC_CHECK(test_val)) {/*OggS*//*RIFF*/
				std::cout << "Can't use. This file is not an audio file.\n";
				ifile.close();
				delete[] old_file;
				delete[] part_b;
				cout << "\n\n";
				continue;
			}
			ifile.seekg(0);
			//AUDIO CHECK
#endif
			ifile.seekg(0, std::ios::end);
			uint64_t new_file_size = ifile.tellg();
			char* part_a;//new file
			part_a = new char[new_file_size];
			ifile.seekg(0, std::ios::beg);
			ifile.read(part_a, new_file_size);
			ifile.clear();
			ifile.close();


			//Check if files are the same
			if (file_size == new_file_size) {
				bool same = false;
				for (int i = 0; i < file_size; i++) {
					if (part_a[i] != old_file[i]) {
						break;
					}
					if ((i == file_size - 1) && (part_a[i] == old_file[i])) {
						std::cout << "Files are the same, skipping...\n";
						same = true;
					}
				}
				if (same == true) {
					delete[] part_a;
					delete[] old_file;
					delete[] part_b;
					continue;
				}
			}
			delete[] old_file;

			std::cout << "Opening \"" << hq2x << "\".";
			ifile.open(hq2x, std::ios::in | std::ios::out | std::ios::binary);//Open hq2x in read&write mode
			if (!ifile) {
				std::cerr << "\n[Error] Can't open \"" << hq2x << "\".\n"
					"If you're trying to open the file from Program Files folder, run the tool from administrator.\n";
				/*"If the path contains non-latin symbols, try using CMD command instead:\n"
				<< command << '\n';*/
				BetterCallPause();
				exit(1);
			}
			std::cout << " Done!\nRewriting the header segment.";
			ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x54));//write new file size
			ifile.write((char*)&new_file_size, sizeof(new_file_size));
			std::cout << " Done!\nRewriting the rest of the file.";
			ifile.seekg(file_pointer);
			ifile.write(part_a, new_file_size);
			ifile.write(part_b, full_size - uint64_t(file_pointer) - uint64_t(file_size));
			uint64_t erase_point = ifile.tellg();
			uint64_t difference = new_file_size - file_size;
			cout << " Done!\nFile difference is " << std::dec << difference << " bytes. Rewriting the header.";
			for (uint32_t i = f_num + 1; i < files_amount; i++)
			{
				ifile.seekg(uint64_t(0x58) * i + uint64_t(0x54));
				ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));
				file_pointer += difference;
				ifile.seekg(-4, ios::cur);
				ifile.write((char*)&file_pointer, 4);
			}
			cout << " Done!\nErasing residual bytes if they exist.\n";
			delete[] part_a;
			delete[] part_b;
			char* buffer = new char[unsigned int(erase_point)];
			ifile.seekg(0);
			ifile.read(buffer, erase_point);
			ifile.close();
			ofile.open(hq2x, ios::out | ios::binary);
			ofile.write(buffer, erase_point);
			ofile.close();

			if (last_backup == true)
			{
				cout << "Creating last backup.\n";
				ofile.open(hq2x + ".last", ios::out | ios::binary);
				if (!ofile)
				{
					cerr << "[Error] Can't create backup of the last successful replacement.\n"
						"If you're trying to create the file in Program Files folder, run the tool from administrator.\n"
						"If the path contains non-latin symbols, try using CMD command instead:\n"
						<< command << '\n';
					cout << "Press Enter to continue...";
					cin.get();
					//cin.ignore();
					exit(2);
				}
				ofile.write(buffer, erase_point);
				ofile.close();
				cout << "Done! Saved as " << hq2x << ".last\n";
			}
			else
			{
				cout << "Last backup is turned off by config.\n";
			}

			delete[] buffer;
			cout << "File replacement is finished.\n\n";
			}
		}
	if (hq2x_type == '1')
	{
		for (uint32_t queue = 0; queue < files.size(); queue += 2)
		{
			cout << "Now replacing: " << files[queue] << "\nOpening \"" << hq2x << "\".";
			ifile.open(hq2x, ios::in | ios::binary);
			if (!ifile)
			{
				cerr << "\n[Error] Can't open \"" << hq2x << "\".\n"
					"If you're trying to open the file from Program Files folder, run the tool from administrator.\n"
					"If the path contains non-latin symbols, try using CMD command instead:\n"
					<< command << '\n';
				cout << "Press Enter to continue...";
				cin.get();
				//cin.ignore();
				exit(1);
			}

			cout << " Done!\nSearching \"" << files[queue] << "\" in the header.";
			found = false;
			str = files[queue];//Path
#ifdef IAMT_AUDIO_ONLY
			if (str.find(".tga") != std::string::npos) {
				ifile.close();
				continue;
			}
#endif
			//Encode file name. res = encoded given name
			res.clear();
			res = str;
			//Search in the header
			//str = "";
			uint32_t f_num = 0;//number of the needed file in the header
			while (f_num < files_amount)
			{
				h_file_offset = 0x4 + 0x58 * f_num;//offset of the file in the header
				ifile.seekg(h_file_offset);

				//Read the name from file
				str.clear();
				c = 0xFF;
				temp_off = 0;
				while (uint8_t(c) != 0x00)
				{
					ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
					temp_off++;
					c = ifile.get();
					if (uint8_t(c) != 0x00)
						str += c;
				}
				if (res == str)
				{
					found = true;
					break;
				}
				f_num++;
		}
			if (found == true)
			{
				ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x50));
				ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));//poiner value
				ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));//size value
			}
			else {
				cerr << "\n[Error] Can't find given file path \"" << files[queue] << "\" in the header.\nTake a look in USAGE.txt\n"
					"You can also try using CMD command instead:\n" << command << "\n\n";
				for (;;)
				{
					string inpt;
					cout << "Do you want to skip this file? (y/n): ";
					getline(cin, inpt);
					if (inpt == "y")
						break;
					if (inpt == "n")
					{
						cout << "\nReplacement canceled.\n";
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(0);
					}
					else
					{
						cout << "\r";
					}
				}
				ifile.clear();
				ifile.close();
				cout << "\n\n";
				continue;
			}
			cout << " Done!\n";
			char* part_a;//new file
			char* old_file = new char[file_size];//to test if they are the same
			//char* part_b;//the rest of the file
			//h_file_offset is still a beginning of the block related to our file
			ifile.seekg(0, ios::end);
			full_size = ifile.tellg();//full hq2x size
			ifile.seekg(uint64_t(file_pointer));
			ifile.read(old_file, uint64_t(file_size));//Old file to check if they are the same
			ifile.seekg(uint64_t(file_pointer) + uint64_t(file_size));//go to the end of the replaceable file
			char* part_b = new char[full_size - uint64_t(file_pointer) - uint64_t(file_size)];
			ifile.read(part_b, full_size - uint64_t(file_pointer) - uint64_t(file_size));//full_size - uint64_t(file_pointer) - uint64_t(file_size)+1
			ifile.clear();
			ifile.close();
			cout << "Opening \"" << files[queue + 1] << "\".";
			ifile.open(files[queue + 1], ios::in | ios::binary);//open file to be replaced with
			if (!ifile)
			{
				cerr << "\n[Error] Can't open \"" << files[queue + 1] << "\".\n"
					"Try running the tool from administrator.\n"
					"If the path contains non-latin symbols, try using CMD command instead:\n"
					<< command << "\n\n";
				for (;;)
				{
					string inpt;
					cout << "Do you want to skip this file? (y/n): ";
					getline(cin, inpt);
					if (inpt == "y")
						break;
					if (inpt == "n")
					{
						cout << "\nReplacement canceled.\n";
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(0);
					}
					else
					{
						cout << "\r";
					}
				}
				ifile.clear();
				delete[] old_file;
				delete[] part_b;
				cout << "\n\n";
				continue;
			}
			cout << " Done!\n";
#ifdef IAMT_AUDIO_ONLY
			//AUDIO CHECK
			uint32_t test_val = 0;
			ifile.read(reinterpret_cast<char*>(&test_val), sizeof(uint32_t));
			if (FOURCC_CHECK(test_val)) {/*OggS*//*RIFF*/
				std::cout << "Can't use. This file is not an audio file.\n";
				ifile.close();
				delete[] old_file;
				delete[] part_b;
				cout << "\n\n";
				continue;
			}
			ifile.seekg(0);
			//AUDIO CHECK
#endif
			ifile.seekg(0, ios::end);
			new_file_size = ifile.tellg();
			part_a = new char[new_file_size];
			ifile.seekg(0, ios::beg);
			ifile.read(part_a, new_file_size);
			ifile.clear();
			ifile.close();

			//Check if files are the same
			if (file_size == new_file_size)
			{
				bool same = false;
				for (int i = 0; i < file_size; i++)
				{
					if (part_a[i] != old_file[i])
					{
						break;
					}
					if ((i == file_size - 1) && (part_a[i] == old_file[i]))
					{
						cout << "Files are the same, skipping...\n";
						same = true;
					}
				}
				if (same == true)
				{
					delete[] part_a;
					delete[] old_file;
					delete[] part_b;
					continue;
				}
			}
			delete[] old_file;

			cout << "Opening \"" << hq2x << "\".";
			ifile.open(hq2x, ios::in | ios::out | ios::binary);//Open hq2x in read&write mode
			if (!ifile)
			{
				cerr << "\n[Error] Can't open \"" << hq2x << "\".\n"
					"If you're trying to open the file from Program Files folder, run the tool from administrator.\n"
					"If the path contains non-latin symbols, try using CMD command instead:\n"
					<< command << '\n';
				cout << "Press Enter to continue...";
				cin.get();
				//cin.ignore();
				exit(1);
			}
			cout << " Done!\nRewriting the header segment.";
			ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x54));//write new file size
			ifile.write((char*)&new_file_size, sizeof(new_file_size));
			cout << " Done!\nRewriting the rest of the file.";
			ifile.seekg(file_pointer);
			ifile.write(part_a, new_file_size);
			ifile.write(part_b, full_size - uint64_t(file_pointer) - uint64_t(file_size));
			erase_point = ifile.tellg();
			difference = new_file_size - file_size;
			cout << " Done!\nFile difference is " << dec << difference << " bytes. Rewriting the header.";
			for (uint32_t i = f_num + 1; i < files_amount; i++)
			{
				ifile.seekg(uint64_t(0x58) * i + uint64_t(0x54));
				ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));
				file_pointer += difference;
				ifile.seekg(-4, ios::cur);
				ifile.write((char*)&file_pointer, 4);
			}
			cout << " Done!\nErasing residual bytes if they exist.\n";
			delete[] part_a;
			delete[] part_b;
			char* buffer = new char[unsigned int(erase_point)];
			ifile.seekg(0);
			ifile.read(buffer, erase_point);
			ifile.close();
			ofile.open(hq2x, ios::out | ios::binary);
			ofile.write(buffer, erase_point);
			ofile.close();

			if (last_backup == true)
			{
				cout << "Creating last backup.\n";
				ofile.open(hq2x + ".last", ios::out | ios::binary);
				if (!ofile)
				{
					cerr << "[Error] Can't create backup of the last successful replacement.\n"
						"If you're trying to create the file in Program Files folder, run the tool from administrator.\n"
						"If the path contains non-latin symbols, try using CMD command instead:\n"
						<< command << '\n';
					cout << "Press Enter to continue...";
					cin.get();
					//cin.ignore();
					exit(2);
				}
				ofile.write(buffer, erase_point);
				ofile.close();
				cout << "Done! Saved as " << hq2x << ".last\n";
			}
			else
			{
				cout << "Last backup is turned off by config.\n";
			}

			delete[] buffer;
			cout << "File replacement is finished.\n\n";
			}
	}
	if (hq2x_type == '4')
	{
		cout << "Chicken Invaders Universe requires additional table to find names right way.\nOpening \"CIU.iamt_table\".\n";
		ifstream ciu_table(tool_folder + "CIU.iamt_table", ios::binary);
		uint64_t format_check;
		uint32_t ver;//crc or version of table
		uint32_t crc;//crc from hq2x
		uint32_t table_files_amount;
		bool name_exists;
		if (!ciu_table)
		{
			cerr << "[Error] Can't open CIU.iamt_table. Check the tool folder.\n";
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
			exit(1);
		}
		ciu_table.read(reinterpret_cast<char*>(&format_check), sizeof(format_check));
		if (format_check != 0x55494320544D4149)//IAMT CIU
		{
			cerr << "[Error] CIU table is corrupted! Please, get a new one or check if you use the right one.\n";
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
			exit(2);
		}
		ciu_table.read(reinterpret_cast<char*>(&ver), sizeof(ver));//read CIU version
		cout << "CIU table version: " << dec << ver << '\n';
		ciu_table.read(reinterpret_cast<char*>(&table_files_amount), sizeof(table_files_amount));
		if (files_amount != table_files_amount)
			cout << "Warning: CIU version can be different to CIU table version. Some files won't use real names (check this by -s header mode).\n\n";

		for (uint32_t queue = 0; queue < files.size(); queue += 2)
		{
			cout << "Now replacing: " << files[queue] << "\nOpening \"" << hq2x << "\".";
			ifile.open(hq2x, ios::in | ios::binary);
			if (!ifile)
			{
				cerr << "\n[Error] Can't open \"" << hq2x << "\".\n"
					"If you're trying to open the file from Program Files folder, run the tool from administrator.\n"
					"If the path contains non-latin symbols, try using CMD command instead:\n"
					<< command << '\n';
				cout << "Press Enter to continue...";
				cin.get();
				//cin.ignore();
				exit(1);
			}
			cout << " Done!\nSearching \"" << files[queue] << "\" in the header.\n";
			found = false;
			name_exists = false;
			str = files[queue];//Path
#ifdef IAMT_AUDIO_ONLY
			if (str.find(".tga") != std::string::npos) {
				ifile.close();
				continue;
			}
#endif
			res.clear();
			res = str;//res = path
			//Search in the header
			//str = "";
			uint32_t f_num = 0;//number of the file which is needed in the header

			//find name in the table
			for (uint64_t i = 0; i < table_files_amount; i++)
			{
				str.clear();
				c = 0xFF;
				temp_off = 0;
				while (uint8_t(c) != 0x00)
				{
					ciu_table.seekg(0x10 + 0x60 * i + uint64_t(temp_off));
					temp_off++;
					c = ciu_table.get();
					if (uint8_t(c) != 0x00)
						str += c;
				}
				if (str == res)
				{
					ciu_table.seekg(0x10 + 0x60 * i);
					name_exists = true;
					break;
				}
			}
			if (name_exists == true)
			{
				cout << "Name has been recognized.\n";
				ciu_table.seekg(0x5c, ios::cur);
				ciu_table.read(reinterpret_cast<char*>(&ver), sizeof(ver));//crc of the needed file
			}
			else
			{
				//turn text into crc in case we can't find it as name
				cout << "Can't find name in the table. Reading as CRC: ";
				ciu_table.clear();//protection from going out of bounds in case files amount written in the table is wrong (higher) for some reason.
				ver = strtoul(res.c_str(), NULL, 16);
				cout << hex << ver << '\n';
			}

			//find crc(ver) in the hq2x
			while (f_num < files_amount)
			{
				h_file_offset = 0x10 + 0x10 * f_num;//file in the header offset
				ifile.seekg(h_file_offset);
				//Read the crc of file
				ifile.read(reinterpret_cast<char*>(&crc), sizeof(crc));
				if (crc == ver)
				{
					found = true;
					break;
				}
				f_num++;
			}

			if (found == true)
			{
				ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x4));
				ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));//poiner value
				ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));//size value
				ifile.read(reinterpret_cast<char*>(&real_file_size), sizeof(real_file_size));
			}
			else {
				cerr << "\n[Error] Can't find given file path/CRC \"" << files[queue] << "\" in the header.\nTake a look in USAGE.txt\n"
					"You can also try using CMD command instead:\n" << command << "\n\n";
				for (;;)
				{
					string inpt;
					cout << "Do you want to skip this file? (y/n): ";
					getline(cin, inpt);
					if (inpt == "y")
						break;
					if (inpt == "n")
					{
						cout << "\nReplacement canceled.\n";
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(0);
					}
					else
					{
						cout << "\r";
					}
				}
				ifile.clear();
				ifile.close();
				cout << "\n\n";
				continue;
			}

			cout << "Done!\n";
			char* part_a;//new file
			char* old_file = new char[real_file_size];//to test if they are the same
			//char* part_b;//the rest of the file
			//h_file_offset is still a beginning of the block related to our file
			ifile.seekg(0, ios::end);
			full_size = ifile.tellg();//full hq2x size
			ifile.seekg(uint64_t(file_pointer));
			ifile.read(old_file, uint64_t(real_file_size));//Old file to check if they are the same
			ifile.seekg(uint64_t(file_pointer) + uint64_t(real_file_size));//go to the end of the replaceable file
			char* part_b = new char[full_size - uint64_t(file_pointer) - uint64_t(real_file_size)];
			ifile.read(part_b, full_size - uint64_t(file_pointer) - uint64_t(real_file_size));//full_size - uint64_t(file_pointer) - uint64_t(file_size)
			ifile.clear();
			ifile.close();
			cout << "Opening \"" << files[queue + 1] << "\".";
			ifile.open(files[queue + 1], ios::in | ios::binary);//open file to be replaced with
			if (!ifile)
			{
				cerr << "\n[Error] Can't open \"" << files[queue + 1] << "\".\n"
					"Try running the tool from administrator.\n"
					"If the path contains non-latin symbols, try using CMD command instead:\n"
					<< command << "\n\n";
				for (;;)
				{
					string inpt;
					cout << "Do you want to skip this file? (y/n): ";
					getline(cin, inpt);
					if (inpt == "y")
						break;
					if (inpt == "n")
					{
						cout << "\nReplacement canceled.\n";
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(0);
					}
					else
					{
						cout << "\r";
					}
				}
				ifile.clear();
				delete[] old_file;
				delete[] part_b;
				cout << "\n\n";
				continue;
			}
			cout << " Done!\n";
#ifdef IAMT_AUDIO_ONLY
			//AUDIO CHECK
			uint32_t test_val = 0;
			ifile.read(reinterpret_cast<char*>(&test_val), sizeof(uint32_t));
			if (FOURCC_CHECK(test_val)) {/*OggS*//*RIFF*/
				std::cout << "Can't use. This file is not an audio file.\n";
				ifile.close();
				delete[] old_file;
				delete[] part_b;
				cout << "\n\n";
				continue;
			}
			ifile.seekg(0);
			//AUDIO CHECK
#endif
			ifile.seekg(0, ios::end);
			new_file_size = ifile.tellg();
			part_a = new char[new_file_size];
			ifile.seekg(0, ios::beg);
			ifile.read(part_a, new_file_size);
			ifile.clear();
			ifile.close();

			//Check if files are the same
			if (real_file_size == new_file_size)
			{
				bool same = false;
				for (int i = 0; i < real_file_size; i++)
				{
					if (part_a[i] != old_file[i])
					{
						break;
					}
					if ((i == real_file_size - 1) && (part_a[i] == old_file[i]))
					{
						cout << "Files are the same, skipping...\n";
						same = true;
					}
				}
				if (same == true)
				{
					delete[] part_a;
					delete[] old_file;
					delete[] part_b;
					continue;
				}
			}
			delete[] old_file;

			cout << "Opening \"" << hq2x << "\".";
			ifile.open(hq2x, ios::in | ios::out | ios::binary);//Open hq2x in read&write mode
			if (!ifile)
			{
				cerr << "\n[Error] Can't open \"" << hq2x << "\".\n"
					"If you're trying to open the file from Program Files folder, run the tool from administrator.\n"
					"If the path contains non-latin symbols, try using CMD command instead:\n"
					<< command << '\n';
				cout << "Press Enter to continue...";
				cin.get();
				//cin.ignore();
				exit(1);
			}
			cout << " Done!\nRewriting the header segment.";

			ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x8));//write new file size
			ifile.write((char*)&new_file_size, sizeof(new_file_size));
			ifile.write((char*)&new_file_size, sizeof(new_file_size));
			cout << " Done!\nRewriting the rest of the file.";
			ifile.seekg(file_pointer);
			ifile.write(part_a, new_file_size);//put new file
			ifile.write(part_b, full_size - uint64_t(file_pointer) - uint64_t(real_file_size));
			erase_point = ifile.tellg();
			difference = new_file_size - real_file_size;
			cout << " Done!\nFile difference is " << dec << difference << " bytes. Rewriting the header.";
			for (uint32_t i = f_num + 1; i < files_amount; i++)
			{
				ifile.seekg(uint64_t(0x10) * i + uint64_t(0x14));
				ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));
				file_pointer += difference;
				ifile.seekg(-4, ios::cur);
				ifile.write((char*)&file_pointer, 4);
			}
			cout << " Done!\nErasing residual bytes if they exist.\n";
			delete[] part_a;
			delete[] part_b;
			char* buffer = new char[erase_point];
			ifile.seekg(0);
			ifile.read(buffer, erase_point);
			ifile.close();
			ofile.open(hq2x, ios::out | ios::binary);
			ofile.write(buffer, erase_point);
			ofile.close();

			if (last_backup == true)
			{
				cout << "Creating last backup.\n";
				ofile.open(hq2x + ".last", ios::out | ios::binary);
				if (!ofile)
				{
					cerr << "[Error] Can't create backup of the last successful replacement.\n"
						"If you're trying to create the file in Program Files folder, run the tool from administrator.\n"
						"If the path contains non-latin symbols, try using CMD command instead:\n"
						<< command << '\n';
					cout << "Press Enter to continue...";
					cin.get();
					//cin.ignore();
					exit(2);
				}
				ofile.write(buffer, erase_point);
				ofile.close();
				cout << "Done! Saved as " << hq2x << ".last\n";
			}
			else
			{
				cout << "Last backup is turned off by config.\n";
			}

			delete[] buffer;
			cout << "File replacement is finished.\n\n\n";
			}
		ciu_table.close();
		}
	cout << "\nAll the files have been replaced :)\a\n";
	}


void modeWadExtract(bool wait_for_files) {}

// Done
void modeTableUpdate() {
	std::cout <<
		"Table update mode.\nIAMT MUST be launched with Administrator previleges!\n\n"
		"Set loadMode = 0 in CIU config. Also, back up your CIU.dat.hq2x as it might get corrupted.\n"
		"Write CIU version (if v41.1 - write 41, if 46.9 - write 46)\n"
		">";

	uint32_t version;
	std::cin >> version;

	// All strings here store information is UTF8 without BOM.
	std::filesystem::path hq2x_path; // C:\\Program Files (x86)\\ChickenInvadersUniverse\\CIU.dat.hq2x
	hq2x_path = OpenFileWindow("CIU WAD Format (.dat.hq2x; .222x)\0*.HQ2X;*.???X\0All Files\0*.*\0");
	std::filesystem::path table_path; // C:\\Users\\belik\\source\\repos\\CIU string decoder encoder\\Release\\share\\CIU.cimt_table
	table_path = OpenFileWindow("IAMT Table Format (.iamt_table)\0*.IAMT_TABLE\0All Files\0*.*\0");
	std::filesystem::path exe_path; // C:\\Program Files (x86)\\ChickenInvadersUniverse\\CIU.exe
	exe_path = OpenFileWindow("CIU Executable (.exe)\0*.EXE\0All Files\0*.*\0");

	std::cout <<
		"\nWAD file: " << hq2x_path <<
		"\nIAMT table: " << table_path <<
		"\nCIU.exe: " << exe_path <<
		"\n\nIf you select a wrong file, please restart IAMT. If you see gibberish, don't panic.\n";
	BetterCallPause();

	std::fstream hq2x(hq2x_path, std::ios::in | std::ios::out | std::ios::binary); // CIU.dat.hq2x
	if (!hq2x) {
		std::cerr << "[Error] Can't open the WAD file!\n";
		BetterCallPause();
		return;
	}

	hq2x.seekg(0xC);
	uint32_t famount; // files amount
	hq2x.read(reinterpret_cast<char*>(&famount), sizeof(famount));
	hq2x.close();

	if (!std::filesystem::exists(table_path)) {
		std::cout << "The table file doesn't exist. Would you like to create it? (y/n): ";
		while (1) {
			std::string inpt;
			std::cin.ignore(std::numeric_limits<std::streamsize>::max());
			std::getline(std::cin, inpt);
			if (inpt == "y") {
				std::ofstream touchfile(table_path, std::ios::binary);
				if (!touchfile) {
					std::cerr << "[Error] Can't create the table file!\n";
					BetterCallPause();
					return;
				}
				touchfile.close();
				break;
			}
			else if (inpt == "n") {
				break;
			}
		}
	}

	std::fstream table; // .iamt_table
	table.open(table_path, std::ios::in | std::ios::out | std::ios::binary);
	if (!table) {
		std::cerr << "[Error] Can't open the table!\n";
		BetterCallPause();
		return;
	}

	/* If the table file is empty we need to add a header */
	if (table.peek() == std::ifstream::traits_type::eof()) {
		std::printf("The table is empty. Writing the header.\n");
		// table.clear(); // clear() is probably not needed due to peeking
		table.write("IAMT CIU", 8);
		for (int i = 0; i < 8; ++i) { table.put(0); }
	}
	table.seekp(0x8);
	table.write(reinterpret_cast<char*>(&version), 4);
	table.close();

	uint32_t* hq2x_hashes = new uint32_t[famount];
	hq2x.open(hq2x_path, std::ios::in | std::ios::binary);
	for (size_t i = 0; i < famount; i++) {
		hq2x.seekg(0x10 + (std::streamoff(i) * 0x10));
		hq2x.read(reinterpret_cast<char*>(&hq2x_hashes[i]), 4); // read the original CRC
	}
	hq2x.close();

	table.open(table_path, std::ios::in | std::ios::binary);
	table.seekg(0xC);
	uint32_t t_famount = 0;
	table.read(reinterpret_cast<char*>(&t_famount), sizeof(t_famount));

	std::vector<TableEntry> table_entries;
	table_entries.reserve(t_famount);
	for (int j = 0; j < t_famount; ++j) {
		table.seekg(0x10 + (std::streamoff(j) * 0x60));
		TableEntry entry;

		// Make 0x5C zeroes UPD: not needed
		//{
		//	char c = 0;
		//	do {
		//		entry.name += c;
		//	} while (c && entry.name.size() < 0x5C);
		//	entry.name += c;
		//}

		table.seekg(0x6C + (std::streamoff(j) * 0x60));
		table.read(reinterpret_cast<char*>(&entry.hash), 4);
		table_entries.push_back(entry);
	}
	table.close();

	std::vector<uint32_t> unknown_hashes;
	for (int j = 0; j < famount; ++j) {
		bool found = false;
		for (int c = 0; !found && (c < t_famount); ++c) {
			found = (table_entries[c].hash == hq2x_hashes[j]);
		}
		if (!found) {
			unknown_hashes.push_back(hq2x_hashes[j]);
		}
	}
	delete[] hq2x_hashes;

	std::ifstream exe_file(exe_path, std::ios::binary);
	if (!exe_file) {
		std::cerr << "[Error] Could not open " << exe_path << ".\n";
		BetterCallPause();
		return;
	}

	exe_file.seekg(0x200);
	{
		uint64_t val = 0; // .rdata\0\0  \0\000at adr.
		while (val != (0x61746164722E)) {
			exe_file.read(reinterpret_cast<char*>(&val), 8);
		}
	}
	std::printf("Found .rdata section header at %X\n", int(exe_file.tellg()) - 8);
	exe_file.seekg(8, std::ios::cur);

	uint32_t rdata_size = 0, rdata_offset = 0;

	exe_file.read(reinterpret_cast<char*>(&rdata_size), 4);
	exe_file.read(reinterpret_cast<char*>(&rdata_offset), 4);
	std::printf("rdata_size: %X\nrdata_offset: %X\n", rdata_size, rdata_offset);

	exe_file.seekg(rdata_offset);
	int str_counter = 0;
	while (exe_file.tellg() < std::streamoff(rdata_offset) + std::streamoff(rdata_size)) {
		char32_t c;
		exe_file.read(reinterpret_cast<char*>(&c), 4);
		std::u32string str;
		if (c > 0x1F && c < 0x7F) { // ANSI character
			str += c;
		}
		else if (c == 0 && !str.empty()) { // if NULL byte and there were characters before
			//std::printf("\t[Found string at %X]\n%s\n", int(exe_file.tellg()) - str.size() * 4, converter.to_bytes(str).c_str());
			++str_counter;

			//Font fix?
			/*
			shaders/compiled/^.cso.^.^
			.shaders
			.z

			pixel.maskcutout
			pixel.textured
			vertex.my
			pixel.solid
			pixel.overlaycolor
			pixel.grayscale.fast
			pixel.gradientramp.rgb
			pixel.gradientramp.a

			pixel.grayscale.proper
			pixel.wireframe

			*/
			if (str == U"font-tiny-0to31" ||
				str == U"font-small-0to31" ||
				str == U"font-medium-0to31" ||
				str == U"font-large-0to31")
			{
				str = U"gfx/font/" + str + U"-1.tga";
			}
			else if (str.find(U"vertex") != std::u32string::npos || str.find(U"pixel") != std::u32string::npos) {
				str = U"shaders/compiled/" + str + U".cso";
			}
			size_t end_point = str.size();
			for (int q = 0; q <= 11; ++q) {
				switch (q) {
				case 1:
					str.erase(end_point);
					str += U".ini.preload";
					//std::printf("\n%s\n", converter.to_bytes(str).c_str());
					break;
				case 2:
					str.erase(end_point);
					str += U".wadcreate";
					//std::printf("\n%s\n", converter.to_bytes(str).c_str());
					break;
				case 3:
					str.erase(end_point);
					str += U".charmap";
					break;
				case 4:
					str.erase(end_point);
					str += U".vs_4_0.release";
					break;
				case 5:
					str.erase(end_point);
					str += U".vs_4_0_level_9_1.release";
					break;
				case 6:
					str.erase(end_point);
					str += U".vs_4_0_level_9_3.release";
					break;
				case 7:
					str.erase(end_point);
					str += U".vs_5_0.release";
					break;
				case 8:
					str.erase(end_point);
					str += U".ps_4_0.release";
					break;
				case 9:
					str.erase(end_point);
					str += U".ps_4_0_level_9_1.release";
					break;
				case 10:
					str.erase(end_point);
					str += U".ps_4_0_level_9_3.release";
					break;
				case 11:
					str.erase(end_point);
					str += U".ps_5_0.release";
					break;
				default:
					break;
				}

				std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
				unsigned char* ansi_str = new unsigned char[converter.to_bytes(str).size() + 1] {0};
				std::string ansi_str_c = converter.to_bytes(str).c_str();
				strcpy_s(reinterpret_cast<char*>(ansi_str), converter.to_bytes(str).size() + 1, ansi_str_c.c_str());
				uint32_t test_hash = djb2_hash(ansi_str);

				for (int i = 0; i < unknown_hashes.size(); ++i) {
					if (unknown_hashes[i] == test_hash) {
						std::printf("\tHash match: %X %s\n", test_hash, converter.to_bytes(str).c_str());
						table_entries.push_back(TableEntry{ converter.to_bytes(str), test_hash });
						break;
					}
				}
				delete[] ansi_str;
			}
			str.clear();
		}
		else if (!str.empty()) { // if not a character
			str.clear();
		}
	}
	exe_file.close();
	std::cout.flush();
	std::printf("\n\nTotal strings found: %d\nWriting changes to the table.\n", str_counter);

	table.open(table_path, std::ios::in | std::ios::out | std::ios::binary);
	if (!table) {
		std::cerr << "[Error] Could not open " << table_path << " for writing.\n";
		BetterCallPause();
		return;
	}
	for (size_t i = t_famount; i < table_entries.size(); ++i) {
		table.seekp(0x10 + (std::streamoff(i) * 0x60));
		table.write(table_entries[i].name.c_str(), table_entries[i].name.size());
		table.seekp(0x6C + (std::streamoff(i) * 0x60));
		table.write(reinterpret_cast<char*>(&table_entries[i].hash), sizeof(uint32_t));
	}
	table.seekp(0xC);
	uint32_t new_t_size = table_entries.size();
	table.write(reinterpret_cast<char*>(&new_t_size), sizeof(uint32_t));
	table.close();
	std::printf("The table has been saved.\n");
}

// Done
void modeTableInfo() {
	std::cout << "Table info mode.\nChose a table file.\n";

	std::filesystem::path table_path;
	table_path = OpenFileWindow("IAMT Table Format (.iamt_table)\0*.IAMT_TABLE\0All Files\0*.*\0");
	std::fstream ciu_table;
	ciu_table.open(table_path, std::ios::in | std::ios::binary);
	if (!ciu_table) {
		std::cerr << "[Error] Can't open the file. Try running the tool as administrator.\n";
		BetterCallPause();
		return;
	}

	uint64_t format_check;
	ciu_table.read(reinterpret_cast<char*>(&format_check), sizeof(format_check));
	uint32_t version;
	ciu_table.read(reinterpret_cast<char*>(&version), sizeof(version));
	uint32_t famount;
	ciu_table.read(reinterpret_cast<char*>(&famount), sizeof(famount));
	ciu_table.seekg(0, std::ios::end);
	std::streamsize file_size;
	file_size = ciu_table.tellg();
	std::cout << "\nFile size: " << int_to_str(file_size, 4) << '\n';

	uint32_t true_famount = 0;
	true_famount = (file_size - std::streamoff(0x10)) / 0x60;
	std::cout << "Table version: " << version << "\nFiles amount: " << famount << '\n';
	if (file_size != ((std::streamoff(famount) * 0x60) + 0x10) || famount != true_famount) {
		std::cout << "Table size is incorrect - files amount: " << famount << ", should be: " << true_famount << '\n';
	}

	//not IAMT CIU
	if (format_check != 0x55494320544D4149) {
		//CIMT CIU
		if (format_check == 0x55494320544D4943) {
			std::cout << "This table is in the old CIMT format.\nChange the name to .iamt_table and the first 2 bytes to \"IA\" using a HEX editor to make it work with IAMT v3.0 and higher.\n\n";
		}
		//IAMI CIU
		else if (format_check == 0x55494320494D4149) {
			std::cout << "This table is encrypted and used with InterAction Mod Installer. Get IAMT table to use it with Mod Tool.\n";
			BetterCallPause();
			return;
		}
		else {
			std::cout << "No known table formats found in the file.\n";
			BetterCallPause();
			return;
		}
	}

	std::cout << "   Num. [CRC       ] Path\n";
	for (uint32_t j = 0; j < true_famount; j++) {
		ciu_table.clear();
		ciu_table.seekg(0x10 + 0x60 * j + 0x5C);
		uint32_t crc;
		ciu_table.read(reinterpret_cast<char*>(&crc), sizeof(crc));
		std::string name;
		char c = 0xFF; // (3232,8): warning C4309: =: усечение константного значения. Gone after restart.
		int temp_off = 0;
		if (ciu_table.eof()) {
			name = "CORRUPTED ENTRY";
			crc = 0xbaaaaaad;
		}
		while (uint8_t(c) != 0x00 && !ciu_table.eof()) {
			ciu_table.seekg(0x10 + 0x60 * std::streamoff(j) + std::streamoff(temp_off++));
			c = ciu_table.get();
			if (uint8_t(c) != 0x00) {
				name += c;
			}
		}
		std::ostringstream offset_display;
		std::ostringstream number_display;
		offset_display << "[" << int_to_str(crc, 4) << "]";
		number_display << j + 1 << ".";
		std::cout << std::setw(7) << std::right << number_display.str() << " " << std::setw(12) << std::left << offset_display.str() << " " << name << '\n';
		offset_display.str("");
		number_display.str("");
	}
}

// Done (did not check thoroughly, revisit in case of issues)
void modeTableEdit(bool wait_for_files) {
	std::cout << "Table edit mode.\nChose a table file.\n";
	std::filesystem::path table_path;
	table_path = OpenFileWindow("IAMT Table Format (.iamt_table)\0*.IAMT_TABLE\0All Files\0*.*\0");
	std::fstream ciu_table;
	ciu_table.open(table_path, std::ios::in | std::ios::binary);
	if (!ciu_table) {
		std::cerr << "[Error] Can't open the file. Try running the tool as administrator.\n";
		BetterCallPause();
		return;
	}
	std::filesystem::path temp_path = table_path;
	temp_path += ".tmp";
	std::fstream temp_file;
	temp_file.open(temp_path, std::ios::out | std::ios::binary);
	if (!temp_file) {
		std::cerr << "[Error] Can't create the temp file.\n";
		BetterCallPause();
		return;
	}
	uint64_t format_check;
	ciu_table.read(reinterpret_cast<char*>(&format_check), sizeof(format_check));
	uint32_t version;
	ciu_table.read(reinterpret_cast<char*>(&version), sizeof(version));
	uint32_t famount;
	ciu_table.read(reinterpret_cast<char*>(&famount), sizeof(famount));
	ciu_table.seekg(0, std::ios::end);
	std::streamsize file_size;
	file_size = ciu_table.tellg();
	std::cout << "\nFile size: " << int_to_str(file_size, 4) << '\n';

	uint32_t true_famount = (file_size - 0x10) / 0x60;
	std::cout << "Table version: " << version << "\nFiles amount: " << famount << '\n';
	if (file_size != ((std::streamoff(famount) * 0x60) + 0x10) || famount != true_famount) {
		std::cout << "Table size is incorrect - files amount: " << famount << ", should be: " << true_famount << '\n';
	}
	ciu_table.clear();
	//not IAMT CIU
	if (format_check != 0x55494320544D4149) {
		//CIMT CIU
		if (format_check == 0x55494320544D4943) {
			std::cout << "This table is in old CIMT format.\nChange the name to .iamt_table and 2 first bytes to \"IA\" using HxD to make it work with IAMT v3.0 and higher.\n\n";
		}
		//IAMI CIU
		else if (format_check == 0x55494320494D4149) {
			std::cout << "This table is encrypted and used with InterAction Mod Installer. Get IAMT table to use it with Mod Tool.\n";
			BetterCallPause();
			return;
		}
		else {
			std::cout << "No known table formats found in the file.\n";
			BetterCallPause();
			return;
		}
	}

	ciu_table.seekg(0, std::ios::end);
	std::streamsize buff_size = ciu_table.tellg();
	char* buffer = new char[buff_size];
	ciu_table.seekg(0, std::ios::beg);
	ciu_table.read(buffer, buff_size);
	temp_file.write(buffer, buff_size);
	delete[] buffer;
	temp_file.close();
	temp_file.open(temp_path, std::ios::in | std::ios::out | std::ios::binary);
	if (!temp_file) {
		std::cerr << "[Error] Can't create the temp file.\n";
		BetterCallPause();
		return;
	}
	std::cout << "   Num. [CRC       ] Path\n";
	for (uint64_t j = 0; j < true_famount; j++) {
		ciu_table.clear();
		ciu_table.seekg(0x10 + 0x60 * j + 0x5C);
		uint32_t crc;
		ciu_table.read(reinterpret_cast<char*>(&crc), sizeof(crc));
		std::string name;
		char c = 0xFF;//(3232,8): warning C4309: =: усечение константного значения. Gone after restart.
		int temp_off = 0;
		if (ciu_table.eof()) {
			name = "CORRUPTED ENTRY";
			crc = 0xbaaaaaad;
		}
		while (uint8_t(c) != 0x00 && !ciu_table.eof()) {
			ciu_table.seekg(0x10 + 0x60 * j + uint64_t(temp_off));
			temp_off++;
			c = ciu_table.get();
			if (uint8_t(c) != 0x00)
				name += c;
		}
		std::ostringstream offset_display;
		std::ostringstream number_display;
		offset_display << "[" << int_to_str(crc, 4) << "]";
		number_display << j + 1 << ".";
		std::cout << std::setw(7) << std::right << number_display.str() << " " << std::setw(12) << std::left << offset_display.str() << " " << name << '\n';
		offset_display.str("");
		number_display.str("");
	}

	for (;;) {
		std::cout << "Enter the command (type \"help\" to see all of them): ";
		std::string inpt;
		std::getline(std::cin, inpt);
		if (inpt == "") {
			std::cout << "\n\nTyping finished. Deleting temp file.\n";
			temp_file.close();
			remove(temp_path);
			break;
		}
		else if (inpt.find("help") != std::string::npos && inpt.find("help") < 5) {
			std::cout << "\nCommands:\n"
				"edit X CRC NAME -> edit Xth entry to CRC and NAME (edit 5 000a2f3C This is an example).\n"
				"remove X        -> Remove Xth entry from the file (remove 8). WILL NOT work with corrupted entries.\n"
				"fix             -> Removes last recognized entry to kill corrupted one, even if last entry wasn't corrupted.\n"
				"add CRC NAME    -> Add new entry with CRC and NAME to the end. DON'T USE if the last entry of the file IS CORRUPTED (add 14a sfx/melorise.wav).\n"
				"version X       -> Change the version of the table to X (must be >= 0 and <= 4 294 967 295).\n"
				"files X         -> Change files amount variable to X.\n"
				"save            -> Saves edited table at the same place.\n"
				"save as, saveas -> Saves edited table wherever you want.\n"
				"show            -> Show edited file content (temp file).\n"
				"help            -> Show this screen.\n"
				"[Empty line]    -> Finish typing and exit from the tool.\n\n";
			continue;
		}
		//edit X CRC NAME
		else if (inpt.find("edit") != std::string::npos && inpt.find("edit") < 4) {
			std::vector<std::string> arguments = SeparateEx(inpt, ' ', 4);
			if (arguments.size() < 4) {
				std::cout << "Not enough data!\n";
				continue;
			}

			//read X and CRC
			uint32_t edit_goto;
			try {
				edit_goto = stoi(arguments[1]);//X
			}
			catch (const std::invalid_argument&) {
				std::cout << "Can't find a proper entry number!\n";
				continue;
			}
			uint32_t temp_crc;
			try {
				temp_crc = stoul(arguments[2], nullptr, 16);//CRC
			}
			catch (const std::invalid_argument&) {
				std::cout << "Can't find proper CRC!\n";
				continue;
			}

			std::string temp_name = inpt.substr(inpt.find(arguments[3], 5 + arguments[1].size() + arguments[2].size()));
			while (temp_name.size() < 0x5C) { temp_name += '\0'; }

			if (edit_goto >= 1 && edit_goto <= true_famount) {
				temp_file.seekp(0x10 + 0x60 * (uint64_t(edit_goto) - 1));
				// cut if bigger
				if (temp_name.size() > 0x5C || temp_name[0x5B] != '\0') {
					temp_name.erase(0x5B);
					temp_name += '\0';
				}
				temp_file.write(temp_name.c_str(), temp_name.size());
				temp_file.seekp(0x6C + 0x60 * (uint64_t(edit_goto) - 1));
				temp_file.write((char*)&temp_crc, sizeof(temp_crc));
				edit_goto = 0;
				continue;
			}
			else {
				std::cout << "Input value is incorrect: out of range.\n";
				continue;
			}
		}
		else if (inpt.find("save") != std::string::npos && inpt.find("save") < 4) {
			if (inpt.find("as") != std::string::npos) {
				std::cout << "Chose file path.\n";
				std::string save_path = SaveFileWindow("IAMT Table format (.iamt_table)\0*.iamt_table\0All Files\0*.*\0", "iamt_table");
				std::cout << "Saving...\n";
				std::ofstream new_file(save_path, std::ios::binary);
				if (!new_file) {
					std::cerr << "[Error] Can't save the file.\n";
					BetterCallPause();
					return;
				}
				temp_file.seekg(0, std::ios::end);
				buff_size = temp_file.tellg();
				temp_file.seekg(0);
				char* buffer = new char[buff_size];
				temp_file.read(buffer, buff_size);
				new_file.write(buffer, buff_size);
				new_file.close();
				delete[] buffer;
				std::cout << "Saved.\n";
			}
			else {
				std::cout << "Saving...\n";
				ciu_table.close();
				std::ofstream new_file(table_path, std::ios::binary);
				if (!new_file) {
					std::cerr << "[Error] Can't save the file.\n";
					BetterCallPause();
					return;
				}
				temp_file.seekg(0, std::ios::end);
				buff_size = temp_file.tellg();
				temp_file.seekg(0);
				char* buffer = new char[buff_size];
				temp_file.read(buffer, buff_size);
				new_file.write(buffer, buff_size);
				new_file.close();
				delete[] buffer;
				std::cout << "Saved.\n";
				ciu_table.open(table_path, std::ios::in | std::ios::binary);
				if (!ciu_table) {
					std::cerr << "[Error] Can't open the file. Try running the tool as administrator.\n";
					BetterCallPause();
					return;
				}
			}
		}
		else if (inpt.find("version") != std::string::npos && inpt.find("version") < 4) {
			uint32_t temp_ver;
			try {
				temp_ver = stoul(inpt.substr(inpt.find("n") + 1));
			}
			catch (const std::invalid_argument&) {
				std::cout << "Can't find proper version number!\n";
				continue;
			}
			temp_file.seekp(0x8);
			temp_file.write(reinterpret_cast<char*>(&temp_ver), sizeof(temp_ver));
		}
		else if (inpt.find("files") != std::string::npos && inpt.find("files") < 4) {
			uint32_t temp_famount;
			try {
				temp_famount = stoul(inpt.substr(inpt.find("s") + 1));
			}
			catch (const std::invalid_argument&) {
				std::cout << "Can't find proper amount number!\n";
				continue;
			}
			temp_file.seekp(0xC);
			temp_file.write(reinterpret_cast<char*>(&temp_famount), sizeof(temp_famount));
		}
		//add CRC NAME
		else if (inpt.find("add") != std::string::npos && inpt.find("add") < 4) {
			std::vector<std::string> arguments = SeparateEx(inpt, ' ', 3);
			if (arguments.size() < 3) {
				std::cout << "Not enough data!\n";
				continue;
			}
			uint32_t temp_crc;
			try {
				temp_crc = stoul(arguments[1], nullptr, 16);//CRC
			}
			catch (const std::invalid_argument&) {
				std::cout << "Can't find proper CRC!\n";
				continue;
			}
			std::string temp_name = inpt.substr(inpt.find(arguments[2], 4 + arguments[1].size()));
			while (temp_name.size() < 0x5C)
				temp_name += '\0';

			temp_file.seekp(0, std::ios::end);
			//cut if bigger
			if (temp_name.size() > 0x5C || temp_name[0x5B] != '\0') {
				temp_name.erase(0x5B);
				temp_name += '\0';
			}
			temp_file.write(temp_name.c_str(), temp_name.size());
			temp_file.write((char*)&temp_crc, sizeof(temp_crc));
			true_famount++;
			continue;
		}
		else if (inpt.find("remove") != std::string::npos && inpt.find("remove") < 4) {
			uint32_t edit_goto;
			try {
				edit_goto = stoul(inpt.substr(inpt.find("ve") + 2));
			}
			catch (const std::invalid_argument&) {
				std::cout << "Can't find proper entry number!\n";
				continue;
			}
			temp_file.seekg(0, std::ios::end);
			std::streamsize temp_fsize = temp_file.tellg();
			char* buffer = new char[temp_fsize - (0x10 + 0x60 * (edit_goto - 1))];
			temp_file.seekg(0x10 + 0x60 * uint64_t(edit_goto));
			temp_file.read(buffer, temp_fsize - (0x10 + 0x60 * (edit_goto - 1)));
			temp_file.seekp(0x10 + 0x60 * (uint64_t(edit_goto) - 1));
			temp_file.write(buffer, temp_fsize - (0x10 + 0x60 * (edit_goto - 1)));
			temp_file.clear();
			delete[] buffer;
			std::filesystem::resize_file(temp_path, temp_fsize - 0x60);
		}
		else if (inpt.find("fix") != std::string::npos && inpt.find("fix") < 4) {
			std::streamsize j2 = 0;
			temp_file.clear();
			temp_file.seekg(0);
			char c = 0xFF;
			while (!temp_file.eof()) {
				temp_file.seekg(0x10 + 0x60 * j2);
				j2++;
				c = temp_file.get();
			}
			std::cout << "Found end of file. Erasing last entry.\n";
			temp_file.clear();
			std::filesystem::resize_file(temp_path, 0x10 + 0x60 * (j2 - 2));
		}
		else if (inpt.find("show") != std::string::npos && inpt.find("show") < 4) {
			std::cout << "\n\nNOTE: This info contains changes you made. If you want to see the file itself, restart the tool.\n";
			temp_file.seekg(0);
			temp_file.read(reinterpret_cast<char*>(&format_check), sizeof(format_check));
			temp_file.read(reinterpret_cast<char*>(&version), sizeof(version));
			temp_file.read(reinterpret_cast<char*>(&famount), sizeof(famount));
			temp_file.seekg(0, std::ios::end);
			file_size = temp_file.tellg();
			std::cout << "File size: " << int_to_str(file_size, 4) << '\n';

			while (0x10 + (0x60 * true_famount) < file_size) {
				temp_file.seekg(0x10 + (0x60 * true_famount));
				true_famount++;
			}
			std::cout << "Table version: " << version << "\nFiles amount: " << famount << '\n';
			if (file_size != ((uint64_t(famount) * 0x60) + 0x10) || famount != true_famount) {
				std::cout << "Table size is incorrect - files amount: " << famount << ", should be: " << true_famount << '\n';
			}
			temp_file.clear();
			//not IAMT CIU
			if (format_check != 0x55494320544D4149) {
				//CIMT CIU
				if (format_check == 0x55494320544D4943) {
					std::cout << "This table is in the old CIMT format.\nChange the name to .iamt_table and 2 first bytes to \"IA\" using HxD to make it work with IAMT v3.0 and higher.\n\n";
				}
				//IAMI CIU
				else if (format_check == 0x55494320494D4149) {
					std::cout << "This table is encrypted and used with InterAction Mod Installer. Get IAMT table to use it with Mod Tool.\n";
					BetterCallPause();
					continue;
				}
				else {
					std::cout << "No known table formats found in the file. Looks like the table got corrupted by changes you did. If this happened, send info to us.\n";
					BetterCallPause();
					continue;
				}
			}
			std::cout << "   Num. [CRC       ] Path\n";
			for (std::streamoff j = 0; j < true_famount; j++) {
				temp_file.clear();
				temp_file.seekg(0x10 + 0x60 * j + 0x5C);
				uint32_t crc;
				temp_file.read(reinterpret_cast<char*>(&crc), sizeof(crc));
				std::string name;
				char c = 0xFF;
				std::streamoff temp_off = 0;
				if (temp_file.eof()) {
					name = "CORRUPTED ENTRY";
					crc = 0xbaaaaaad;
				}
				while (uint8_t(c) != 0x00 && !temp_file.eof()) {
					temp_file.seekg(0x10 + 0x60 * j + temp_off);
					temp_off++;
					c = temp_file.get();
					if (uint8_t(c) != 0x00) {
						name += c;
					}
				}
				std::ostringstream offset_display;
				std::ostringstream number_display;
				offset_display << "[" << int_to_str(crc, 4) << "]";
				number_display << j + 1 << ".";
				std::cout << std::setw(7) << std::right << number_display.str() << " " << std::setw(12) << std::left << offset_display.str() << " " << name << '\n';
				offset_display.str("");
				number_display.str("");
			}
		}
	}
}

// Done
void modeTableConvert() {
	std::cout << "Table convert mode.\nChoose a CIU.iamt_table.\n";
	std::filesystem::path table_path;
	table_path = OpenFileWindow("IAMT Table format (*.iamt_table)\0*.iamt_table\0All Files\0*.*\0");

	std::cout << "Choose save path.\n";
	std::filesystem::path iami_path;
	iami_path = SaveFileWindow("IAMI format (*.iami)\0*.iami\0All Files\0*.*\0", "iami");

	std::fstream ciu_table;
	ciu_table.open(table_path, std::ios::in | std::ios::binary);
	if (!ciu_table) {
		std::cerr << "[Error] Can't open .iamt_table!\n";
		BetterCallPause();
		return;
	}
	ciu_table.seekg(0, std::ios::end);
	std::streamsize buf_size = ciu_table.tellg();
	char* buf = new char[buf_size];
	if (!buf) {
		std::cerr << "[Error] Could not allocate memory for the buffer (" << buf_size << " bytes).\n";
		return;
	}
	ciu_table.seekg(0);
	ciu_table.read(buf, buf_size);
	ciu_table.close();

	std::fstream iami;
	iami.open(iami_path, std::ios::out | std::ios::binary);
	if (!iami) {
		std::cerr << "[Error] Can't create IAMI table file!\n";
		delete[] buf;
		BetterCallPause();
		return;
	}

	iami.write(buf, buf_size);
	delete[] buf;
	iami.close();
	iami.open(iami_path, std::ios::in | std::ios::out | std::ios::binary);
	iami.seekp(3);
	iami.put(char(0x49));
	iami.seekg(0xC);
	std::uint32_t famount;
	iami.read(reinterpret_cast<char*>(&famount), sizeof(famount));

	std::cout << "Protection: stage 1...\n";
	for (size_t i = 0; i < famount; i++) {
		char c = 0xFF;
		int end = -1;
		for (size_t j = 0; j < 0x5C; j++) {
			iami.seekg(0x10 + 0x60 * std::streamoff(i) + std::streamoff(j));
			c = iami.get();
			if (uint8_t(c) == 0) {
				if (end == -1) {
					end = j;
					continue;
				}
				else if (j > end) {
					char q = rand() % 255 + 1;
					iami.seekp(0x10 + 0x60 * std::streamoff(i) + std::streamoff(j));
					iami.put(q);
				}
			}
		}
	}
	std::cout << "Protection: stage 2...\n";
	iami.seekg(0, std::ios::end);
	buf_size = iami.tellg();
	buf = new char[buf_size];
	iami.seekg(0);
	iami.read(buf, buf_size);
	protection(buf, buf_size, 'e');
	iami.seekp(0);
	iami.write(buf, buf_size);
	iami.close();
	delete[] buf;
	std::cout << "Done.\n";
}

// Done
void modeModEdit(bool wait_for_files) {
	std::cout << "\nMod edit mode. Choose mod file.\n";

	std::string file_path = OpenFileWindow(
		"IAMI CIU Mod (*.CIUMod)\0*.CIUMod\0"
		"IAMI CI5 Mod (*.CI5Mod)\0*.CI5Mod\0"
		"IAMI CI4 Mod (*.CI4Mod)\0*.CI4Mod\0"
		"IAMI CI3 Mod (*.CI3Mod)\0*.CI3Mod\0"
		"IAMI Old CI3 Mod (*.CI3OldMod)\0*.CI3OldMod\0"
		"IAMI CI2 Mod (*.CI2Mod)\0*.CI2Mod\0"
		"IAMI CI2 Remastered Mod (*.CI2RMod)\0*.CI2RMod\0"
		"IAMI CI1 Mod (*.CI1Mod)\0*.CI1Mod\0"
		"IAMI UNI-Mod (*.UNIMod)\0*.UNIMod\0"
		"IAMI Smileyville Mod (*.SVMod)\0*.SVMod\0"
		"IAMI Piggly Mod (*.PIMod)\0*.PIMod\0"
		"All Files\0*.*\0");
	std::fstream mod(file_path, std::ios::in | std::ios::binary);
	if (!mod) {
		std::cerr << "[Error] Can't open the file. Try running the tool as administrator.\n";
		BetterCallPause();
		exit(1);
	}
	mod.seekg(4);
	uint32_t format_check;
	mod.read(reinterpret_cast<char*>(&format_check), sizeof(uint32_t));
	if (format_check != 0x6799186F)//****IAMF
	{
		std::cerr << "[Error] Bruh this file is not a mod ._ .\nNow go crash\n";
		BetterCallPause();
		exit(1);
	}
	std::fstream temp_file(file_path + ".tmp",
		std::ios::trunc | std::ios::out | std::ios::in | std::ios::binary);
	if (!temp_file) {
		std::cerr << "[Error] Can't create the temp file.\n";
		BetterCallPause();
		exit(1);
	}
	mod.seekg(0, std::ios::end);
	uint64_t buffer_size = mod.tellg();

	char* buffer = new char[buffer_size];
	mod.seekg(0);
	mod.read(buffer, buffer_size);
	mod.close();
	protection(buffer, buffer_size, 'd');//decode all shit to reduce cancer
	temp_file.write(buffer, buffer_size);
	temp_file.seekg(0);
	delete[] buffer;

	char mod_game_code[4] = { 0 };
	temp_file.read(mod_game_code, sizeof(uint32_t));
	temp_file.seekg(8);
	uint32_t mod_version;
	uint32_t mod_files_amount;
	temp_file.read(reinterpret_cast<char*>(&mod_version), sizeof(uint32_t));
	temp_file.read(reinterpret_cast<char*>(&mod_files_amount), sizeof(uint32_t));

	buffer = new char[0x50];
	temp_file.seekg(0x10);
	temp_file.read(buffer, 0x50);
	std::string mod_name;
	std::string mod_description;
	mod_name = buffer;
	delete[] buffer;

	buffer = new char[0x100];
	temp_file.seekg(0x60);
	temp_file.read(buffer, 0x100);
	mod_description = buffer;
	delete[] buffer;

	std::cout << "\n\n[Mod information]\n"
		"Name: " << mod_name <<
		"\nVersion: " << mod_version <<
		"\n\n[Description]\n" << mod_description <<
		"\n\n";
	for (;;) {
		std::cout << "Enter the command (type \"help\" to see all of them): ";
		std::string inpt;
		std::getline(std::cin, inpt);
		if (inpt == "") {
			std::cout << "\n\nTyping finished. Deleting temp file.\n";
			temp_file.close();
			remove((file_path + ".tmp").c_str());
			break;
		}
		else if (inpt.find("help") != std::string::npos && inpt.find("help") < 5) {
			std::cout << "\nCommands:\n"
				"version X       -> Change the version of the mod to X (must be >= 0 and <= 4 294 967 295).\n"
				"name X          -> Change mod name to X (name assasin\'s creed (what))"
				"description X   -> Change description to X (description mod which adds banana peal)"
				"save            -> Saves edited table at the same place.\n"
				"save as, saveas -> Saves edited table wherever you want.\n"
				"help            -> Show this screen.\n"
				"[Empty line]    -> Finish typing and exit from the tool.\n\n";
			continue;
		}
		else if (inpt.find("save") != std::string::npos && inpt.find("save") < 4) {
			if (inpt.find("as") != std::string::npos) {
				std::cout << "Chose file path.\n";
				std::string save_path = SaveFileWindow("IAMI CIU Mod (*.CIUMod)\0*.CIUMod\0"
					"IAMI CI5 Mod (*.CI5Mod)\0*.CI5Mod\0"
					"IAMI CI4 Mod (*.CI4Mod)\0*.CI4Mod\0"
					"IAMI CI3 Mod (*.CI3Mod)\0*.CI3Mod\0"
					"IAMI Old CI3 Mod (*.CI3OldMod)\0*.CI3OldMod\0"
					"IAMI CI2 Mod (*.CI2Mod)\0*.CI2Mod\0"
					"IAMI CI2 Remastered Mod (*.CI2RMod)\0*.CI2RMod\0"
					"IAMI CI1 Mod (*.CI1Mod)\0*.CI1Mod\0"
					"IAMI UNI-Mod (*.UNIMod)\0*.UNIMod\0"
					"IAMI Smileyville Mod (*.SVMod)\0*.SVMod\0"
					"IAMI Piggly Mod (*.PIMod)\0*.PIMod\0"
					"All Files\0*.*\0", ".IAMIMod");
				std::cout << "Saving...\n";
				std::ofstream new_file(save_path, std::ios::trunc | std::ios::binary);
				if (!new_file) {
					std::cerr << "[Error] Can't save the file.\n";
					BetterCallPause();
					exit(1);
				}
				//CRC and Hardcode
				//Calculate CRC
				temp_file.seekg(-4, std::ios::end);
				uint32_t fsize = temp_file.tellg();
				temp_file.seekg(0);
				buffer = new char[fsize];
				temp_file.read(buffer, fsize);
				uint32_t CRC = CRC32(buffer, fsize);
				CRC += 0x93E56B02;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				delete[] buffer;
				temp_file.seekp(fsize);
				temp_file.write((char*)&CRC, sizeof(CRC));

				//Hardcoded mess
				temp_file.seekg(0, std::ios::end);
				fsize = temp_file.tellg();
				temp_file.seekg(0);
				buffer = new char[fsize];
				temp_file.read(buffer, fsize);
				protection(buffer, fsize, 'e');

				new_file.write(buffer, buffer_size);
				new_file.close();
				delete[] buffer;
				std::cout << "Saved.\n";
			}
			else {
				std::cout << "Saving...\n";
				std::ofstream new_file(file_path, std::ios::trunc | std::ios::binary);
				if (!new_file) {
					std::cerr << "[Error] Can't save the file.\n";
					BetterCallPause();
					exit(1);
				}
				//CRC and Hardcode
				//Calculate CRC
				temp_file.seekg(-4, std::ios::end);
				uint32_t fsize = temp_file.tellg();
				temp_file.seekg(0);
				buffer = new char[fsize];
				temp_file.read(buffer, fsize);
				uint32_t CRC = CRC32(buffer, fsize);
				CRC += 0x93E56B02;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				delete[] buffer;
				temp_file.seekp(fsize);
				temp_file.write((char*)&CRC, sizeof(CRC));

				//Hardcoded mess
				temp_file.seekg(0, std::ios::end);
				fsize = temp_file.tellg();
				temp_file.seekg(0);
				buffer = new char[fsize];
				temp_file.read(buffer, fsize);
				protection(buffer, fsize, 'e');

				new_file.write(buffer, buffer_size);
				new_file.close();
				delete[] buffer;
			}
		}
		else if (inpt.find("version") != std::string::npos && inpt.find("version") < 4) {
			uint32_t temp_ver;
			try {
				uint32_t temp_ver = stoul(inpt.substr(inpt.find("n") + 1));
			}
			catch (const std::invalid_argument&) {
				std::cout << "Can't find proper version number!\n";
				continue;
			}
			temp_file.seekp(0x8);
			temp_file.write((char*)&temp_ver, sizeof(uint32_t));
		}
		else if (inpt.find("name") != std::string::npos && inpt.find("name") < 4) {
			std::string temp_name = inpt.substr(inpt.find("name") + 5);
			while (temp_name.size() < 0x50)
				temp_name += '\0';
			if (temp_name.size() > 0x50) {
				temp_name.erase(0x4F);
				temp_name += '\0';
			}
			temp_file.seekp(0x10);
			temp_file.write(temp_name.c_str(), 0x50);
		}
		else if (inpt.find("description") != std::string::npos &&
			inpt.find("description") < 4)
		{
			std::string temp_name = inpt.substr(inpt.find("description") + 12);
			while (temp_name.find("\\n") != std::string::npos) {
				temp_name[temp_name.find("\\n")] = '\n';
				temp_name.erase(temp_name.rfind('\n') + 1, 1);
			}
			while (temp_name.size() < 0x100) {
				temp_name += '\0';
			}
			if (temp_name.size() > 0x100) {
				temp_name.erase(0xFF);
				temp_name += '\0';
			}
			temp_file.seekp(0x60);
			temp_file.write(temp_name.c_str(), 0x100);
		}
	}
}

// Done
void modeModCreate(bool wait_for_files) {
	std::cout << "Mod creation mode.\n";
	/*
	0		- tool path
	1		- "-mc"
	2		- output mod file
	3+(2x)	- paths
	4+(2x)	- files

	make this so you can build mod file out of repack command easily by replacing hq2x with output file and -r with -mc
	*/

	std::vector<std::string>files;

	// TODO: This is very pathetic and requires a completely different approach.

	//vector<string> bad_words = {
	//	"cheat","Cheat", "CHEAT", "cheet", "Cheet", "CHEET", "chit", "Chit", "CHIT", "hack", "Hack", "HACK",
	//	"hax", "Hax", "HAX", "hitbox", "Hitbox", "HitBox", "HITBOX", "hit box", "Hit box", "Hit Box", "HIT BOX"
	//};

	std::string mod_file_path; // output
	// Read files and paths and put them in the vector
	if (wait_for_files == false) {
		for (int i = 3; i < g_argc; i++) {
			files.push_back(std::string(g_argv[i]));//0, 2 - path; 1, 3 - file
		}
		mod_file_path = g_argv[2];
	}
	else {

		if (g_argc < 3) {
			std::cout << "Enter output file path:\n";
			mod_file_path = SaveFileWindow("IAMI CIU Mod (*.CIUMod)\0*.CIUMod\0"
				"IAMI CI5 Mod (*.CI5Mod)\0*.CI5Mod\0"
				"IAMI CI4 Mod (*.CI4Mod)\0*.CI4Mod\0"
				"IAMI CI3 Mod (*.CI3Mod)\0*.CI3Mod\0"
				"IAMI Old CI3 Mod (*.CI3OldMod)\0*.CI3OldMod\0"
				"IAMI CI2 Mod (*.CI2Mod)\0*.CI2Mod\0"
				"IAMI CI2 Remastered Mod (*.CI2RMod)\0*.CI2RMod\0"
				"IAMI CI1 Mod (*.CI1Mod)\0*.CI1Mod\0"
				"IAMI UNI-Mod (*.UNIMod)\0*.UNIMod\0"
				"IAMI Smileyville Mod (*.SVMod)\0*.SVMod\0"
				"IAMI Piggly Mod (*.PIMod)\0*.PIMod\0"
				"All Files\0*.*\0", ".IAMIMod");
		}
		else {
			mod_file_path = g_argv[2];
		}
		std::cout <<
			"\nEnter the file paths you want to work with. Put an empty line when you finish.\n"
			"(Click RMB to paste path from buffer)\n\n"
			"\"folder\" - use folder as the root of paths\n"
			"\"txt\" - use text file instead (will clear the ones you wrote before)\n"
			"\"edit X\" - re-enter Xth pair.\n"
			"\"undo\" - re-enter last pair.\n"
			;
		int counter = 1; // Counts the path entries
		for (;;) {
			std::cout << counter << ". Path in hq2x: ";
			std::string inpt;
			getline(std::cin, inpt);
			while (inpt.find('\"') != std::string::npos) {
				inpt.erase(inpt.find('\"'), 1); // delete " symbols
			}
			if (inpt == "") {
				std::cout << "\n\nTyping finished.\n";
				break;
			}
			else if (inpt == "undo") {
				if (files.size() >= 2) {
					files.pop_back();
					files.pop_back();
					counter--;
					continue;
				}
				else {
					std::cout << "You've reached the beginning!\n";
					continue;
				}
			}
			else if (inpt.find("edit") == 0 && inpt.size() > 4 && inpt[4] == ' ') {
				uint32_t edit_goto = UINT32_MAX;
				try {
					edit_goto = stoi(inpt.substr(4));
				}
				catch (const std::invalid_argument&) {
					std::cout << "Can't find a number in the line!\n";
					continue;
				}
				// edit the given pair of files
				if (edit_goto >= 1 && edit_goto < counter) {
					std::cout << edit_goto << ". Path in hq2x: ";
					getline(std::cin, inpt);
					while (inpt.find('\"') != std::string::npos)
					{
						inpt.erase(inpt.find('\"'), 1);//delete " symbols
					}
					edit_goto = (edit_goto - 1) * 2;//1 = [0, 1]
					files[edit_goto] = inpt;
					std::cout << " | Replace with: ";
					inpt = OpenFileWindow(
						"All Files\0*.*\0"
						"Audio Files (*.ogg;*.wav;*.mp3)\0*.OGG;*.WAV;*.MP3\0"
#ifndef IAMT_AUDIO_ONLY
						"Graphics (*.tga; *.png; *.jpg; *.jpeg; *.bmp; *.preload)\0*.TGA;*.PNG;*.JPG;*.JPEG;*.BMP;*.INI;*.PRELOAD\0"
#endif
					);
					files[edit_goto + 1] = inpt;
					edit_goto = 0;
					continue;
				}
				else {
					std::cout << "Input value is incorrect.\n";
					continue;
				}
			}
			else if (inpt == "txt" || inpt == "TXT") {
				inpt = OpenFileWindow("Text File (*.txt)\0*.TXT\0All Files\0*.*\0");
				std::ifstream txtf(inpt);
				if (!txtf) {
					std::cout << "[Error] Can't open the file!\n";
					txtf.clear();
					continue;
				}
				files.clear();
				files = txt2vec(txtf);
				std::cout << "Done.\n";
				continue;
			}
			else if (inpt == "folder" || inpt == "FOLDER") {
				inpt = BrowseFolder(mod_file_path.substr(0, mod_file_path.rfind('\\')));
				if (!inpt.empty()) {
					files.clear();
					files = get_paths(inpt);
					std::cout << "Done.\n";
				}
				continue;
			}
			files.push_back(inpt);
			std::cout << " | Replace with: ";
			inpt = OpenFileWindow("All Files\0*.*\0Audio Files (*.ogg;*.wav;*.mp3)\0*.OGG;*.WAV;*.MP3\0"
#ifndef IAMT_AUDIO_ONLY
				"Graphics Files (*.tga; *.png; *.jpg; *.jpeg; *.bmp)\0*.TGA;*.PNG;*.JPG;*.JPEG;*.BMP\0"
#endif
			);
			files.push_back(inpt);
			counter++;
		}

	}
	if (int(files.size()) % 2 != 0)//if SOMEHOW you entered one more file
		files.pop_back();

	std::cout << "\nFinal list:\n";
	for (int i = 0; i < files.size(); i += 2) {
		std::cout << "[" << i + 1 << "] " << files[i] << " -> " << files[i + 1] << '\n';
	}

	//for (int i = 0; i < bad_words.size(); i++) {
	//	if (mod_file_path.find(bad_words[i]) != string::npos) {
	//		cerr << "[Error] Can't create mod file because no\n";
	//		cout << "Press Enter to continue...";
	//		cin.get();
	//		//cin.ignore();
	//		exit(69);
	//	}
	//}

	std::cout <<
		"Select the game you want to create mod for:\n"
		"ciu - Chicken Invaders Universe\n"
		"ci5 - Chicken Invaders 5: Cluck of the Dark Side\n"
		"ci4 - Chicken Invaders 4: Ultimate Omelette\n"
		"ci3 - Chicken Invaders 3: Revenge of the Yolk\n"
		"ci3o - CI3 v3.51 and older\n"
		"ci2 - Chicken Invaders 2: The Next Wave\n"
		"ci2r - Chicken Invaders 2 - The Next Wave Remastered\n"
		"ci1 - Classic Chicken Invaders\n"
		"pi - Piggly\n"
		"sv - Smileyville\n"
		"uni - Several games in mod description (CIU INCOMPATIBLE)\n"
		">";
	std::string game_code; // CIU\0 to ciu
	getline(std::cin, game_code);
	//There is no switch () for std::string
	// "CIU\0" "CI5\0" "CI4\0" "CI3\0" "CI3O" "CI2\0" "CI2R" "CI1\0" "PIG\0" "SMVL" "UNI\0"
	if (game_code == "ciu") game_code = "CIU\0";
	else if (game_code == "ci5") game_code = "CI5\0";
	else if (game_code == "ci4") game_code = "CI4\0";
	else if (game_code == "ci3") game_code = "CI3\0";
	else if (game_code == "ci3o") game_code = "CI3O";
	else if (game_code == "ci2") game_code = "CI2\0";
	else if (game_code == "ci2r") game_code = "CI2R";
	else if (game_code == "ci1") game_code = "CI1\0";
	else if (game_code == "pi") game_code = "PIG\0";
	else if (game_code == "sv") game_code = "SMVL";
	else if (game_code == "uni") game_code = "UNI\0";



	uint32_t mod_version = 0;
	while (mod_version == 0) {
		std::cout << "\nEnter mod version (integer 1-4\'294\'967\'295): ";
		std::string inpt{};
		getline(std::cin, inpt);
		try {
			mod_version = stoul(inpt);
		}
		catch (const std::invalid_argument&) {
			std::cout << "Can't find proper version number!\n";
			continue;
		}
	}

	std::cout << "Enter mod name (Limit - 80 symbols): ";
	std::string mod_name;
	getline(std::cin, mod_name);
	while (mod_name.size() < 0x50) {
		mod_name += '\0';
	}
	if (mod_name.size() > 0x50) {
		mod_name.erase(0x50);
	}

	//for (int i = 0; i < bad_words.size(); i++)
	//{
	//	if (mod_name.find(bad_words[i]) != string::npos)
	//	{
	//		cerr << "[Error] Can't create mod file because no\n";
	//		cout << "Press Enter to continue...";
	//		cin.get();
	//		//cin.ignore();
	//		exit(69);
	//	}
	//}

	std::cout <<
		"\nEnter mod description (Use \\n as a new line symbol).\n"
		"In universal mod you must write compatible games here too.\nLimit - 256 symbols, the rest is truncated.\n>";
	std::string mod_description;
	getline(std::cin, mod_description);
	while (mod_description.find("\\n") != std::string::npos) {
		mod_description[mod_description.find("\\n")] = '\n';
		mod_description.erase(mod_description.rfind('\n') + 1, 1);
	}
	while (mod_description.size() < 0x100) {
		mod_description += '\0';
	}
	if (mod_description.size() > 0x100) {
		mod_description.erase(0x100);
	}

	//for (int i = 0; i < bad_words.size(); i++)
	//{
	//	if (mod_description.find(bad_words[i]) != string::npos)
	//	{
	//		cerr << "[Error] Can't create mod file because no\n";
	//		cout << "Press Enter to continue...";
	//		cin.get();
	//		//cin.ignore();
	//		exit(69);
	//	}
	//}

	for (;;) {
		std::string inpt;
		std::cout << "Do you want to start? (y/n): ";
		getline(std::cin, inpt);
		if (inpt == "y") { break; }
		if (inpt == "n") {
			std::cout << "\nCreation canceled.\n";
			BetterCallPause();
			exit(0);
		}
		else {
			std::cout << "\r";
		}
	}
	//Make mess in description and name
	/*srand(static_cast<unsigned int>(time(0)));*/
	char c = 0xff;
	int end = -1;
	for (uint64_t j = 0; j < 0x50; j++) {
		c = mod_name[j];
		if (uint8_t(c) == 0x00) {
			if (end == -1) {
				end = j;
				continue;
			}
			else if (j > end) {
				char q = rand() % 255 + 1;
				mod_name[j] = q;
			}
		}
	}
	c = 0xff;
	end = -1;
	for (uint64_t j = 0; j < 0x100; j++) {
		c = mod_description[j];
		if (uint8_t(c) == 0x00) {
			if (end == -1) {
				end = j;
				continue;
			}
			else if (j > end) {
				char q = rand() % 255 + 1;
				mod_description[j] = q;
			}
		}
	}

	std::fstream mod_file(mod_file_path, std::ios::trunc | std::ios::in | std::ios::out | std::ios::binary);
	if (!mod_file) {
		std::cerr << "[Error] Can't create mod file. Try running the tool as administrator.\n";
		BetterCallPause();
		exit(1);
	}
	std::fstream temp_files;
	temp_files.open(mod_file_path + ".iamtemp", std::ios::trunc | std::ios::in | std::ios::out | std::ios::binary);
	if (!temp_files) {
		std::cerr << "[Error] Can't create temp file. Try running the tool as administrator.\n";
		BetterCallPause();
		exit(1);
	}
	mod_file.write(game_code.c_str(), sizeof(uint32_t));
	mod_file.put('I');
	mod_file.put('A');
	mod_file.put('M');
	mod_file.put('F');
	mod_file.write((char*)&mod_version, sizeof(uint32_t));
	mod_file.put('\0');//famount
	mod_file.put('\0');
	mod_file.put('\0');
	mod_file.put('\0');
	mod_file.write(mod_name.c_str(), 0x50);
	mod_file.write(mod_description.c_str(), 0x100);//1st header size = 0x160


	uint32_t famount = 0;//Calculate AFTER doing everything else, to be sure
	for (uint32_t i = 0; i < files.size(); i += 2)//0-path, 1-file
	{
		famount++;
		//command += '\"' + files[i] + "\" \"" + files[i + 1] + "\" ";
		std::string temp_path = files[i];
		while (temp_path.find('\\') != std::string::npos) {
			temp_path[temp_path.find('\\')] = '/'; // Change all \ to / 'causeyou dum if you write \ here
		}
		while (temp_path.size() < 0x58) {
			temp_path += '\0';
		}
		temp_path.erase(0x57);
		temp_path += '\0';
		mod_file.seekp(0x160 + 0x60 * (uint64_t(famount) - 1));
		for (int fill = 0; fill < 0x60; fill++) {
			mod_file.put('\0');
		}
		mod_file.seekp(0x160 + 0x60 * (uint64_t(famount) - 1));
		mod_file.write(temp_path.c_str(), 0x58);
		std::cout << "Opening " << files[i + 1] << '\n';

		std::fstream t_file;
		t_file.open(files[i + 1], std::ios::in | std::ios::binary);
		if (!t_file) {
			std::cerr << "[Error] Can't open the file " << files[i + 1] << ". Try running the tool as administrator.\n";
			for (;;) {
				std::string inpt;
				std::cout << "Do you want to skip this file? (y/n): ";
				getline(std::cin, inpt);
				if (inpt == "y") { break; }
				if (inpt == "n") {
					std::cout << "\nMod creation canceled.\n";
					BetterCallPause();
					exit(1);
				}
				else {
					std::cout << "\r";
				}
			}
			t_file.clear();
			famount--;
			continue;
		}
#ifdef IAMT_AUDIO_ONLY
		//AUDIO CHECK
		uint32_t test_val = 0;
		t_file.read(reinterpret_cast<char*>(&test_val), sizeof(uint32_t));
		if (FOURCC_CHECK(test_val)) {/*OggS*//*RIFF*/
			std::cout << "Can't use. This file is not an audio file.\n";
			t_file.close();
			--famount;
			cout << "\n\n";
			continue;
		}
		//AUDIO CHECK
#endif
		t_file.seekg(0, std::ios::end);
		uint32_t fsize = t_file.tellg();
		t_file.seekg(0);
		char* buffer = new char[fsize];
		t_file.read(buffer, fsize);
		t_file.close();
		//temp_files.seekg(0, ios::end);
		temp_files.write(buffer, fsize);
		delete[] buffer;
		mod_file.seekp(0x1B8 + 0x60 * (uint64_t(famount) - 1));
		mod_file.put('\0');//offsets are written at the end
		mod_file.put('\0');
		mod_file.put('\0');
		mod_file.put('\0');
		mod_file.write((char*)&fsize, sizeof(fsize));
		}
	/*temp_files.close();
	temp_files.open(mod_file_path + ".iamtemp", ios::in | ios::binary);
	if (!temp_files)
	{
		cerr << "[Error] Can't create temp file. Try running the tool as administrator.\n";
		system("pause");
		exit(1);
	}*/
	//temp_files.seekg(0, ios::end);
	uint32_t header_size = temp_files.tellg();//Actually, it's all files size at once, I'm just too lazy to rewrite this
	temp_files.seekg(0);
	char* buffer = new char[header_size];
	temp_files.read(buffer, header_size);
	mod_file.seekg(0x160 + 0x60 * uint64_t(famount));
	mod_file.write(buffer, header_size);
	delete[] buffer;
	temp_files.close();
	std::cout << "Deleting temp file.\n";
	remove((mod_file_path + ".iamtemp").c_str());

	//fix offsets
	std::cout << "Writing offsets.\n";
	/*mod_file.close();
	mod_file.open(mod_file_path, ios::in | ios::out | ios::binary);
	if (!mod_file)
	{
		cerr << "[Error] Can't open mod file. Try running the tool as administrator.\n";
		system("pause");
		exit(1);
	}*/

	mod_file.seekp(0xC);
	mod_file.write((char*)&famount, sizeof(famount));

	uint64_t size_offset = 0;
	uint32_t foffset;
	for (uint64_t i = 0; i < famount; i++) {
		foffset = 0x160 + (famount * 0x60) + size_offset;
		mod_file.seekp(0x1B8 + (0x60 * i));
		mod_file.write((char*)&foffset, sizeof(foffset));
		mod_file.seekg(0x1BC + (0x60 * i));
		uint32_t fsize;
		mod_file.read(reinterpret_cast<char*>(&fsize), sizeof(fsize));
		size_offset += fsize;
	}

	//Make mess
	std::cout << "Making mess.\n";
	for (uint64_t i = 0; i < famount; i++) {
		c = 0xFF;
		end = -1;
		for (uint64_t j = 0; j < 0x58; j++) {
			mod_file.seekg(0x160 + 0x60 * i + j);
			c = mod_file.get();
			if (uint8_t(c) == 0x00) {
				if (end == -1) {
					end = j;
					continue;
				}
				else if (j > end) {
					char q = rand() % 255 + 1;
					mod_file.seekp(0x160 + 0x60 * i + j);
					mod_file.put(q);
				}
			}
		}
	}
	//Calculate CRC
	mod_file.seekg(0, std::ios::end);
	uint32_t fsize = mod_file.tellg();
	mod_file.seekg(0);
	buffer = new char[fsize];
	mod_file.read(buffer, fsize);
	uint32_t CRC = CRC32(buffer, fsize);
	CRC += 0x93E56B02;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	delete[] buffer;
	mod_file.seekp(0, std::ios::end);
	mod_file.write((char*)&CRC, sizeof(CRC));

	//Hardcoded mess
	mod_file.seekg(0, std::ios::end);
	fsize = mod_file.tellg();
	mod_file.seekg(0);
	buffer = new char[fsize];
	mod_file.read(buffer, fsize);
	protection(buffer, fsize, 'e');
	mod_file.seekp(0);
	mod_file.write(buffer, fsize);
	delete[] buffer;
	mod_file.close();
	}

