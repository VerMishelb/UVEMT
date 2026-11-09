/*
Copyright 2020-2026 VerMishelb <vermishelb535@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

InterAction Mod Tool (Chicken Invaders Mod Tool, CIU string decoder encoder), 07.03.2020

A tool initially designed for getting readable strings from WAD files of CI4 which
eventually spiralled out into an asset editor of Ultra VGA Engine games.

Please, bear in mind that this has been started 07.03.2020.
This code is an amalgamation of every mistake I did between 2020 and 2026.
Use this as a reference only. Treat every line as if it tried to kill you in the past.
Proceed with caution.
	-- mish, 09.11.2026

-e encode
-d decode
-r repack
-s show header
-ex extract

CHANGELOG
todo
x Default hq2x as an option [utf issues]
DONE Automatic format detection where possible
? Hide iamtemp
x Memory optimisation from IAMI (in case I'm not dying in the process) [I am]
? In case the files are identical, don't create another copy of it but redirect another entry to the same zone instead.
? Maybe try to speed it up one last time
? Save CRC instead of name to make CIU.iami unnecessary (but the file name won't be shown if an error occurs)
? uve32 support
x (Most of) ttf files are now allowed to be repacked in audio restricted version. [Pain to detect]

v3.6.5.?
* Fixed path issues when extracting files from WAD and mods.
* The entire code was refactored (no, not for GUI, for my own sanity as for now).
* Added "I CAN READ" captcha.
* Half of previously critical errors no longer force quitting the application.
* Some of the messages were reworked to be more clear (and, in some cases, more family friendly).

v3.6.6
+ Table update mode is now capable of creating the file if it doesn't exist (forgot to add to the changelog at some point).
+ "make_tableless_mods" config option.
+ Now prints out config values after reading the config, or defaults if it wasn't found.
+H Added a config option to keep the temp file.
+H Added a config option to not do the "mess" step on mods. Makes the created mod useless, but also lets me debug IAMT.
+H Added an experiment for adding files instead of skipping them if they are missing (CI1 only, "r" mode only, EXPERIMENT_use_add_files config option)

!!! don't forget to port everything above to the rewrite

v3.6.5.1
- Fixed command line arguments not even working (KnockoffFeline :medal_sports: Bug?)

v3.6.5
- Added .???x to OpenFileDialog filters.
- CRC no longer needs 0x in the beginning when you repack CIU by CRC instead of name.
- Added version resource file.
- Completely reworked table update function. Now it's a lot faster and doesn't require you to crash the game for every single file.
- Fixed some of the pause calls. Let me know if some of these get skipped (asks for input but continues anyway).
- Table updating is no longer locked behind the full version. Good luck figuring out the usage though.
- Changed the file size reading in extract and info modes to make it work with android versions (Android repacking is not working as for now).

3.6.4:
	* Removed skip file confirmation when extracting CIU files to avoid confirmation of no longer used files.
3.6.3.1: FOURCC_CHECK(x) instead of copy-paste
*/
#define NOMINMAX

// ONLY ONE OF THE FOLLOWING
//#define IAMT_AUDIO_ONLY
//#define IAMT_NO_CIU

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#if defined(IAMT_AUDIO_ONLY)
#define IAMT_VERSION "v3.6.6 [AUDIO RESTRICTED BUILD]"
#elif defined(IAMT_NO_CIU)
#define IAMT_VERSION "v3.6.6nociu_dev3"
#else
#define IAMT_VERSION "v3.6.6"
#endif

// This is for format check when you mod non-audio files in IAMTa -- mish, 09.11.2026
#define FOURCC_CHECK(x) ((((x) & 0x00FFFFFF) != 'ggO') && (((x) & 0xFFFFFFFF) != 'FFIR') && (((x) & 0x00FFFFFF) != '3DI'))

#include <iostream>
//#include <ctime>
#include <string>
#include <fstream>
#include <iomanip>//Makes -s beautiful
#include <sstream>//cout as a variable
#include <vector>
#include <direct.h>//Open dialog
#include <Windows.h>//Open dialog
//#include <shellapi.h>//Open dialog ._ .
#include <filesystem>//Paths and stuff
#include <ShlObj.h>
//#include <limits>//to get rid of system() and use Enter instead
#include <locale>
#include <codecvt>

#include "FileOperations.h"
#include "Functions.h"
#include "asciiDickClark.h"

using namespace std;

struct TableEntry {
	std::string name;
	uint32_t hash = 0;
};

std::string credits = "\nInterAction Mod Tool " IAMT_VERSION " (" __DATE__ ")\n\n"
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

bool g_FirstLaunch = true;
bool g_KeepTempFiles = false;
bool g_DoNotEncryptMods = false;
bool g_UseAddFiles = false;

bool MagicIsAudio(std::ifstream file) {
	std::streampos old_pos = file.tellg();
	file.seekg(0);
	uint32_t block = 0;
	file.read(reinterpret_cast<char*>(&block), 4);

	/*
	* Ogg, RIFF (wav), ID3 (mp3)
	*/
	return (((block & 0x00FFFFFF) != 'ggO') && ((block & 0xFFFFFFFF) != 'FFIR') && ((block & 0x00FFFFFF) != '3DI'));
}

bool MagicTtf() { // cmap block
	return 1;
}


int main(int argc, char* argv[])
{
	//setlocale(LC_ALL, ".UTF8");
	cout << "VerMishelb's InterAction Mod Tool " IAMT_VERSION " (" __DATE__ ")\nIf you found any bugs or crashes, please let me know in Discord (VerMishelb#6318)\nPlease note that InterAction studios IS NOT related to this tool development and won't help you if you corrupted your game files while doing mods.\n"
		"Also, if you want to share your mod with others, DO NOT share your modded game files (.dat, .dat.hq2x, .dat.222x etc). For this kind of stuff mod creation function exists. If it crashes for you \"because no\", contact us and we'll try to fix it.\n"
#if !defined(IAMT_AUDIO_ONLY) && !defined(IAMT_NO_CIU)
		"================\n"
		"!!! ATTENTION !!!\n"
		"SINCE UPDATE v2.0 TOOL CAN ONLY BE OBTAINED BY SEVERAL PEOPLE WHO MAKE MODS FOR CI OR EXTRACT FILES FOR WIKI.\n"
		"IF YOU DIDN'T OBTAIN THIS TOOL FROM Emerald, VerMishelb, Francis OR One-Winged Lunarian, LET US KNOW BY DISCORD IMMEDIATELY.\n"
		"================\n"
#endif
		;
	string str;
	string res;
	string tool_folder = string(argv[0]).substr(0, string(argv[0]).rfind('\\') + 1);
	string command = '\"' + string(argv[0]) + '\"';
	char mode = '0'; //0=Menu, d/e=(de/en)code, r=repack, s=show, x=extract, t=SHOW table, T=EDIT table, u=table update, i=turn .iamt_table to .iami, m=mod creation, M=Edit mod file
	char hq2x_type = '?'; // ? 1 2(2old-3old) 3(2RM-3HD-4-5-CIU) 4(WAD4) M(mod file)
	bool mode_selected = false;
	bool wait_for_files = false;
	fstream ifile;
	fstream ofile;

	//Read config file
	bool full_backup = 1; //default values
	bool last_backup = 0;
	bool gMakeTablelessMods = 1;
	int loading_timeout = 10;
	ifstream config(tool_folder + "config.cfg");
	
	std::string config_lines[7] = {
		"full_backup",
		"last_backup",
		"loading_timeout",
		"make_tableless_mods",
		"keep_temp_files",
		"do_not_encrypt_mods",
		"EXPERIMENT_use_add_files"
	}; // lines in config to search
	if (!config) {
		cerr << "[Warning] Can't find config.cfg in tool folder! Get one from tool archive.\nTool will use default values.\n";
	}
	else {
		cout << "Reading config.cfg\n";
		while (getline(config, str)) {
			//if (str.empty()) { continue; }

			// Not in the same line with str.empty() because it will try to get impossible symbol in case of empty string, which results in crash.
			if (str.empty() || str.at(0) == '#' || str.at(0) == ';') { continue; }
			else if (str.find(config_lines[0]) != string::npos) {
				str = str.substr(str.find('=', str.find(config_lines[0])) + 1); // 0, 1
				full_backup = stoi(str);
				
				continue;
			}
			else if (str.find(config_lines[1]) != string::npos) {
				str = str.substr(str.find('=', str.find(config_lines[1])) + 1); // 0, 1
				last_backup = stoi(str);
				
				continue;
			}
			else if (str.find(config_lines[2]) != string::npos) {
				str = str.substr(str.find('=', str.find(config_lines[2])) + 1); // int
				loading_timeout = stoi(str);
				if (loading_timeout < 1) { loading_timeout = 15; }
				
				continue;
			}
			else if (str.find(config_lines[3]) != std::string::npos) {
				str = str.substr(str.find('=', str.find(config_lines[3])) + 1); // 0, 1
				gMakeTablelessMods = stoi(str);
				
				continue;
			}
			else if (str.find(config_lines[4]) != std::string::npos) {
				str = str.substr(str.find('=', str.find(config_lines[4])) + 1); // 0, 1
				g_KeepTempFiles = stoi(str);

				continue;
			}
			else if (str.find(config_lines[5]) != std::string::npos) {
				str = str.substr(str.find('=', str.find(config_lines[5])) + 1); // 0, 1
				g_DoNotEncryptMods = stoi(str);

				continue;
			}
			else if (str.find(config_lines[6]) != std::string::npos) {
				str = str.substr(str.find('=', str.find(config_lines[6])) + 1); // 0, 1
				g_UseAddFiles = stoi(str);

				continue;
			}
		}
	}
	std::cout << config_lines[0] << "=" << full_backup << '\n';
	std::cout << config_lines[1] << "=" << last_backup << '\n';
	std::cout << config_lines[2] << "=" << loading_timeout << '\n';
	std::cout << config_lines[3] << "=" << gMakeTablelessMods << '\n';
	std::cout << config_lines[6] << "=" << g_UseAddFiles << '\n';
	std::cout << std::endl;

	//printf_s("Argument list:\n");
	//for (int i = 0; i < argc; ++i) {
	//	printf_s("argv[%i]=%s\n", i, argv[i]);
	//}

	//mode setup
	if (argc > 1) {
		if (strcmp(argv[1], "-d") == 0) { mode = 'd'; }
		if (strcmp(argv[1], "-e") == 0) { mode = 'e'; }
		if (strcmp(argv[1], "-r") == 0 && argc > 4) { mode = 'r'; }
		if (strcmp(argv[1], "-s") == 0 && argc > 2) { mode = 's'; }
		if (strcmp(argv[1], "-ex") == 0) {
			mode = 'x';
			if (argc > 3) { wait_for_files = false; }
			else { wait_for_files = true; }
		}
#ifndef IAMT_AUDIO_ONLY
		if (strcmp(argv[1], "-u") == 0) { mode = 'u'; }
		if (strcmp(argv[1], "-ts") == 0) { mode = 't'; }
		if (strcmp(argv[1], "-te") == 0) { mode = 'T'; }
		if (strcmp(argv[1], "-ti") == 0) { mode = 'i'; }
		if (strcmp(argv[1], "-me") == 0) { mode = 'M'; }
#endif
		if (strcmp(argv[1], "-mc") == 0) {
			mode = 'm';
			if (argc > 3)
				wait_for_files = false;
			else
				wait_for_files = true;
		}
	}

MODE_SELECTION:
	if (mode != '0' && !g_FirstLaunch) {
		mode = '0';
		mode_selected = false;
		wait_for_files = true;
	}
	g_FirstLaunch = false;
	//Mode selection screen
	if (mode == '0') {
		string inpt;
		while (mode_selected == false) {
			cout << "\rPlease, select mode you want to work with (you can also use letters instead, like \"r\" to enter repack mode):\n"
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
			getline(cin, inpt);
			if (inpt == "0" || inpt == "ciao" || inpt == "exit") { exit(0); }
			else if (inpt == "1" || inpt == "decode" || inpt == "d") {
				mode = 'd';
				mode_selected = true;
				break;
			}
			else if (inpt == "2" || inpt == "encode" || inpt == "e") {
				mode = 'e';
				mode_selected = true;
				break;
			}
			else if (inpt == "3" || inpt == "show" || inpt == "s") {
				mode = 's';
				if (argc <= 2) { wait_for_files = true; }
				mode_selected = true;
				break;
			}
			else if (inpt == "4" || inpt == "repack" || inpt == "r") {
				mode = 'r';
				wait_for_files = true;
				mode_selected = true;
				break;
			}
			else if (inpt == "5" || inpt == "extract" || inpt == "ex") {
				mode = 'x';
				wait_for_files = true;
				mode_selected = true;
				break;
			}
			else if (inpt == "6" || inpt == "update" || inpt == "u") {
				mode = 'u';
				mode_selected = true;
				break;
			}
			else if (inpt == "9" || inpt == "tiami" || inpt == "ti") {
				mode = 'i';
				mode_selected = true;
				break;
			}
#ifndef IAMT_AUDIO_ONLY
			else if (inpt == "7" || inpt == "tshow" || inpt == "ts") {
				mode = 't';
				mode_selected = true;
				break;
			}
			else if (inpt == "8" || inpt == "tedit" || inpt == "te") {
				mode = 'T';
				wait_for_files = true;
				mode_selected = true;
				break;
			}
			else if (inpt == "11" || inpt == "medit" || inpt == "me") {
				mode = 'M';
				wait_for_files = true;
				mode_selected = true;
				break;
			}
#endif
			else if (inpt == "10" || inpt == "mod" || inpt == "mc") {
				mode = 'm';
				wait_for_files = true;
				mode_selected = true;
				break;
			}
			else if (inpt == "C" || inpt == "c" || inpt == "credits" || inpt == "Credits") {
				printf("%s\n", credits.c_str());
				BetterCallPause();
			}
			else if (inpt == "dickclark") {
				system("cls");
				std::cout << "wat he returneds eg ain ?|?|?!?!??!evelen!?\n\n\n";
				std::cout << asciiDickClark << '\n';
			}
		}
	}
	//DECODE MODE
	if (mode == 'd') {
		string temp_str;
		cout << 
			"Decode mode.\n"
			"Enter the byte array you would like to read (use RMB to paste value).\n>";
		getline(cin, temp_str);
		cout << "\nErasing spaces...\n";
		for (uint32_t i = 0; i < size(temp_str); i++) {
			if (temp_str[i] != ' ') {
				str += temp_str[i];
			}
		}
		temp_str = str;
		temp_str.clear();
		res.clear();
		uint8_t text2int;
		for (uint32_t i = 0; i < size(str); i += 2) {
			text2int = strtoul((str.substr(i, 2)).c_str(), NULL, 16);
			res += text2int;
		}
		cout << "Decoding the string...\n";
		res = str_xor(res);
		cout << "Result: " << res << '\n';
		for (uint32_t i = 0; i < res.length(); i++) {
			cout << hex << +static_cast<uint8_t>(res[i]) << " ";
		}
		cout << '\n';
	}
	//ENCODE MODE
	else if (mode == 'e') {
		cout << "Encode mode.\nEnter the string you want to encode.\n>";
		getline(cin, str);
		cout << "\nEncoding the string...\n";
		res = str_xor(str);
		cout << "Result:\n" << res << "\n";
		for (uint32_t i = 0; i < res.length(); i++)
			cout << hex << +static_cast<uint8_t>(res[i]) << " ";
		cout << '\n';
	}
	//REPACK MODE
	else if (mode == 'r') {
		cout << "Repack mode.\n";//argv[2] - hq2x, argv[3, 5...] - path, argv[4, 6...] - file
		command += " -r ";

		vector<string> files;
		uint32_t h_file_offset;
		uint32_t temp_off;
		uint32_t files_amount;
		uint32_t file_pointer;
		uint32_t file_size  = 0;
		uint32_t real_file_size = 0;
		uint32_t new_file_size = 0;
		uint32_t edit_goto;
		uint64_t full_size;
		uint64_t erase_point;//uint64_t
		string hq2x;
		unsigned char c;
		int difference;
		bool found;

		//Read files and paths and put them into the vector
		if (wait_for_files == false)
		{
			for (int i = 3; i < argc; i++)
				files.push_back(string(argv[i]));//0, 2 - path; 1, 3 - file, then repeats...?
			hq2x = argv[2];
		}
		else//If there are no files given
		{
			int counter = 1;
			string inpt;
			if (argc < 3)
			{
				cout << "Enter the path to \"dat.hq2x\" file:\n";
				hq2x = OpenFileWindow("InterAction WAD Format (.dat; .hq2x; .222x)\0*.DAT;*.HQ2X;*.222X\0All Files\0*.*\0");
				/*
				Old code (in case this won't work for someone and I have an option to revert changes)
				getline(cin, hq2x);
				while (hq2x.find('\"') != string::npos)
				{
					hq2x.erase(hq2x.find('\"'), 1);
				}
				*/
			}
			else
			{
				hq2x = argv[2];
			}
			cout << "\nEnter the file paths you want to work with. Put an empty line when you finish.\n(Click RMB to paste path from buffer)\n\n"
				"\"undo\" - re-enter last pair.\n\"edit X\" - re-enter Xth pair.\n\"txt\" - use text file instead (will clear the ones you wrote before)\n\"folder\" - use folder as the root of paths\n";
			for (;;)
			{
				cout << counter << ". Path in hq2x: ";
				getline(cin, inpt);
				while (inpt.find('\"') != string::npos)
				{
					inpt.erase(inpt.find('\"'), 1);//delete " symbols
				}
				if (inpt == "")
				{
					cout << "\n\nTyping finished.\n";
					break;
				}
				else if (inpt == "undo")
				{
					if (files.size() >= 2)
					{
						files.pop_back();
						files.pop_back();
						counter--;
						continue;
					}
					else {
						cout << "You've reached the beginning!\n";
						continue;
					}
				}
				else if (inpt.find("edit") == 0 && inpt.size() > 4 && inpt[4] == ' ')//edit NUM. In case of "edit 9"... This shit better work on the first try (UPD: Ok it doesn't work) (UPD2: Fixed a bug when 4. edit 4 caused crash)
				{
					try
					{
						edit_goto = stoi(inpt.substr(4));//...this thing will be = 9
					}
					catch (const std::invalid_argument&)
					{
						cout << "Can't find proper number in the line!\n";
						continue;
					}
					//go edit the given pair of files
					if (edit_goto >= 1 && edit_goto < counter)
					{
						cout << edit_goto << ". Path in hq2x: ";
						getline(cin, inpt);
						while (inpt.find('\"') != string::npos)
						{
							inpt.erase(inpt.find('\"'), 1);//delete " symbols
						}
						edit_goto = (edit_goto - 1) * 2;//1 = [0, 1], 2 = [2, 3]
						files[edit_goto] = inpt;
						cout << " | Replace with: ";
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
					else
					{
						cout << "Input value is incorrect.\n";
						continue;
					}
				}
				else if (inpt == "txt" || inpt == "TXT")
				{
					inpt = OpenFileWindow("Text File (*.txt)\0*.TXT\0All Files\0*.*\0");
					ifstream txtf(inpt);
					if (!txtf)
					{
						cout << "[Error] Can't open the file!\n";
						txtf.clear();
						continue;
					}
					files.clear();
					files = txt2vec(txtf);
					cout << "Done.\n";
					continue;
				}
				else if (inpt == "folder" || inpt == "FOLDER") {
					inpt = BrowseFolder("C:\\");
					if (!inpt.empty()) {
						files.clear();
						files = get_paths(inpt);
						cout << "Done.\n";
					}
					continue;
				}
				files.push_back(inpt);
				cout << " | Replace with: ";
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
			while (files[i].find('\\') != string::npos)
			{
				files[i].at(files[i].find('\\')) = '/';
			}
		}
		command += '\"' + hq2x + "\" ";
		//Display final result
		cout << "Changes to be made:\nDAT file: \"" << hq2x << "\"\n";
		for (uint32_t i = 0; i < files.size(); i += 2)
		{
			command += '\"' + files[i] + "\" \"" + files[i + 1] + "\" ";
			cout << "@ " << files[i] << " -> " << files[i + 1] << '\n';
		}
		cout << "\nCMD command:\n" << command << "\n\n";
		for (;;)
		{
			string inpt;
			cout << "Do you want to start the replacement? (y/n): ";
			getline(cin, inpt);
			if (inpt == "y")
				break;
			if (inpt == "n")
			{
				cout << "\nReplacement canceled.\n";
				cout << "Press Enter to continue...";
				cin.get();
				exit(0);
			}
			else
			{
				cout << "\r";
			}
		}

		//Open hq2x in reading mode
		cout << "\nOpening the \"" << hq2x << "\".";
		ifile.open(hq2x, ios::in | ios::binary);
		if (!ifile)
		{
			cerr << "\n[Error] Can't open \"" << hq2x << "\".\n"
				"If you're trying to open the file from Program Files folder, run the tool from administrator.\n"
				"If the path contains non-latin symbols, try using CMD command instead:\n"
				<< command << '\n';
			cout << "Press Enter to continue...";
			cin.get();
			exit(1);
		}
		cout << " Done!\n";

		//Which hq2x type
		ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		//CIU v35+ WAD4 "UVE " line
#ifndef IAMT_NO_CIU
		if (files_amount == 0x20455655) {
			hq2x_type = '4';
		}
#endif
		//old CI4-all HD versions + old CIU
		if (files_amount >= 0 && files_amount <= 10) {
			hq2x_type = '3';
		}
		else
		{
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
			if (files_amount == 0x2F786667 || files_amount == 0x2F786673 || files_amount == 0x6973756D || files_amount == 0x74737973)// "gfx/" "sfx/" "musi" "syst"
				hq2x_type = '1';
			else
				hq2x_type = '2';
		}

		files_amount = 0;
		//Read files amount
		if (hq2x_type == '3')
		{
			ifile.seekg(0x4);
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		}
#ifndef IAMT_NO_CIU
		else if (hq2x_type == '4')
		{
			ifile.seekg(0xC);
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		}
#endif
		else
		{
			ifile.seekg(0);
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		}

		if (full_backup == true)
		{
			cout << "Creating a backup.\n";
			ifile.seekg(0, ios::end);
			erase_point = ifile.tellg();
			char* buffer = new char[erase_point];
			ifile.seekg(0);
			ifile.read(buffer, erase_point);
			ifile.close();

			ofile.open(hq2x + ".backup", ios::out | ios::binary);
			if (!ofile)
			{
				cerr << "[Error] Can't create backup file.\n"
					"Try running the tool from administrator.\n"
					"If the path contains non-latin symbols, try using CMD command instead:\n"
					<< command << '\n';
				cout << "Press Enter to continue...";
				cin.get();
				exit(1);
			}
			ofile.write(buffer, erase_point);
			ofile.close();
			delete[] buffer;
			cout << "Backup successfully created.\n\n";
		}
		else
		{
			cout << "Full backup is turned off by config.\n\n";
			ifile.close();
		}
		//Get header size
		//header_size = 0x5c * files_amount + 0x8;

		//queue = number of pair of files (file+0, path+1)
		if (hq2x_type == '3')
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
				//Encode file name. res = encoded name
				res.clear();
				res = str_xor(str);
				//Search in the header
				//str = "";
				uint32_t f_num = 0;//number of the file which is needed in the header
				while (f_num < files_amount)
				{
					h_file_offset = 0x8 + 0x5c * f_num;//file in the header offset
					ifile.seekg(h_file_offset);
					//Read the name from file
					str.clear();
					c = 0xFF;
					temp_off = 0;
					while (uint8_t(c) != 0xCC)
					{
						ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
						temp_off++;
						c = ifile.get();
						if (uint8_t(c) != 0xcc)
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
					ifile.read(reinterpret_cast<char*>(&real_file_size), sizeof(real_file_size));
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
				char* old_file = new char[real_file_size];//to test if they are the same
				//char* part_b;//the rest of the file
				//h_file_offset is still a beginning of the block related to our file
				ifile.seekg(0, ios::end);
				full_size = ifile.tellg();//full hq2x size
				ifile.seekg(uint64_t(file_pointer));
				ifile.read(old_file, uint64_t(real_file_size));//Old file to check if they are the same
				ifile.seekg(uint64_t(file_pointer) + uint64_t(real_file_size));//go to the end of the replaceable file
				char* part_b = new char[full_size - uint64_t(file_pointer) - uint64_t(real_file_size)];
				ifile.read(part_b, full_size - uint64_t(file_pointer) - uint64_t(real_file_size));//full_size - uint64_t(file_pointer) - uint64_t(file_size)+1
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
				ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x54));//write new file size
				ifile.write((char*)&new_file_size, sizeof(new_file_size));
				ifile.write((char*)&new_file_size, sizeof(new_file_size));
				cout << " Done!\nRewriting the rest of the file.";
				ifile.seekg(file_pointer);
				ifile.write(part_a, new_file_size);
				ifile.write(part_b, full_size - uint64_t(file_pointer) - uint64_t(real_file_size));
				erase_point = ifile.tellg();
				difference = new_file_size - real_file_size;
				cout << " Done!\nFile difference is " << dec << difference << " bytes. Rewriting the header.";
				for (uint32_t i = f_num + 1; i < files_amount; i++)
				{
					ifile.seekg(uint64_t(0x5c) * i + uint64_t(0x58));
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
		if (hq2x_type == '2')
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
				res = str_xor(str);
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
					while (uint8_t(c) != 0xCC)
					{
						ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
						temp_off++;
						c = ifile.get();
						if (uint8_t(c) != 0xcc)
							str += c;
					}
					if (res == str)
					{
						found = true;
						break;
					}
					f_num++;
				}

				bool adding_file = false;
				if (found == true)
				{
					ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x50));
					ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));//poiner value
					ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));//size value
				}
				else if (g_UseAddFiles) {
					std::cout << "File \"" << files[queue] << "\" does not exist. Adding as a new file.\n";
					adding_file = true;
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
				//char* part_b;//the rest of the file
				//h_file_offset is still a beginning of the block related to our file
				ifile.seekg(0, ios::end);
				full_size = ifile.tellg();//full hq2x size
				char* old_file = nullptr;
				char* part_b = nullptr;
				if (found) {
					old_file = new char[file_size];//to test if they are the same
					ifile.seekg(uint64_t(file_pointer));
					ifile.read(old_file, uint64_t(file_size));//Old file to check if they are the same
					ifile.seekg(uint64_t(file_pointer) + uint64_t(file_size));//go to the end of the replaceable file
					part_b = new char[full_size - uint64_t(file_pointer) - uint64_t(file_size)];
					ifile.read(part_b, full_size - uint64_t(file_pointer) - uint64_t(file_size));//full_size - uint64_t(file_pointer) - uint64_t(file_size)+1
				}
				else {
					part_b = new char[full_size - (0x4 + 0x58 * files_amount)];
				}
				ifile.clear();
				ifile.close();

				cout << "Opening \"" << files[queue + 1] << "\".";
				ifile.open(files[queue + 1], ios::in | ios::binary);//open file to be replaced with
				if (!ifile)
				{
					cerr << "\n[Error] Can't open \"" << files[queue + 1] << "\".\n"
						"Try running the tool from administrator.\n"
						"If the path contains non-latin symbols, try using CMD command instead:\n" << command << "\n\n";
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
					if (old_file) delete[] old_file;
					if (part_b) delete[] part_b;
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
				if (!adding_file && file_size == new_file_size)
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
				if (adding_file) {	
					ifile.seekg(0x4 + 0x58 * files_amount);
					ifile.read(part_b, full_size - (0x4 + 0x58 * files_amount));
					ifile.seekp(0);
					h_file_offset = 0x4 + 0x58 * files_amount; // 0x50 name, 0x4 ptr, 0x4 size
					files_amount++;
					ifile.write(reinterpret_cast<char*>(&files_amount), 4);
					for (uint32_t i = 0; i < files_amount-1; i++)
					{
						ifile.seekg(uint64_t(0x58) * i + uint64_t(0x54));
						ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));
						file_pointer += 0x58;
						ifile.seekg(-4, ios::cur);
						ifile.write((char*)&file_pointer, 4);
					}
					ifile.seekp(h_file_offset);
					if (files[queue].size() > 0x50 - 1) { files[queue].resize(0x50 - 1); }
					ifile.write(str_xor(files[queue]).c_str(), files[queue].size());
					for (auto i = 0x50 - files[queue].size(); i > 0; i--) {
						ifile.put(0xCC);
					}

					auto new_file_ptr = full_size + 0x58;
					ifile.write(reinterpret_cast<char*>(&new_file_ptr), 4);
					ifile.write(reinterpret_cast<char*>(&new_file_size), 4);
					ifile.write(part_b, full_size - (0x4 + 0x58 * (files_amount - 1)));
					ifile.write(part_a, new_file_size);
					delete[] part_a;
					delete[] part_b;
					ifile.close();
				}
				else {
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
				}

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

				bool adding_file = false;
				if (found) {
					ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x50));
					ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));//poiner value
					ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));//size value
				}
				else if (g_UseAddFiles) {
					std::cout << "File \"" << files[queue] << "\" does not exist. Adding as a new file.\n";
					adding_file = true;
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
				//char* part_b;//the rest of the file
				//h_file_offset is still a beginning of the block related to our file
				ifile.seekg(0, ios::end);
				full_size = ifile.tellg();//full hq2x size
				char* old_file = nullptr;
				char* part_b = nullptr;
				if (found) {
					old_file = new char[file_size];//to test if they are the same
					ifile.seekg(uint64_t(file_pointer));
					ifile.read(old_file, uint64_t(file_size));//Old file to check if they are the same
					ifile.seekg(uint64_t(file_pointer) + uint64_t(file_size));//go to the end of the replaceable file
					part_b = new char[full_size - uint64_t(file_pointer) - uint64_t(file_size)];
					ifile.read(part_b, full_size - uint64_t(file_pointer) - uint64_t(file_size));//full_size - uint64_t(file_pointer) - uint64_t(file_size)+1
				}
				else {
					part_b = new char[full_size - (0x4 + 0x58 * files_amount)];
				}
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
					if (old_file) delete[] old_file;
					if (part_b) delete[] part_b;
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
				if (!adding_file && file_size == new_file_size)
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
				if (adding_file) {
					ifile.seekg(0x4 + 0x58 * files_amount);
					ifile.read(part_b, full_size - (0x4 + 0x58 * files_amount));
					ifile.seekp(0);
					h_file_offset = 0x4 + 0x58 * files_amount; // 0x50 name, 0x4 ptr, 0x4 size
					files_amount++;
					ifile.write(reinterpret_cast<char*>(&files_amount), 4);
					for (uint32_t i = 0; i < files_amount-1; i++)
					{
						ifile.seekg(uint64_t(0x58) * i + uint64_t(0x54));
						ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));
						file_pointer += 0x58;
						ifile.seekp(-4, ios::cur);
						ifile.write((char*)&file_pointer, 4);
					}
					ifile.seekp(h_file_offset);
					if (files[queue].size() > 0x50 - 1) { files[queue].resize(0x50 - 1); }
					ifile.write(files[queue].c_str(), files[queue].size());
					for (auto i = 0x50 - files[queue].size(); i > 0; i--) {
						ifile.put(0);
					}
					auto new_file_ptr = full_size + 0x58;
					ifile.write(reinterpret_cast<char*>(&new_file_ptr), 4);
					ifile.write(reinterpret_cast<char*>(&new_file_size), 4);
					ifile.write(part_b, full_size - (0x4 + 0x58 * (files_amount-1)));
					ifile.write(part_a, new_file_size);
					delete[] part_a;
					delete[] part_b;
					ifile.close();
				}
				else {
					//write new file size
					ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x54));
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
				}
				cout << "File replacement is finished.\n\n";
			}
		}
#ifndef IAMT_NO_CIU
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
#endif
		cout << "\nAll the files have been replaced :)\a\n";
	}
	else if (mode == 's')//HEADER MODE
	{
		cout << "Header info mode.\n";

		string hq2x;
		command += " -s ";

		if (wait_for_files == false)
		{
			hq2x = argv[2];
		}
		else//If there are no files given
		{
			hq2x = OpenFileWindow("InterAction WAD Format (.dat; .hq2x; .222x)\0*.HQ2X;*.DAT;*.222X\0All Files\0*.*\0");
		}
		command += '\"' + hq2x + '\"';
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
		cout << "Opened \"" << hq2x << "\".\n\n";

		uint32_t files_amount;
		uint32_t h_file_offset;//Offset of the file in header
		uint32_t file_pointer;//Offset of the file itself
		uint32_t file_size;
		uint32_t header_size;
		uint32_t files_counter = 1;
		uint32_t temp_off;//Used for reading names
		int size_multiplier;
		int size_str_i;
		unsigned char c;
		// This is the only practical change since the latest release -- mish, 09.11.2026
		string size_str[3] = { "B","KiB","MiB" };//Bytes, Kibibytes, Mebibytes
		std::ostringstream size_display;
		std::ostringstream header_offset_display;
		std::ostringstream file_offset_display;

		//Which hq2x type
		ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		if (files_amount == 0x20455655)//CIU v35 WAD4 "UVE "
			hq2x_type = '4';
		else if (files_amount >= 0 && files_amount <= 10)//old CI4-all HD versions + old CIU
			hq2x_type = '3';
		else
		{
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));//Reads 5-8 bytes, NOT 1-4!!!
			if (files_amount == 0x2F786667 || files_amount == 0x2F786673 || files_amount == 0x6973756D || files_amount == 0x74737973)// "gfx/" "sfx/" "musi" "syst"
				hq2x_type = '1';
			else
				hq2x_type = '2';
		}

		//Read files amount
		if (hq2x_type == '3')
		{
			ifile.seekg(0x4);
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		}
		else if (hq2x_type == '4')
		{
			ifile.seekg(0xC);
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		}
		else
		{
			ifile.seekg(0);
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		}

		//Header size
		if (hq2x_type == '4')
			header_size = 0x10 * (files_amount + 1);
		else if (hq2x_type == '3')
			header_size = 0x5c * files_amount + 0x8;
		else
			header_size = 0x58 * files_amount + 0x4;

		if (hq2x_type == '3')
		{
			cout << "Files amount: " << files_amount << "\nHeader size: 0x" << hex << header_size
				<< "\n[Header OFFS] [File OFFS] (size) path\n";
			for (uint32_t i = 0; i < files_amount; i++)
			{
				h_file_offset = 0x8 + 0x5c * i;
				ifile.seekg(h_file_offset);

				//Name
				str.clear();
				c = 0xFF;
				temp_off = 0;
				while (uint8_t(c) != 0xCC)
				{
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
				ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));

				//File size
				ifile.seekg(uint64_t(h_file_offset) + 0x58);
				ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));
				size_multiplier = 1;
				size_str_i = 0;
				while (file_size > uint32_t(1024 * size_multiplier) && size_str_i < 2)
				{
					size_multiplier *= 1024;
					size_str_i += 1;
				}
				float file_size_f = float(file_size) / float(size_multiplier);//File size to display

				size_display << "(" << round(file_size_f * 10) / 10 << size_str[size_str_i] << ")";
				header_offset_display << "[0x" << hex << h_file_offset << "]";
				file_offset_display << "[0x" << hex << file_pointer << "]";
#ifdef IAMT_AUDIO_ONLY
				if (str.find(".tga") == std::string::npos &&
					str.find(".preload") == std::string::npos &&
					str.find(".jpg") == std::string::npos &&
					str.find(".hmap") == std::string::npos &&
					str.find(".jpeg") == std::string::npos &&
					str.find(".txt") == std::string::npos)
#endif
					cout << setw(5) << left << std::dec << files_counter++ << setw(9) << left << header_offset_display.str() << setw(12) << left << file_offset_display.str() << setw(13) << left << size_display.str() << str << '\n';
				size_display.str("");
				header_offset_display.str("");
				file_offset_display.str("");
				size_display.clear();
				header_offset_display.clear();
				file_offset_display.clear();
			}
		}
		else if (hq2x_type == '2')
		{
			cout << "Files amount: " << files_amount << "\nHeader size: 0x" << hex << header_size
				<< "\n[Header OFFS] [File OFFS] (size) path\n";
			for (uint32_t i = 0; i < files_amount; i++)
			{
				h_file_offset = 0x4 + 0x58 * i;
				ifile.seekg(h_file_offset);

				//Name
				str.clear();
				c = 0xFF;
				temp_off = 0;
				while (uint8_t(c) != 0xCC)
				{
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
				ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));

				//File size
				ifile.seekg(uint64_t(h_file_offset) + 0x54);
				ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));
				size_multiplier = 1;
				size_str_i = 0;
				while (file_size > uint32_t(1024 * size_multiplier) && size_str_i < 2)
				{
					size_multiplier *= 1024;
					size_str_i += 1;
				}
				float file_size_f = float(file_size) / float(size_multiplier);//File size to display

				size_display << "(" << round(file_size_f * 10) / 10 << size_str[size_str_i] << ")";
				header_offset_display << "[0x" << hex << h_file_offset << "]";
				file_offset_display << "[0x" << hex << file_pointer << "]";
#ifdef IAMT_AUDIO_ONLY
				if (str.find(".tga") == std::string::npos &&
					str.find(".preload") == std::string::npos &&
					str.find(".jpg") == std::string::npos &&
					str.find(".hmap") == std::string::npos &&
					str.find(".jpeg") == std::string::npos &&
					str.find(".txt") == std::string::npos)
#endif
					cout << setw(5) << left << std::dec << files_counter++ << setw(9) << left << header_offset_display.str() << setw(12) << left << file_offset_display.str() << setw(13) << left << size_display.str() << str << '\n';
				size_display.str("");
				header_offset_display.str("");
				file_offset_display.str("");
				size_display.clear();
				header_offset_display.clear();
				file_offset_display.clear();
			}
		}
		else if (hq2x_type == '1')
		{
			cout << "Files amount: " << files_amount << "\nHeader size: 0x" << hex << header_size
				<< "\n[Header OFFS] [File OFFS] (size) path\n";
			for (uint32_t i = 0; i < files_amount; i++)
			{
				h_file_offset = 0x4 + 0x58 * i;
				ifile.seekg(h_file_offset);

				//Name
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

				//File pointer
				ifile.seekg(uint64_t(h_file_offset) + 0x50);
				ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));

				//File size
				ifile.seekg(uint64_t(h_file_offset) + 0x54);
				ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));
				size_multiplier = 1;
				size_str_i = 0;
				while (file_size > uint32_t(1024 * size_multiplier) && size_str_i < 2)
				{
					size_multiplier *= 1024;
					size_str_i += 1;
				}
				float file_size_f = float(file_size) / float(size_multiplier);//File size to display

				size_display << "(" << round(file_size_f * 10) / 10 << size_str[size_str_i] << ")";
				header_offset_display << "[0x" << hex << h_file_offset << "]";
				file_offset_display << "[0x" << hex << file_pointer << "]";
				cout << setw(5) << left << std::dec << files_counter++ << setw(9) << left << header_offset_display.str() << setw(12) << left << file_offset_display.str() << setw(13) << left << size_display.str() << str << '\n';
				size_display.str("");
				header_offset_display.str("");
				file_offset_display.str("");
				size_display.clear();
				header_offset_display.clear();
				file_offset_display.clear();
			}
		}
		if (hq2x_type == '4')
		{
			cout << "Chicken Invaders Universe requires additional table to display results right way.\nOpening \"CIU.iamt_table\".\n";

			ifstream ciu_table(tool_folder + "CIU.iamt_table", ios::binary);
			uint64_t format_check;
			uint32_t ver;//crc or version of table
			uint32_t crc;//crc from hq2x
			uint32_t table_files_amount;

			if (!ciu_table)
			{
				cerr << "[Error] Can't open CIU.iamt_table. Check tool folder.\n";
				cout << "Press Enter to continue...";
				cin.get();
				//cin.ignore();
				exit(1);
			}
			ciu_table.read(reinterpret_cast<char*>(&format_check), sizeof(format_check));
			if (format_check != 0x55494320544D4149)
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
				cout << "Warning: CIU version can be different to CIU table version. Some files won't show names.\n";
			cout << "Files amount: " << files_amount << "\nHeader size: 0x" << hex << header_size
				<< "\n[Header OFFS] [File OFFS] (size) path\n";
			for (uint64_t i = 0; i < files_amount; i++)
			{
				h_file_offset = 0x10 * (i + 1);
				ifile.seekg(h_file_offset);

				//Name				
				ifile.read(reinterpret_cast<char*>(&crc), sizeof(crc));
				for (uint64_t j = 0; j < table_files_amount; j++)
				{
					ciu_table.seekg(0x10 + 0x60 * j + 0x5C);//go read crc from table
					ciu_table.read(reinterpret_cast<char*>(&ver), sizeof(ver));

					if (crc == ver)
					{
						ciu_table.seekg(0x10 + 0x60 * j);
						str.clear();
						c = 0xFF;
						temp_off = 0;
						while (uint8_t(c) != 0x00)
						{
							ciu_table.seekg(0x10 + 0x60 * j + uint64_t(temp_off));
							temp_off++;
							c = ciu_table.get();
							if (uint8_t(c) != 0x00)
								str += c;
						}
						break;
					}
					str = int_to_str(crc, 4);//Reads CRC
					ciu_table.clear();//protection... again...
				}

				//File pointer
				ifile.seekg(uint64_t(h_file_offset) + 0x4);
				ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));

				//File size
				ifile.seekg(uint64_t(h_file_offset) + 0xC);
				ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));
				size_multiplier = 1;
				size_str_i = 0;
				while (file_size > uint32_t(1024 * size_multiplier) && size_str_i < 2)
				{
					size_multiplier *= 1024;
					size_str_i += 1;
				}
				float file_size_f = float(file_size) / float(size_multiplier);//File size to display

				size_display << "(" << round(file_size_f * 10) / 10 << size_str[size_str_i] << ")";
				header_offset_display << "[0x" << hex << h_file_offset << "]";
				file_offset_display << "[0x" << hex << file_pointer << "]";
#ifdef IAMT_AUDIO_ONLY
				if (str.find(".tga") == std::string::npos &&
					str.find(".preload") == std::string::npos &&
					str.find(".jpg") == std::string::npos &&
					str.find(".hmap") == std::string::npos &&
					str.find(".jpeg") == std::string::npos &&
					str.find(".txt") == std::string::npos)
#endif
					cout << setw(5) << left << std::dec << files_counter++ << setw(9) << left << header_offset_display.str() << setw(12) << left << file_offset_display.str() << setw(13) << left << size_display.str() << str << '\n';
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
		cout << "CMD command:\n" << command << '\n';
	}
	else if (mode == 'x')
	{
		cout << "Extract mode.\n";
		command += " -ex ";
		vector<string> files;//Files and folders paths
		string hq2x;//DAT file path
		uint32_t edit_goto;//for "edit X"
		if (wait_for_files == false)
		{
			for (int i = 3; i < argc; i++)
				files.push_back(string(argv[i]));
			hq2x = argv[2];
		}
		else//If there are no files given
		{
			int counter = 1;
			string inpt;
			if (argc < 3)
			{
				hq2x = OpenFileWindow("InterAction WAD Format (.dat; .hq2x; .222x)\0*.HQ2X;*.DAT;*.???X\0IAMI Mod (*.CIUMod;*.CI5Mod;*.CI4Mod;*.CI3Mod;*.CI3OldMod;*.CI2Mod;*.CI2RMod;*.CI1Mod;*.UNIMod;*.SVMod;*.PIMod)\0*.CIUMod;*.CI5Mod;*.CI4Mod;*.CI3Mod;*.CI3OldMod;*.CI2Mod;*.CI2RMod;*.CI1Mod;*.UNIMod;*.SVMod;*.PIMod\0"
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
			}
			else
			{
				hq2x = argv[2];
			}
			cout << "\nEnter the file paths/CRCs of files/folders you want to extract. Put an empty line when you finish.\n"
				"Put an empty line in \"1.\" if you want to extract the whole archive.\n"
				"(Click RMB to paste path from buffer)\n\n"
				"\"undo\" - re-enter last pair.\n\"edit X\" - re-enter Xth pair.\n";
			for (;;)
			{
				cout << counter << ". Path in hq2x: ";
				getline(cin, inpt);
				while (inpt.find('\"') != string::npos)
				{
					inpt.erase(inpt.find('\"'), 1);
				}
				if (inpt == "")
				{
					cout << "\n\nTyping finished.\n";
					break;
				}
				else if (inpt == "undo")
				{
					if (files.size() >= 1)
					{
						files.pop_back();
						counter--;
						continue;
					}
					else {
						cout << "You've reached the beginning!\n";
						continue;
					}
				}
				else if (inpt.find("edit") == 0 && inpt.size() > 4 && inpt[4] == ' ')//edit NUM. In case of "edit 9"... This shit better work on the first try (UPD: Ok it doesn't work)
				{
					try
					{
						edit_goto = stoi(inpt.substr(4));//...this thing will be = 9
					}
					catch (const std::invalid_argument&)
					{
						cout << "Can't find proper number in the line!\n";
						continue;
					}
					//go edit the given path
					if (edit_goto >= 1 && edit_goto < counter)
					{
						cout << edit_goto << ". Path in hq2x: ";
						getline(cin, inpt);
						while (inpt.find('\"') != string::npos)
						{
							inpt.erase(inpt.find('\"'), 1);//delete " symbols
						}
						edit_goto -= 1;
						files[edit_goto] = inpt;
						edit_goto = 0;
						cout << '\n';
						continue;
					}
					else
					{
						cout << "Input value is incorrect.\n";
						continue;
					}
				}
				files.push_back(inpt);
				counter++;
			}
		}

		for (uint32_t i = 0; i < files.size(); i++)//Replace all \ with / in hq2x paths
		{
			while (files[i].find('\\') != string::npos)
			{
				files[i].at(files[i].find('\\')) = '/';
			}
		}
		command += '\"' + hq2x + "\" ";

		cout << "DAT file: \"" << hq2x << "\"\nFiles/folders to be extracted:\n";
		if (files.size() == 0)
			cout << "Extracting the whole archive.\n";
		else
		{
			for (uint32_t i = 0; i < files.size(); i++)
			{
				command += '\"' + files[i] + "\" ";
				cout << "@ " << files[i] << '\n';
			}
		}
		cout << "\nCMD command:\n" << command << "\n\n";
		for (;;)
		{
			string inpt;
			cout << "Do you want to extract files? (y/n): ";
			getline(cin, inpt);
			if (inpt == "y")
				break;
			if (inpt == "n")
			{
				cout << "\nExtraction canceled.\n";
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
		cout << "Opened \"" << hq2x << "\".\n\n";
		uint32_t files_amount;
		uint32_t h_file_offset;//Offset of the file in header
		uint32_t file_pointer;//Offset of the file itself
		uint32_t file_size;
		uint32_t real_file_size;
		//uint32_t header_size;
		uint32_t temp_off;//Used for reading names
		string folder;//root extraction folder
		string sub_folder;//for files
		string file_name;
		unsigned char c;

		//Which hq2x type
		ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		if (files_amount == 0x20455655)//CIU v35+ WAD4 "UVE " line
			hq2x_type = '4';
		else if (files_amount >= 0 && files_amount <= 10)//old CI4-all HD versions + old CIU
			hq2x_type = '3';
		else//check 04-07
		{
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
			if (files_amount == '/xfg' || files_amount == '/xfs' || files_amount == 'isum' || files_amount == 'tsys')// "gfx/" "sfx/" "musi" "syst"
				hq2x_type = '1';
			else if (files_amount == 0x6799186F)//mods
				hq2x_type = 'M';
			else
				hq2x_type = '2';
		}

		//Read files amount
		if (hq2x_type == '3')
		{
			ifile.seekg(0x4);
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		}
		else if (hq2x_type == '4')
		{
			ifile.seekg(0xC);
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		}
		else if (hq2x_type == 'M')//Mod
		{
			ifile.seekg(0xC);
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
			files_amount = decursed(files_amount, 0xC);
		}
		else
		{
			ifile.seekg(0);
			ifile.read(reinterpret_cast<char*>(&files_amount), sizeof(uint32_t));
		}


		string ffolder = hq2x.substr(hq2x.rfind('\\') + 1);//input file folder
		while (ffolder.find('.') != string::npos)
			ffolder[ffolder.find('.')] = '_';
		folder = hq2x.substr(0, hq2x.rfind('\\') + 1) + "@" + ffolder + '\\';
		_mkdir(folder.c_str());
		//extract all
		if (files.size() == 0)
		{
			if (hq2x_type == '4')
			{
				cout << "Chicken Invaders Universe requires additional table to get file name.\nOpening \"CIU.iamt_table\".\n";
				ifstream ciu_table(tool_folder + "CIU.iamt_table", ios::binary);
				uint64_t format_check;
				uint32_t ver;//crc or version of table
				uint32_t crc;//crc from hq2x
				uint32_t table_files_amount;

				if (!ciu_table)
				{
					cerr << "[Error] Can't open CIU.iamt_table. Check tool folder.\n";
					cout << "Press Enter to continue...";
					cin.get();
					//cin.ignore();
					exit(1);
				}
				ciu_table.read(reinterpret_cast<char*>(&format_check), sizeof(format_check));
				if (format_check != 0x55494320544D4149)
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

				for (uint64_t i = 0; i < files_amount; i++)
				{
					h_file_offset = 0x10 * (i + 1);
					ifile.seekg(h_file_offset);

					//Name				
					ifile.read(reinterpret_cast<char*>(&crc), sizeof(crc));
					for (uint64_t j = 0; j < table_files_amount; j++)
					{
						ciu_table.seekg(0x10 + 0x60 * j + 0x5C);//go read crc from table
						ciu_table.read(reinterpret_cast<char*>(&ver), sizeof(ver));

						if (crc == ver)
						{
							ciu_table.seekg(0x10 + 0x60 * j);
							str.clear();
							c = 0xFF;
							temp_off = 0;
							while (uint8_t(c) != 0x00)
							{
								ciu_table.seekg(0x10 + 0x60 * j + uint64_t(temp_off));
								temp_off++;
								c = ciu_table.get();
								if (uint8_t(c) != 0x00)
									str += c;//str == path of the file in hq2x
							}
							sub_folder = str.substr(0, str.rfind('/') + 1);
							file_name = str.substr(str.rfind('/') + 1);
							break;
						}
						ciu_table.clear();//protection
						str = int_to_str(crc, 4);//Reads CRC
						sub_folder = "NO_NAME/";
						file_name = str.substr(2);//remove 0x
					}
#if defined(IAMT_AUDIO_ONLY) || defined(IAMT_NO_CIU)
					if (file_name.find(".tga") != std::string::npos) {
						continue;
					}
#endif

					//File pointer
					ifile.seekg(uint64_t(h_file_offset) + 0x4);
					ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));
					//File size
					ifile.seekg(uint64_t(h_file_offset) + 0xC);
					ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));
					char* buffer = new char[file_size];
					ifile.seekg(file_pointer);
					ifile.read(buffer, file_size);

					//Guess the extension
					if (sub_folder == "NO_NAME/") {
						ifile.seekg(file_pointer);
						std::string guessed_extension = ".txt";
						uint32_t Magic = 0;
						ifile.read(reinterpret_cast<char*>(&Magic), sizeof(Magic));
						switch (Magic) {
						case 'SggO':
							guessed_extension = ".ogg";
							break;
						case 'FFIR':
							guessed_extension = ".wav";
							break;
						default:
							if ((Magic & 0x00FFFFFF) == '\0003DI') {
								guessed_extension = ".mp3";
								break;
							}
							if ((Magic & 0x0000FFFF) == 0x0000D8FF) {
								guessed_extension = ".jpg";
								break;
							}

							if (Magic < 0xFFFF && Magic != 0) {
								ifile.read(reinterpret_cast<char*>(&Magic), sizeof(Magic));
								if (Magic < 0xFFFF && Magic != 0) {
									ifile.read(reinterpret_cast<char*>(&Magic), sizeof(Magic));
									switch (Magic) {
									case 8:
									case 24:
									case 32:
										guessed_extension = ".tga";
										break;
									default:
										break;
									}
								}
							}

							ifile.seekg(0xC);
							ifile.read(reinterpret_cast<char*>(&Magic), sizeof(Magic));
							if ((Magic & 0xFFFFFF00) == 0xCCCCCC00) {
								guessed_extension = ".tga.ini.preload";
								break;
							}
							break;
						}
						file_name += guessed_extension;
					}

					cout << "Creating folder: " << folder + sub_folder << '\n';
					vector<string> sub_folder_sep = Separate(sub_folder);
					string temp_path = folder;
					for (uint32_t j = 0; j < sub_folder_sep.size(); j++)
					{
						temp_path += sub_folder_sep[j] + "/";
						_mkdir(temp_path.c_str());
					}
					ofile.open(folder + sub_folder + file_name, ios::out | ios::binary);
					if (!ofile)
					{
						cerr << "[Error] Can't create " << folder + sub_folder + file_name << '\n';
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					ofile.write(buffer, file_size);
					ofile.close();
					delete[] buffer;
					cout << "Extracted: " << str << '\n';
				}
				ifile.close();
				ciu_table.close();
			}
			if (hq2x_type == '3')
			{
				for (uint64_t i = 0; i < files_amount; i++)
				{
					h_file_offset = 0x8 + 0x5c * i;
					ifile.seekg(h_file_offset);

					//Name				
					str.clear();
					c = 0xFF;
					temp_off = 0;
					while (uint8_t(c) != 0xCC)
					{
						ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
						temp_off++;
						c = ifile.get();
						if (uint8_t(c) != 0xcc)
							str += c;
					}

					//Decode name
					str = str_xor(str);
#ifdef IAMT_AUDIO_ONLY
					if (str.find(".tga") != std::string::npos) {
						continue;
					}
#endif
					sub_folder = str.substr(0, str.rfind('/') + 1);
					file_name = str.substr(str.rfind('/') + 1);

					// File pointer
					ifile.seekg(uint64_t(h_file_offset) + 0x50);
					ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));
					//File size
					ifile.seekg(uint64_t(h_file_offset) + 0x58);
					ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));

					char* buffer = new char[file_size];
					ifile.seekg(file_pointer);
					ifile.read(buffer, file_size);
					cout << "Creating folder: " << folder + sub_folder << '\n';
					vector<string> sub_folder_sep = Separate(sub_folder);
					string temp_path = folder;
					for (uint32_t j = 0; j < sub_folder_sep.size(); j++)
					{
						temp_path += sub_folder_sep[j] + "/";
						_mkdir(temp_path.c_str());
					}
					ofile.open(folder + sub_folder + file_name, ios::out | ios::binary);
					if (!ofile)
					{
						cerr << "[Error] Can't create " << folder + sub_folder + file_name << '\n';
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					ofile.write(buffer, file_size);
					ofile.close();
					delete[] buffer;
					cout << "Extracted: " << str << '\n';
				}
				ifile.close();
			}
			if (hq2x_type == '2')
			{
				for (uint64_t i = 0; i < files_amount; i++)
				{
					h_file_offset = 0x4 + 0x58 * i;
					ifile.seekg(h_file_offset);

					//Name				
					str.clear();
					c = 0xFF;
					temp_off = 0;
					while (uint8_t(c) != 0xCC)
					{
						ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
						temp_off++;
						c = ifile.get();
						if (uint8_t(c) != 0xcc)
							str += c;
					}

					//Decode name
					str = str_xor(str);
#ifdef IAMT_AUDIO_ONLY
					if (str.find(".tga") != std::string::npos) {
						continue;
					}
#endif
					sub_folder = str.substr(0, str.rfind('/') + 1);
					file_name = str.substr(str.rfind('/') + 1);

					// File pointer
					ifile.seekg(uint64_t(h_file_offset) + 0x50);
					ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));
					//File size
					ifile.seekg(uint64_t(h_file_offset) + 0x54);
					ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));

					char* buffer = new char[file_size];
					ifile.seekg(file_pointer);
					ifile.read(buffer, file_size);
					cout << "Creating folder: " << folder + sub_folder << '\n';
					vector<string> sub_folder_sep = Separate(sub_folder);
					string temp_path = folder;
					for (uint32_t j = 0; j < sub_folder_sep.size(); j++)
					{
						temp_path += sub_folder_sep[j] + "/";
						_mkdir(temp_path.c_str());
					}
					ofile.open(folder + sub_folder + file_name, ios::out | ios::binary);
					if (!ofile)
					{
						cerr << "[Error] Can't create " << folder + sub_folder + file_name << '\n';
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					ofile.write(buffer, file_size);
					ofile.close();
					delete[] buffer;
					cout << "Extracted: " << str << '\n';
				}
				ifile.close();
			}
			if (hq2x_type == '1')
			{
				for (uint64_t i = 0; i < files_amount; i++)
				{
					h_file_offset = 0x4 + 0x58 * i;
					ifile.seekg(h_file_offset);

					//Name				
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

					sub_folder = str.substr(0, str.rfind('/') + 1);
					file_name = str.substr(str.rfind('/') + 1);

					// File pointer
					ifile.seekg(uint64_t(h_file_offset) + 0x50);
					ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));
					//File size
					ifile.seekg(uint64_t(h_file_offset) + 0x54);
					ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));

					char* buffer = new char[file_size];
					ifile.seekg(file_pointer);
					ifile.read(buffer, file_size);
					cout << "Creating folder: " << folder + sub_folder << '\n';
					vector<string> sub_folder_sep = Separate(sub_folder);
					string temp_path = folder;
					for (uint32_t j = 0; j < sub_folder_sep.size(); j++)
					{
						temp_path += sub_folder_sep[j] + "/";
						_mkdir(temp_path.c_str());
					}
					ofile.open(folder + sub_folder + file_name, ios::out | ios::binary);
					if (!ofile)
					{
						cerr << "[Error] Can't create " << folder + sub_folder + file_name << '\n';
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					ofile.write(buffer, file_size);
					ofile.close();
					delete[] buffer;
					cout << "Extracted: " << str << '\n';
				}
				ifile.close();
			}
			if (hq2x_type == 'M')
			{
#if defined(IAMT_AUDIO_ONLY) || defined(IAMT_NO_CIU)
				ifile.close();
				std::cout << "Can't use. Mod files are not allowed to be extracted.\n";
				goto MODE_SELECTION;
#endif

				for (uint64_t i = 0; i < files_amount; i++)
				{
					h_file_offset = 0x160 + 0x60 * i;
					ifile.seekg(h_file_offset);

					//Read file name (in IAMI main.cpp needed stuff starts from 430th line)
					/*str.clear();
					c = 0xFF;
					temp_off = 0;
					while (uint8_t(c) != 0x00)
					{
						ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
						temp_off++;
						c = ifile.get();
						if (uint8_t(c) != 0x00)
							str += c;
					}*/

					char* buffer = new char[0x58];
					ifile.read(buffer, 0x58);
					IamiProtection(buffer, 0x58, h_file_offset);
					str = buffer;
					delete[] buffer;

					sub_folder = str.substr(0, str.rfind('/') + 1);
					file_name = str.substr(str.rfind('/') + 1);

					// File pointer
					ifile.seekg(uint64_t(h_file_offset) + 0x58);
					ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(uint32_t));
					file_pointer = decursed(file_pointer, uint64_t(h_file_offset) + 0x58);
					//File size
					ifile.seekg(uint64_t(h_file_offset) + 0x5C);
					ifile.read(reinterpret_cast<char*>(&file_size), sizeof(uint32_t));
					file_size = decursed(file_size, uint64_t(h_file_offset) + 0x5C);

					buffer = new char[file_size];
					ifile.seekg(file_pointer);
					ifile.read(buffer, file_size);
					IamiProtection(buffer, file_size, file_pointer);
					cout << "Creating folder: " << folder + sub_folder << '\n';
					vector<string> sub_folder_sep = Separate(sub_folder);
					string temp_path = folder;
					for (uint32_t j = 0; j < sub_folder_sep.size(); j++)
					{
						temp_path += sub_folder_sep[j] + "/";
						_mkdir(temp_path.c_str());
					}
					ofile.open(folder + sub_folder + file_name, ios::out | ios::binary);
					if (!ofile)
					{
						cerr << "[Error] Can't create " << folder + sub_folder + file_name << '\n';
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					ofile.write(buffer, file_size);
					ofile.close();
					delete[] buffer;
					cout << "Extracted: " << str << '\n';
			}
				ifile.close();
		}
	}
		else
		{
			if (hq2x_type == '4')
			{
				cout << "Chicken Invaders Universe requires additional table to get file name.\nOpening \"CIU.iamt_table\".\n";
				ifstream ciu_table(tool_folder + "CIU.iamt_table", ios::binary);
				uint64_t format_check;
				uint32_t ver;//crc or version of table
				uint32_t crc;//crc from hq2x
				uint32_t table_files_amount;
				bool name_exists = false;
				bool found = false;

				if (!ciu_table)
				{
					cerr << "[Error] Can't open CIU.iamt_table. Check tool folder.\n";
					cout << "Press Enter to continue...";
					cin.get();
					//cin.ignore();
					exit(1);
				}
				ciu_table.read(reinterpret_cast<char*>(&format_check), sizeof(format_check));
				if (format_check != 0x55494320544D4149)
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

				//read all folders and turn them into paths for files
				for (uint64_t queue = 0; queue < files.size(); queue++)
				{
					res = files[queue];//read path
					if (res.find('.') != string::npos)//if there is a dot
					{
						continue;
					}
					else
					{
						for (uint64_t i = 0; i < table_files_amount; i++)
						{
							str.clear();
							c = 0xFF;
							temp_off = 0;
							//read the path from table
							while (uint8_t(c) != 0x00)
							{
								ciu_table.seekg(0x10 + 0x60 * i + uint64_t(temp_off));
								temp_off++;
								c = ciu_table.get();
								if (uint8_t(c) != 0x00)
									str += c;
							}
							//str - from table
							//res - from query
							if ((str.find(res) != string::npos) && (size(str) != size(res)))//if found a part with a needed folder and it's not the whole path
							{
								files.push_back(str);
							}
						}
					}
				}
				int num = files.size() - 1;
				//Remove fodlers from paths list
				while (num >= 0)
				{
					res = files[num];//read path
					if (res.find('.') == string::npos)//if there is NO dot
					{
						files.erase(files.begin() + num);
						continue;
					}
					num--;
				}

				for (uint64_t queue = 0; queue < files.size(); queue++)
				{
					uint32_t f_num = 0;//number of the file which is needed in the header

					res = files[queue];//searching for this
#if defined(IAMT_AUDIO_ONLY) || defined(IAMT_NO_CIU)
					if (res.find(".tga") != std::string::npos) {
						continue;
					}
#endif
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
						sub_folder = res.substr(0, res.rfind('/') + 1);
						file_name = res.substr(res.rfind('/') + 1);
					}
					else
					{
						//turn text into crc in case we can't find it as name
						cout << "Can't find name in the table. Reading as CRC: ";
						ver = strtoul(res.c_str(), NULL, 16);
						cout << hex << ver << '\n';
						sub_folder = "NO_NAME/";
						file_name = res;
						ciu_table.clear();//protection
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
						ifile.read(reinterpret_cast<char*>(&real_file_size), sizeof(uint32_t));
					}
					else
					{
						cerr << "\n[Error] Can't find given file path/CRC \"" << files[queue] << "\" in the header.\n"
							"You can try using CMD command instead:\n" << command << '\n';
						//for (;;)
						//{
						//	string inpt;
						//	cout << "Do you want to skip this file? (y/n): ";
						//	getline(cin, inpt);
						//	if (inpt == "y")
						//		break;
						//	if (inpt == "n")
						//	{
						//		cout << "\nExtraction canceled.\n";
						//		cout << "Press Enter to continue...";
						//		cin.get();
						//		//cin.ignore();
						//		exit(0);
						//	}
						//	else
						//	{
						//		cout << "\r";
						//	}
						//}
						ciu_table.clear();
						ifile.clear();
						cout << "\n\n";
						continue;
					}

					//Guess the extension
					if (sub_folder == "NO_NAME/") {
						ifile.seekg(file_pointer);
						std::string guessed_extension = ".txt";
						uint32_t Magic = 0;
						ifile.read(reinterpret_cast<char*>(&Magic), sizeof(Magic));
						switch (Magic) {
						case 'SggO':
							guessed_extension = ".ogg";
							break;
						case 'FFIR':
							guessed_extension = ".wav";
							break;
						default:
							if ((Magic & 0x00FFFFFF) == '\0003DI') {
								guessed_extension = ".mp3";
								break;
							}
							if ((Magic & 0x0000FFFF) == 0x0000D8FF) {
								guessed_extension = ".jpg";
								break;
							}

							if (Magic < 0xFFFF && Magic != 0) {
								ifile.read(reinterpret_cast<char*>(&Magic), sizeof(Magic));
								if (Magic < 0xFFFF && Magic != 0) {
									ifile.read(reinterpret_cast<char*>(&Magic), sizeof(Magic));
									switch (Magic) {
									case 8:
									case 24:
									case 32:
										guessed_extension = ".tga";
										break;
									default:
										break;
									}
								}
							}

							ifile.seekg(0xC);
							ifile.read(reinterpret_cast<char*>(&Magic), sizeof(Magic));
							if ((Magic & 0xFFFFFF00) == 0xCCCCCC00) {
								guessed_extension = ".tga.ini.preload";
								break;
							}
							break;
						}
						file_name += guessed_extension;
					}

					char* buffer = new char[real_file_size];
					ifile.seekg(file_pointer);
					ifile.read(buffer, real_file_size);
					cout << "Creating folder: " << folder + sub_folder << '\n';
					vector<string> sub_folder_sep = Separate(sub_folder);
					string temp_path = folder;
					for (uint32_t j = 0; j < sub_folder_sep.size(); j++)
					{
						temp_path += sub_folder_sep[j] + "/";
						_mkdir(temp_path.c_str());
					}
					ofile.open(folder + sub_folder + file_name, ios::out | ios::binary);
					if (!ofile)
					{
						cerr << "[Error] Can't create " << folder + sub_folder + file_name << '\n';
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					ofile.write(buffer, real_file_size);
					ofile.close();
					delete[] buffer;
					cout << "Extracted: " << sub_folder + file_name << '\n';
				}
				ifile.close();
				ciu_table.close();
			}
			if (hq2x_type == '3')
			{
				bool name_exists = false;
				bool found = false;

				//read all folders and turn them into paths for files
				for (uint64_t queue = 0; queue < files.size(); queue++)
				{
					res = files[queue];//read path
					if (res.find('.') != string::npos)//if there is a dot
					{
						continue;
					}
					else
					{
						for (uint64_t i = 0; i < files_amount; i++)
						{
							h_file_offset = 0x8 + 0x5c * i;//file in the header offset
							str.clear();
							c = 0xFF;
							temp_off = 0;
							//read the path from table
							while (uint8_t(c) != 0xcc)
							{
								ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
								temp_off++;
								c = ifile.get();
								if (uint8_t(c) != 0xcc)
									str += c;
							}
							str = str_xor(str);
							//str - from header
							//res - from query
							if ((str.find(res) != string::npos) && (size(str) != size(res)))//if found a part with a needed folder and it's not the whole path
							{
								files.push_back(str);
							}
						}
					}
				}
				int num = files.size() - 1;
				//Remove fodlers from paths list
				while (num >= 0)
				{
					res = files[num];//read path
					if (res.find('.') == string::npos)//if there is NO dot
					{
						files.erase(files.begin() + num);
						continue;
					}
					num--;
				}
				///////////////

				for (uint64_t queue = 0; queue < files.size(); queue++)
				{
					str = files[queue];
#ifdef IAMT_AUDIO_ONLY
					if (str.find(".tga") != std::string::npos) {
						continue;
					}
#endif
					res.clear();
					res = str_xor(str);
					uint32_t f_num = 0;//number of the file which is needed in the header
					while (f_num < files_amount)
					{
						h_file_offset = 0x8 + 0x5c * f_num;//file in the header offset
						ifile.seekg(h_file_offset);
						//Read the name from file
						str.clear();
						c = 0xFF;
						temp_off = 0;
						while (uint8_t(c) != 0xCC)
						{
							ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
							temp_off++;
							c = ifile.get();
							if (uint8_t(c) != 0xcc)
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
						ifile.read(reinterpret_cast<char*>(&real_file_size), sizeof(uint32_t));
						sub_folder = files[queue].substr(0, files[queue].rfind('/') + 1);
						file_name = files[queue].substr(files[queue].rfind('/') + 1);
					}
					else {
						cerr << "\n[Error] Can't find given file path \"" << files[queue] << "\" in the header.\n"
							"You can also try using CMD command instead:\n" << command << '\n';
						for (;;)
						{
							string inpt;
							cout << "Do you want to skip this file? (y/n): ";
							getline(cin, inpt);
							if (inpt == "y")
								break;
							if (inpt == "n")
							{
								cout << "\nExtraction canceled.\n";
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
						cout << "\n\n";
						continue;
						}

					char* buffer = new char[real_file_size];
					ifile.seekg(file_pointer);
					ifile.read(buffer, real_file_size);
					cout << "Creating folder: " << folder + sub_folder << '\n';
					vector<string> sub_folder_sep = Separate(sub_folder);
					string temp_path = folder;
					for (uint32_t j = 0; j < sub_folder_sep.size(); j++)
					{
						temp_path += sub_folder_sep[j] + "/";
						_mkdir(temp_path.c_str());
					}
					ofile.open(folder + sub_folder + file_name, ios::out | ios::binary);
					if (!ofile)
					{
						cerr << "[Error] Can't create " << folder + sub_folder + file_name << '\n';
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					ofile.write(buffer, real_file_size);
					ofile.close();
					delete[] buffer;
					cout << "Extracted: " << files[queue] << '\n';
					}
				ifile.close();
				}
			if (hq2x_type == '2')
			{
				bool name_exists = false;
				bool found = false;

				//read all folders and turn them into paths for files
				for (uint64_t queue = 0; queue < files.size(); queue++)
				{
					res = files[queue];//read path
					if (res.find('.') != string::npos)//if there is a dot
					{
						continue;
					}
					else
					{
						for (uint64_t i = 0; i < files_amount; i++)
						{
							h_file_offset = 0x4 + 0x58 * i;//file in the header offset
							str.clear();
							c = 0xFF;
							temp_off = 0;
							//read the path from table
							while (uint8_t(c) != 0xcc)
							{
								ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
								temp_off++;
								c = ifile.get();
								if (uint8_t(c) != 0xcc)
									str += c;
							}
							str = str_xor(str);
							//str - from header
							//res - from query
							if ((str.find(res) != string::npos) && (size(str) != size(res)))//if found a part with a needed folder and it's not the whole path
							{
								files.push_back(str);
							}
						}
					}
				}
				int num = files.size() - 1;
				//Remove fodlers from paths list
				while (num >= 0)
				{
					res = files[num];//read path
					if (res.find('.') == string::npos)//if there is NO dot
					{
						files.erase(files.begin() + num);
						continue;
					}
					num--;
				}
				///////////////

				for (uint64_t queue = 0; queue < files.size(); queue++)
				{
					str = files[queue];
#ifdef IAMT_AUDIO_ONLY
					if (str.find(".tga") != std::string::npos) {
						continue;
					}
#endif
					res.clear();
					res = str_xor(str);
					uint32_t f_num = 0;//number of the file which is needed in the header
					while (f_num < files_amount)
					{
						h_file_offset = 0x4 + 0x58 * f_num;//file in the header offset
						ifile.seekg(h_file_offset);
						//Read the name from file
						str.clear();
						c = 0xFF;
						temp_off = 0;
						while (uint8_t(c) != 0xCC)
						{
							ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
							temp_off++;
							c = ifile.get();
							if (uint8_t(c) != 0xcc)
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
						sub_folder = files[queue].substr(0, files[queue].rfind('/') + 1);
						file_name = files[queue].substr(files[queue].rfind('/') + 1);
					}
					else {
						cerr << "\n[Error] Can't find given file path \"" << files[queue] << "\" in the header.\n"
							"You can also try using CMD command instead:\n" << command << '\n';
						for (;;)
						{
							string inpt;
							cout << "Do you want to skip this file? (y/n): ";
							getline(cin, inpt);
							if (inpt == "y")
								break;
							if (inpt == "n")
							{
								cout << "\nExtraction canceled.\n";
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
						cout << "\n\n";
						continue;
						}

					char* buffer = new char[file_size];
					ifile.seekg(file_pointer);
					ifile.read(buffer, file_size);
					cout << "Creating folder: " << folder + sub_folder << '\n';
					vector<string> sub_folder_sep = Separate(sub_folder);
					string temp_path = folder;
					for (uint32_t j = 0; j < sub_folder_sep.size(); j++)
					{
						temp_path += sub_folder_sep[j] + "/";
						_mkdir(temp_path.c_str());
					}
					ofile.open(folder + sub_folder + file_name, ios::out | ios::binary);
					if (!ofile)
					{
						cerr << "[Error] Can't create " << folder + sub_folder + file_name << '\n';
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					ofile.write(buffer, file_size);
					ofile.close();
					delete[] buffer;
					cout << "Extracted: " << files[queue] << '\n';
					}
				ifile.close();
				}
			if (hq2x_type == '1')
			{
				bool name_exists = false;
				bool found = false;

				//read all folders and turn them into paths for files
				for (uint64_t queue = 0; queue < files.size(); queue++)
				{
					res = files[queue];//read path
					if (res.find('.') != string::npos)//if there is a dot
					{
						continue;
					}
					else
					{
						for (uint64_t i = 0; i < files_amount; i++)
						{
							h_file_offset = 0x4 + 0x58 * i;//file in the header offset
							str.clear();
							c = 0xFF;
							temp_off = 0;
							//read the path from table
							while (uint8_t(c) != 0x00)
							{
								ifile.seekg(uint64_t(h_file_offset) + uint64_t(temp_off));
								temp_off++;
								c = ifile.get();
								if (uint8_t(c) != 0x00)
									str += c;
							}
							//str - from header
							//res - from query
							if ((str.find(res) != string::npos) && (size(str) != size(res)))//if found a part with a needed folder and it's not the whole path
							{
								files.push_back(str);
							}
						}
					}
				}
				int num = files.size() - 1;
				//Remove fodlers from paths list
				while (num >= 0)
				{
					res = files[num];//read path
					if (res.find('.') == string::npos)//if there is NO dot
					{
						files.erase(files.begin() + num);
						continue;
					}
					num--;
				}
				///////////////

				for (uint64_t queue = 0; queue < files.size(); queue++)
				{
					str = files[queue];
					res.clear();
					res = str;
					uint32_t f_num = 0;//number of the file which is needed in the header
					while (f_num < files_amount)
					{
						h_file_offset = 0x4 + 0x58 * f_num;//file in the header offset
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
						sub_folder = files[queue].substr(0, files[queue].rfind('/') + 1);
						file_name = files[queue].substr(files[queue].rfind('/') + 1);
					}
					else {
						cerr << "\n[Error] Can't find given file path \"" << files[queue] << "\" in the header.\n"
							"You can also try using CMD command instead:\n" << command << '\n';
						for (;;)
						{
							string inpt;
							cout << "Do you want to skip this file? (y/n): ";
							getline(cin, inpt);
							if (inpt == "y")
								break;
							if (inpt == "n")
							{
								cout << "\nExtraction canceled.\n";
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
						cout << "\n\n";
						continue;
					}

					char* buffer = new char[file_size];
					ifile.seekg(file_pointer);
					ifile.read(buffer, file_size);
					cout << "Creating folder: " << folder + sub_folder << '\n';
					vector<string> sub_folder_sep = Separate(sub_folder);
					string temp_path = folder;
					for (uint32_t j = 0; j < sub_folder_sep.size(); j++)
					{
						temp_path += sub_folder_sep[j] + "/";
						_mkdir(temp_path.c_str());
					}
					ofile.open(folder + sub_folder + file_name, ios::out | ios::binary);
					if (!ofile)
					{
						cerr << "[Error] Can't create " << folder + sub_folder + file_name << '\n';
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					ofile.write(buffer, file_size);
					ofile.close();
					delete[] buffer;
					cout << "Extracted: " << files[queue] << '\n';
				}
				ifile.close();
			}
			if (hq2x_type == 'M')
			{
#ifdef IAMT_AUDIO_ONLY
				ifile.close();
				std::cout << "Can't use. Mod files are not allowed to be extracted.\n";
				goto MODE_SELECTION;
#endif
				bool name_exists = false;
				bool found = false;

				//read all folders and turn them into paths for files
				for (uint64_t queue = 0; queue < files.size(); queue++)
				{
					res = files[queue];//read path
					if (res.find('.') != string::npos)//if there is a dot
					{
						continue;
					}
					else
					{
						for (uint64_t i = 0; i < files_amount; i++)
						{
							h_file_offset = 0x160 + 0x60 * i;//file in the header offset
							ifile.seekg(h_file_offset);
							char* buffer = new char[0x58];
							ifile.read(buffer, 0x58);
							IamiProtection(buffer, 0x58, h_file_offset);
							str = buffer;
							delete[] buffer;
							//str - from header
							//res - from query
							if ((str.find(res) != string::npos) && (size(str) != size(res)))//if found a part with a needed folder and it's not the whole path
							{
								files.push_back(str);
							}
						}
					}
				}
				int num = files.size() - 1;
				//Remove folders from paths list
				while (num >= 0)
				{
					res = files[num];//read path
					if (res.find('.') == string::npos)//if there is NO dot
					{
						files.erase(files.begin() + num);
						continue;
					}
					num--;
				}

				for (uint64_t queue = 0; queue < files.size(); queue++)
				{
					str = files[queue];
					res.clear();
					uint32_t f_num = 0;//number of the file which is needed in the header
					while (f_num < files_amount)
					{
						h_file_offset = 0x160 + 0x60 * f_num;//file in the header offset
						ifile.seekg(h_file_offset);
						//Read the name from file
						char* buffer = new char[0x58];
						ifile.read(buffer, 0x58);
						IamiProtection(buffer, 0x58, h_file_offset);
						res = buffer;
						delete[] buffer;
						if (res == str)
						{
							found = true;
							break;
						}
						f_num++;
				}
					if (found == true)
					{
						ifile.seekg(uint64_t(h_file_offset) + uint64_t(0x58));
						ifile.read(reinterpret_cast<char*>(&file_pointer), sizeof(file_pointer));//poiner value
						ifile.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));//size value
						file_pointer = decursed(file_pointer, uint64_t(h_file_offset) + 0x58);
						file_size = decursed(file_size, uint64_t(h_file_offset) + 0x5C);
						sub_folder = files[queue].substr(0, files[queue].rfind('/') + 1);
						file_name = files[queue].substr(files[queue].rfind('/') + 1);
					}
					else {
						cerr << "\n[Error] Can't find given file path \"" << files[queue] << "\" in the header.\n"
							"You can also try using CMD command instead:\n" << command << '\n';
						for (;;)
						{
							string inpt;
							cout << "Do you want to skip this file? (y/n): ";
							getline(cin, inpt);
							if (inpt == "y")
								break;
							if (inpt == "n")
							{
								cout << "\nExtraction canceled.\n";
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
						cout << "\n\n";
						continue;
					}

					char* buffer = new char[file_size];
					ifile.seekg(file_pointer);
					ifile.read(buffer, file_size);
					IamiProtection(buffer, file_size, file_pointer);
					cout << "Creating folder: " << folder + sub_folder << '\n';
					vector<string> sub_folder_sep = Separate(sub_folder);
					string temp_path = folder;
					for (uint32_t j = 0; j < sub_folder_sep.size(); j++)
					{
						temp_path += sub_folder_sep[j] + "/";
						_mkdir(temp_path.c_str());
					}
					ofile.open(folder + sub_folder + file_name, ios::out | ios::binary);
					if (!ofile)
					{
						cerr << "[Error] Can't create " << folder + sub_folder + file_name << '\n';
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					ofile.write(buffer, file_size);
					ofile.close();
					delete[] buffer;
					cout << "Extracted: " << files[queue] << '\n';
			}
				ifile.close();
			}
			}
		cout << "Extraction finished!\n\a";
		}

	//NEW ONE
	else if (mode == 'u')
	{
		std::cout << "Table update mode.\nNote that you NEED to launch this shit as administrator for this mode!\n\nThis mode needs some important additional explanation. Check USAGE.txt for more info.\nSet loadMode=0 in CIU config. Also, back up your CIU.dat.hq2x as it might get corrupted.\n"
			"Write CIU version down (if v41.1 - write 41, if 46.9 - write 46)\n>";
		command += " -u ";

		string hq2x_path;// = "C:\\Program Files (x86)\\ChickenInvadersUniverse\\CIU.dat.hq2x";
		string table_path;// = "C:\\Users\\████\\source\\repos\\CIU string decoder encoder\\Release\\share\\CIU.cimt_table";
		string exe_path;// = "C:\\Program Files (x86)\\ChickenInvadersUniverse\\CIU.exe";
		uint32_t version;
		std::cin >> version;

		hq2x_path = OpenFileWindow("CIU WAD Format (.dat.hq2x; .222x)\0*.HQ2X;*.???X\0All Files\0*.*\0");
		table_path = OpenFileWindow("IAMT Table Format (.iamt_table)\0*.IAMT_TABLE\0All Files\0*.*\0");
		exe_path = OpenFileWindow("CIU Executable (.exe)\0*.EXE\0All Files\0*.*\0");

		std::cout << "\nhq2x_path:" << hq2x_path <<
			"\ntable_path:" << table_path <<
			"\nexe_path:" << exe_path << "\n\nIf you select the wrong file, please, close the tool and try again.\nIf you see gibberish instead of other non-latin symbols, don't worry: that's probably encoding issue which won't affect anything.\n";
		BetterCallPause();
		//cin.ignore();

		ifstream hq2x(hq2x_path, ios::in | ios::out | ios::binary);//<CIU.dat.hq2x>
		if (!hq2x) {
			std::cerr << "[Error] Can't open the hq2x!\n";
			BetterCallPause();
			std::exit(5);
		}

		fstream table;//<.cimt_table>

		uint32_t famount;//files amount
		uint32_t t_famount = 0;
		uint32_t hash;
		uint32_t t_crc = 0;//crc in the table
		string name = "";
		bool exists = false;

		hq2x.seekg(0xC);
		hq2x.read(reinterpret_cast<char*>(&famount), sizeof(famount));
		hq2x.close();

		table.open(table_path, ios::in | ios::out | ios::binary);
		if (!table) {
			std::cerr << "[Error] Can't open the table!\n";
			BetterCallPause();
			std::exit(7);
		}
		/* If the file is empty */
		if (table.peek() == std::ifstream::traits_type::eof()) {
			std::printf("The table is empty. Writing the header.\n");
			table.clear();
			table.write("IAMT CIU", 8);
			for (int i = 0; i < 8; ++i) {
				table.put(0);
			}
		}
		table.seekg(0x8);
		table.write(reinterpret_cast<char*>(&version), 4);
		table.close();

		uint32_t* hq2x_hashes = new uint32_t[famount];

		hq2x.open(hq2x_path, ios::binary);
		for (uint64_t i = 0; i < famount; i++) {
			hq2x.seekg(0x10 + (i * 0x10));
			hq2x.read(reinterpret_cast<char*>(&hq2x_hashes[i]), sizeof(hq2x_hashes[0]));//read the original CRC
		}
		hq2x.close();

		table.open(table_path, std::ios::in | std::ios::binary);
		table.seekg(0xC);
		table.read(reinterpret_cast<char*>(&t_famount), sizeof(t_famount));
		std::vector<TableEntry> table_entries;
		table_entries.reserve(t_famount);
		for (int j = 0; j < t_famount; ++j) {
			table.seekg(0x10 + (j * 0x60));
			TableEntry entry;
			entry.name.clear();
			{
				char c = 0;
				do {
					entry.name += c;
				} while (c && entry.name.size() < 0x5C);
				entry.name += '\000';
			}
			table.seekg(0x6C + (j * 0x60));
			table.read(reinterpret_cast<char*>(&entry.hash), sizeof(uint32_t));
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
				unknown_hashes.emplace_back(hq2x_hashes[j]);
			}
		}

		delete[] hq2x_hashes;

		std::ifstream exe_file(exe_path, std::ios::binary);
		if (!exe_file) {
			std::cerr << "[Error] Could not open " << exe_path << ".\n";
			BetterCallPause();
			std::exit(-1);
		}

		//exe_file.seekg(0x3C);
		//uint32_t PE_offset = 0;
		//exe_file.read(reinterpret_cast<char*>(&PE_offset), sizeof(uint32_t));
		exe_file.seekg(0x200);
		{
			uint64_t val = 0;//.rdata\0\0  \0\000at adr.
			while (val != (0x61746164722E)) {
				exe_file.read(reinterpret_cast<char*>(&val), sizeof(uint64_t));
			}
		}
		std::printf("Found .rdata section header at %X\n", int(exe_file.tellg()) - 8);
		exe_file.seekg(8, std::ios::cur);
		uint32_t rdata_size = 0, rdata_offset = 0;
		exe_file.read(reinterpret_cast<char*>(&rdata_size), sizeof(uint32_t));
		exe_file.read(reinterpret_cast<char*>(&rdata_offset), sizeof(uint32_t));
		std::printf("rdata_size: %X\nrdata_offset: %X\n", rdata_size, rdata_offset);
			
		exe_file.seekg(rdata_offset);
		std::u32string str;
		int str_counter = 0;
		while (exe_file.tellg() < rdata_offset + rdata_size) {
			char32_t c;
			exe_file.read(reinterpret_cast<char*>(&c), 4);
			if (c > 0x1F && c < 0x0000007F) {//if ANSI character
				str += c;
			}
			else if (c == 0 && !str.empty()) {//if NULL byte and there were characters before
				std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
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
				else if 
					(str.find(U"vertex") != std::u32string::npos || str.find(U"pixel") != std::u32string::npos) {
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

					unsigned char* ansi_str = new unsigned char[converter.to_bytes(str).size() + 1] {0};
					std::string ansi_str_c = converter.to_bytes(str).c_str();
					strcpy_s(reinterpret_cast<char*>(ansi_str), converter.to_bytes(str).size()+1, ansi_str_c.c_str());
					uint32_t test_hash = djb2_hash(ansi_str);

					for (int i = 0; i < unknown_hashes.size(); ++i) {
						if (unknown_hashes[i] == test_hash) {
							std::printf("\tHASH MATCH: %X %s\n", test_hash, converter.to_bytes(str).c_str());
							table_entries.push_back(TableEntry{ converter.to_bytes(str), test_hash });
							break;
						}
					}

					delete[] ansi_str;
				}

				str.clear();
			}
			else if (!str.empty()) {//if not a character
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
			std::exit(-1);
		}
		for (int i = t_famount; i < table_entries.size(); ++i) {
			table.seekp(0x10 + (i * 0x60));
			table.write(table_entries[i].name.c_str(), table_entries[i].name.size());
			table.seekp(0x6C + (i * 0x60));
			table.write(reinterpret_cast<char*>(&table_entries[i].hash), sizeof(uint32_t));
		}
		table.seekp(0xC);
		uint32_t new_t_size = table_entries.size();
		table.write(reinterpret_cast<char*>(&new_t_size), sizeof(uint32_t));
		table.close();
		std::printf("The table has been saved.\n");
			//table.seekg(-0x60, ios::end);
			//table.write(name.c_str(), name.size());
			//table.seekg(0xC);
			//table.read(reinterpret_cast<char*>(&t_famount), sizeof(t_famount));
			//t_famount++;
			//table.seekg(0xC);
			//table.write(reinterpret_cast<char*>(&t_famount), sizeof(t_famount));//write down new famount to the table
			//table.close();

	}
#ifndef IAMT_AUDIO_ONLY
	else if (mode == 't')
	{
		string table_path;
		string name;
		uint64_t format_check;
		uint64_t file_size;
		uint64_t i = 0;//yews
		uint32_t famount;
		uint32_t crc;
		uint32_t version;
		fstream ciu_table;
		std::ostringstream offset_display;
		std::ostringstream number_display;

		cout << "Table info mode.\nChose a table file.\n";
		table_path = OpenFileWindow("IAMT Table Format (.iamt_table)\0*.IAMT_TABLE\0All Files\0*.*\0");
		ciu_table.open(table_path, ios::in | ios::binary);
		if (!ciu_table)
		{
			cerr << "[Error] Can't open the file. Try running the tool as administrator.\n";
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
			exit(1);
		}
		ciu_table.read(reinterpret_cast<char*>(&format_check), sizeof(format_check));
		ciu_table.read(reinterpret_cast<char*>(&version), sizeof(version));
		ciu_table.read(reinterpret_cast<char*>(&famount), sizeof(famount));
		ciu_table.seekg(0, ios::end);
		file_size = ciu_table.tellg();
		cout << "\nFile size: " << int_to_str(file_size, 4) << '\n';

		i = (file_size - 0x10) / 0x60;
		cout << "Table version: " << version << "\nFiles amount: " << famount << '\n';
		if (file_size != ((uint64_t(famount) * 0x60) + 0x10) || famount != i)
		{
			cout << "Table size is incorrect - files amount: " << famount << ", should be: " << i << '\n';
		}
		//ciu_table.clear();
		if (format_check != 0x55494320544D4149)//IAMT CIU
		{
			if (format_check == 0x55494320544D4943)//CIMT CIU
				cout << "This table is in the old CIMT format.\nChange the name to .iamt_table and 2 first bytes to \"IA\" using HxD to make it work with IAMT v3.0 and higher.\n\n";
			else if (format_check == 0x55494320494D4149)//IAMI CIU
			{
				cout << "This table is encrypted and used with InterAction Mod Installer. Get IAMT table to use it with Mod Tool.\n";
				cout << "Press Enter to continue...";
				cin.get();
				//cin.ignore();
				exit(0);
			}
			else
			{
				cout << "No known table formats found in the file.\n";
				cout << "Press Enter to continue...";
				cin.get();
				//cin.ignore();
				exit(0);
			}
		}
		char c;
		int temp_off;

		cout << "   Num. [CRC       ] Path\n";
		for (uint64_t j = 0; j < i; j++)
		{
			ciu_table.clear();
			ciu_table.seekg(0x10 + 0x60 * j + 0x5C);
			ciu_table.read(reinterpret_cast<char*>(&crc), sizeof(crc));
			name.clear();
			c = 0xFF;//(3232,8): warning C4309: =: усечение константного значения. Gone after restart.
			temp_off = 0;
			if (ciu_table.eof())
			{
				name = "CORRUPTED ENTRY";
				crc = 0xbaaaaaad;
			}
			while (uint8_t(c) != 0x00 && !ciu_table.eof())
			{
				ciu_table.seekg(0x10 + 0x60 * j + uint64_t(temp_off));
				temp_off++;
				c = ciu_table.get();
				if (uint8_t(c) != 0x00)
					name += c;
			}
			offset_display << "[" << int_to_str(crc, 4) << "]";
			number_display << j + 1 << ".";
			cout << setw(7) << right << number_display.str() << " " << setw(12) << left << offset_display.str() << " " << name << '\n';
			offset_display.str("");
			number_display.str("");
		}
	}
	else if (mode == 'T')
	{
		string table_path;
		string name;
		uint64_t format_check;
		uint64_t file_size;
		uint64_t i = 0;//yews
		uint32_t famount;
		uint32_t crc;
		uint32_t version;
		fstream ciu_table;
		fstream temp_file;
		ofstream new_file;
		std::ostringstream offset_display;
		std::ostringstream number_display;

		cout << "Table edit mode.\nChose a table file.\n";
		table_path = OpenFileWindow("IAMT Table Format (.iamt_table)\0*.IAMT_TABLE\0All Files\0*.*\0");
		ciu_table.open(table_path, ios::in | ios::binary);
		if (!ciu_table)
		{
			cerr << "[Error] Can't open the file. Try running the tool as administrator.\n";
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
			exit(1);
		}
		temp_file.open(table_path + ".tmp", ios::out | ios::binary);
		if (!temp_file)
		{
			cerr << "[Error] Can't create the temp file.\n";
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
			exit(1);
		}
		ciu_table.read(reinterpret_cast<char*>(&format_check), sizeof(format_check));
		ciu_table.read(reinterpret_cast<char*>(&version), sizeof(version));
		ciu_table.read(reinterpret_cast<char*>(&famount), sizeof(famount));
		ciu_table.seekg(0, ios::end);
		file_size = ciu_table.tellg();
		cout << "\nFile size: " << int_to_str(file_size, 4) << '\n';

		i = (file_size - 0x10) / 0x60;
		cout << "Table version: " << version << "\nFiles amount: " << famount << '\n';
		if (file_size != ((uint64_t(famount) * 0x60) + 0x10) || famount != i)
		{
			cout << "Table size is incorrect - files amount: " << famount << ", should be: " << i << '\n';
		}
		ciu_table.clear();
		if (format_check != 0x55494320544D4149)//IAMT CIU
		{
			if (format_check == 0x55494320544D4943)//CIMT CIU
				cout << "This table is in the old CIMT format.\nChange the name to .iamt_table and 2 first bytes to \"IA\" using HxD to make it work with IAMT v3.0 and higher.\n\n";
			else if (format_check == 0x55494320494D4149)//IAMI CIU
			{
				cout << "This table is encrypted and used with InterAction Mod Installer. Get IAMT table to use it with Mod Tool.\n";
				cout << "Press Enter to continue...";
				cin.get();
				//cin.ignore();
				exit(0);
			}
			else
			{
				cout << "No known table formats found in the file.\n";
				cout << "Press Enter to continue...";
				cin.get();
				//cin.ignore();
				exit(0);
			}
		}
		char c;
		int temp_off;

		ciu_table.seekg(0, ios::end);
		uint64_t buff_size = ciu_table.tellg();
		char* buffer = new char[buff_size];
		ciu_table.seekg(0, ios::beg);
		ciu_table.read(buffer, buff_size);
		temp_file.write(buffer, buff_size);
		delete[] buffer;
		temp_file.close();
		temp_file.open(table_path + ".tmp", ios::in | ios::out | ios::binary);
		if (!temp_file)
		{
			cerr << "[Error] Can't create the temp file.\n";
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
			exit(1);
		}
		cout << "   Num. [CRC       ] Path\n";
		for (uint64_t j = 0; j < i; j++)
		{
			ciu_table.clear();
			ciu_table.seekg(0x10 + 0x60 * j + 0x5C);
			ciu_table.read(reinterpret_cast<char*>(&crc), sizeof(crc));
			name.clear();
			c = 0xFF;//(3232,8): warning C4309: =: усечение константного значения. Gone after restart.
			temp_off = 0;
			if (ciu_table.eof())
			{
				name = "CORRUPTED ENTRY";
				crc = 0xbaaaaaad;
			}
			while (uint8_t(c) != 0x00 && !ciu_table.eof())
			{
				ciu_table.seekg(0x10 + 0x60 * j + uint64_t(temp_off));
				temp_off++;
				c = ciu_table.get();
				if (uint8_t(c) != 0x00)
					name += c;
			}
			offset_display << "[" << int_to_str(crc, 4) << "]";
			number_display << j + 1 << ".";
			cout << setw(7) << right << number_display.str() << " " << setw(12) << left << offset_display.str() << " " << name << '\n';
			offset_display.str("");
			number_display.str("");
		}
		uint32_t edit_goto;
		uint32_t temp_crc;
		uint32_t temp_ver = 0;
		uint32_t temp_fsize;
		uint32_t temp_famount = 0;
		string temp_name;
		string inpt;
		string save_path;
		save_path.clear();
		for (;;)
		{
			cout << "Enter the command (type \"help\" to see all of them): ";
			getline(cin, inpt);
			if (inpt == "")
			{
				cout << "\n\nTyping finished. Deleting temp file.\n";
				temp_file.close();
				remove((table_path + ".tmp").c_str());
				break;
			}
			else if (inpt.find("help") != string::npos && inpt.find("help") < 5)
			{
				cout << "\nCommands:\n"
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
			else if (inpt.find("edit") != string::npos && inpt.find("edit") < 4)//edit X CRC NAME
			{
				vector<string> arguments = SeparateEx(inpt, ' ', 4);
				if (arguments.size() < 4)
				{
					cout << "Not enough data!\n";
					continue;
				}

				//read X and CRC
				try
				{
					edit_goto = stoi(arguments[1]);//X
				}
				catch (const std::invalid_argument&)
				{
					cout << "Can't find proper entry number!\n";
					continue;
				}

				try
				{
					temp_crc = stoul(arguments[2], nullptr, 16);//CRC
				}
				catch (const std::invalid_argument&)
				{
					cout << "Can't find proper CRC!\n";
					continue;
				}
				temp_name = inpt.substr(inpt.find(arguments[3], 5 + arguments[1].size() + arguments[2].size()));
				while (temp_name.size() < 0x5C)
					temp_name += '\0';

				if (edit_goto >= 1 && edit_goto <= i)
				{
					temp_file.seekp(0x10 + 0x60 * (uint64_t(edit_goto) - 1));
					if (temp_name.size() > 0x5C || temp_name[0x5B] != '\0')//cut if bigger
					{
						temp_name.erase(0x5B);
						temp_name += '\0';
					}
					temp_file.write(temp_name.c_str(), temp_name.size());
					temp_file.seekp(0x6C + 0x60 * (uint64_t(edit_goto) - 1));
					temp_file.write((char*)&temp_crc, sizeof(temp_crc));
					edit_goto = 0;
					continue;
				}
				else
				{
					cout << "Input value is incorrect: out of range.\n";
					continue;
				}
			}
			else if (inpt.find("save") != string::npos && inpt.find("save") < 4)
			{
				if (inpt.find("as") != string::npos)
				{
					cout << "Chose file path.\n";
					save_path = SaveFileWindow("IAMT Table format (.iamt_table)\0*.iamt_table\0All Files\0*.*\0", "iamt_table");
					cout << "Saving...\n";
					new_file.open(save_path, ios::binary);
					if (!new_file)
					{
						cerr << "[Error] Can't save the file.\n";
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					temp_file.seekg(0, ios::end);
					buff_size = temp_file.tellg();
					temp_file.seekg(0);
					char* buffer = new char[buff_size];
					temp_file.read(buffer, buff_size);
					new_file.write(buffer, buff_size);
					new_file.close();
					delete[] buffer;
					cout << "Saved.\n";
				}
				else
				{
					cout << "Saving...\n";
					ciu_table.close();
					new_file.open(table_path, ios::binary);
					if (!new_file)
					{
						cerr << "[Error] Can't save the file.\n";
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					temp_file.seekg(0, ios::end);
					buff_size = temp_file.tellg();
					temp_file.seekg(0);
					char* buffer = new char[buff_size];
					temp_file.read(buffer, buff_size);
					new_file.write(buffer, buff_size);
					new_file.close();
					delete[] buffer;
					cout << "Saved.\n";
					ciu_table.open(table_path, ios::in | ios::binary);
					if (!ciu_table)
					{
						cerr << "[Error] Can't open the file. Try running the tool as administrator.\n";
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
				}
			}
			else if (inpt.find("version") != string::npos && inpt.find("version") < 4)
			{
				try
				{
					temp_ver = stoul(inpt.substr(inpt.find("n") + 1));
				}
				catch (const std::invalid_argument&)
				{
					cout << "Can't find proper version number!\n";
					continue;
				}
				temp_file.seekp(0x8);
				temp_file.write((char*)&temp_ver, sizeof(temp_ver));
			}
			else if (inpt.find("files") != string::npos && inpt.find("files") < 4)
			{
				try
				{
					temp_famount = stoul(inpt.substr(inpt.find("s") + 1));
				}
				catch (const std::invalid_argument&)
				{
					cout << "Can't find proper amount number!\n";
					continue;
				}
				temp_file.seekp(0xC);
				temp_file.write((char*)&temp_famount, sizeof(temp_famount));
			}
			else if (inpt.find("add") != string::npos && inpt.find("add") < 4)//add CRC NAME
			{
				vector<string> arguments = SeparateEx(inpt, ' ', 3);
				if (arguments.size() < 3)
				{
					cout << "Not enough data!\n";
					continue;
				}

				try
				{
					temp_crc = stoul(arguments[1], nullptr, 16);//CRC
				}
				catch (const std::invalid_argument&)
				{
					cout << "Can't find proper CRC!\n";
					continue;
				}
				temp_name = inpt.substr(inpt.find(arguments[2], 4 + arguments[1].size()));
				while (temp_name.size() < 0x5C)
					temp_name += '\0';

				temp_file.seekp(0, ios::end);
				if (temp_name.size() > 0x5C || temp_name[0x5B] != '\0')//cut if bigger
				{
					temp_name.erase(0x5B);
					temp_name += '\0';
				}
				temp_file.write(temp_name.c_str(), temp_name.size());
				temp_file.write((char*)&temp_crc, sizeof(temp_crc));
				i++;
				continue;
			}
			else if (inpt.find("remove") != string::npos && inpt.find("remove") < 4)
			{
				try
				{
					edit_goto = stoul(inpt.substr(inpt.find("ve") + 2));
				}
				catch (const std::invalid_argument&)
				{
					cout << "Can't find proper entry number!\n";
					continue;
				}
				temp_file.seekg(0, ios::end);
				temp_fsize = temp_file.tellg();
				char* buffer = new char[temp_fsize - (0x10 + 0x60 * (edit_goto - 1))];
				temp_file.seekg(0x10 + 0x60 * uint64_t(edit_goto));
				temp_file.read(buffer, temp_fsize - (0x10 + 0x60 * (edit_goto - 1)));
				temp_file.seekp(0x10 + 0x60 * (uint64_t(edit_goto) - 1));
				temp_file.write(buffer, temp_fsize - (0x10 + 0x60 * (edit_goto - 1)));
				temp_file.clear();
				delete[] buffer;
				std::filesystem::resize_file(table_path + ".tmp", temp_fsize - 0x60);
			}
			else if (inpt.find("fix") != string::npos && inpt.find("fix") < 4)
			{
				uint64_t j2 = 0;
				temp_file.clear();
				temp_file.seekg(0);
				c = 0xFF;
				while (!temp_file.eof())
				{
					temp_file.seekg(0x10 + 0x60 * j2);
					j2++;
					c = temp_file.get();
				}
				cout << "Found end of file. Erasing last entry.\n";
				temp_file.clear();
				std::filesystem::resize_file(table_path + ".tmp", 0x10 + 0x60 * (j2 - 2));
			}
			else if (inpt.find("show") != string::npos && inpt.find("show") < 4)
			{
				cout << "\n\nNOTE: This info contains changes you made. If you want to see the file itself, restart the tool.\n";
				temp_file.seekg(0);
				temp_file.read(reinterpret_cast<char*>(&format_check), sizeof(format_check));
				temp_file.read(reinterpret_cast<char*>(&version), sizeof(version));
				temp_file.read(reinterpret_cast<char*>(&famount), sizeof(famount));
				temp_file.seekg(0, ios::end);
				file_size = temp_file.tellg();
				cout << "File size: " << int_to_str(file_size, 4) << '\n';

				while (0x10 + (0x60 * i) < file_size)
				{
					temp_file.seekg(0x10 + (0x60 * i));
					i++;
				}
				cout << "Table version: " << version << "\nFiles amount: " << famount << '\n';
				if (file_size != ((uint64_t(famount) * 0x60) + 0x10) || famount != i)
				{
					cout << "Table size is incorrect - files amount: " << famount << ", should be: " << i << '\n';
				}
				temp_file.clear();
				if (format_check != 0x55494320544D4149)//IAMT CIU
				{
					if (format_check == 0x55494320544D4943)//CIMT CIU
						cout << "This table is in the old CIMT format.\nChange the name to .iamt_table and 2 first bytes to \"IA\" using HxD to make it work with IAMT v3.0 and higher.\n\n";
					else if (format_check == 0x55494320494D4149)//IAMI CIU
					{
						cout << "This table is encrypted and used with InterAction Mod Installer. Get IAMT table to use it with Mod Tool.\n";
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						continue;
					}
					else
					{
						cout << "No known table formats found in the file. Looks like the table got corrupted by changes you did. If this happened, send info to us.\n";
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						continue;
					}
				}
				cout << "   Num. [CRC       ] Path\n";
				for (uint64_t j = 0; j < i; j++)
				{
					temp_file.clear();
					temp_file.seekg(0x10 + 0x60 * j + 0x5C);
					temp_file.read(reinterpret_cast<char*>(&crc), sizeof(crc));
					name.clear();
					c = 0xFF;
					temp_off = 0;
					if (temp_file.eof())
					{
						name = "CORRUPTED ENTRY";
						crc = 0xbaaaaaad;
					}
					while (uint8_t(c) != 0x00 && !temp_file.eof())
					{
						temp_file.seekg(0x10 + 0x60 * j + uint64_t(temp_off));
						temp_off++;
						c = temp_file.get();
						if (uint8_t(c) != 0x00)
							name += c;
					}
					offset_display << "[" << int_to_str(crc, 4) << "]";
					number_display << j + 1 << ".";
					cout << setw(7) << right << number_display.str() << " " << setw(12) << left << offset_display.str() << " " << name << '\n';
					offset_display.str("");
					number_display.str("");
				}
			}
		}
	}
#endif
	else if (mode == 'i')
	{
		srand(static_cast<unsigned int>(time(0)));
		fstream ciu_table;
		fstream iami;
		string table_path;
		string iami_path;
		uint32_t famount;
		char c = 0xFF;
		char q = 0xFF;
		int end = -1;
		cout << "Table convert mode.\nChoose a CIU.iamt_table.\n";
		table_path = OpenFileWindow("IAMT Table format (*.iamt_table)\0*.iamt_table\0All Files\0*.*\0");
		cout << "Choose save path.\n";
		iami_path = SaveFileWindow("IAMI format (*.iami)\0*.iami\0All Files\0*.*\0", "iami");
		ciu_table.open(table_path, ios::in | ios::binary);
		if (!ciu_table)
		{
			cerr << "[Error] Can't open .iamt_table!\n";
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
			exit(1);
		}
		ciu_table.seekg(0, ios::end);
		uint64_t buf_size = ciu_table.tellg();
		char* buf = new char[buf_size];
		ciu_table.seekg(0);
		ciu_table.read(buf, buf_size);
		ciu_table.close();
		//buf[3] = char(0x49);
		iami.open(iami_path, ios::out | ios::binary);
		if (!iami)
		{
			cerr << "[Error] Can't create .iami!\n";
			delete[] buf;
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
			exit(1);
		}
		iami.write(buf, buf_size);
		delete[] buf;
		iami.close();
		iami.open(iami_path, ios::in | ios::out | ios::binary);
		iami.seekp(3);
		iami.put(char(0x49));
		iami.seekg(0xC);
		iami.read(reinterpret_cast<char*>(&famount), sizeof(famount));
		cout << "Making cancer...\n";
		for (uint64_t i = 0; i < famount; i++)
		{
			c = 0xFF;
			end = -1;
			for (uint64_t j = 0; j < 0x5C; j++)
			{
				iami.seekg(0x10 + 0x60 * i + j);
				c = iami.get();
				if (uint8_t(c) == 0x00)
				{
					if (end == -1)
					{
						end = j;
						continue;
					}
					else if (j > end)
					{
						q = rand() % 255 + 1;
						iami.seekp(0x10 + 0x60 * i + j);
						iami.put(q);
					}
				}
			}
		}
		cout << "Making hardcoded cancer...\n";
		iami.seekg(0, ios::end);
		buf_size = iami.tellg();
		buf = new char[buf_size];
		iami.seekg(0);
		iami.read(buf, buf_size);
		protection(buf, buf_size, 'e');
		iami.seekp(0);
		iami.write(buf, buf_size);
		iami.close();
		delete[] buf;
		cout << "Regular cancer and hardcoded cancer successfully applied!\n";
	}
	else if (mode == 'M')
	{
		cout << "\nMod edit mode. Choose mod file.\n";
		string file_path;
		string name;
		string mod_name;
		string mod_description;
		string save_path;
		string inpt;
		char mod_game_code[4];
		uint32_t format_check;
		uint32_t crc;
		uint32_t mod_version;
		uint32_t mod_files_amount;
		uint32_t full_size;
		char* buffer;
		fstream mod;
		fstream temp_file;
		ofstream new_file;

		file_path = OpenFileWindow("IAMI CIU Mod (*.CIUMod)\0*.CIUMod\0"
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
		mod.open(file_path, ios::in | ios::binary);
		if (!mod)
		{
			cerr << "[Error] Can't open the file. Try running the tool as administrator.\n";
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
			exit(1);
		}
		mod.seekg(4);
		mod.read(reinterpret_cast<char*>(&format_check), sizeof(uint32_t));
		if (format_check != 0x6799186F)//****IAMF
		{
			cerr << "[Error] Bruh this file is not a mod ._ .\nNow go crash\n";
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
			exit(1);
		}
		temp_file.open(file_path + ".tmp", ios::trunc | ios::out | ios::in | ios::binary);
		if (!temp_file)
		{
			cerr << "[Error] Can't create the temp file.\n";
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
			exit(1);
		}
		mod.seekg(0, ios::end);
		uint64_t buffer_size = mod.tellg();
		buffer = new char[buffer_size];
		mod.seekg(0);
		mod.read(buffer, buffer_size);
		mod.close();
		protection(buffer, buffer_size, 'd');//decode all shit to reduce cancer
		temp_file.write(buffer, buffer_size);
		temp_file.seekg(0);
		delete[] buffer;

		temp_file.read(mod_game_code, sizeof(uint32_t));
		temp_file.seekg(8);
		temp_file.read(reinterpret_cast<char*>(&mod_version), sizeof(uint32_t));
		temp_file.read(reinterpret_cast<char*>(&mod_files_amount), sizeof(uint32_t));

		buffer = new char[0x50];
		temp_file.seekg(0x10);
		temp_file.read(buffer, 0x50);
		mod_name = buffer;
		delete[] buffer;

		buffer = new char[0x100];
		temp_file.seekg(0x60);
		temp_file.read(buffer, 0x100);
		mod_description = buffer;
		delete[] buffer;

		cout << "\n\n[Mod information]\n"
			"Name: " << mod_name <<
			"\nVersion: " << mod_version <<
			"\n\n[Description]\n" << mod_description <<
			"\n\n";
		for (;;)
		{
			cout << "Enter the command (type \"help\" to see all of them): ";
			getline(cin, inpt);
			if (inpt == "")
			{
				cout << "\n\nTyping finished. Deleting temp file.\n";
				temp_file.close();
				remove((file_path + ".tmp").c_str());
				break;
			}
			else if (inpt.find("help") != string::npos && inpt.find("help") < 5)
			{
				cout << "\nCommands:\n"
					"version X       -> Change the version of the mod to X (must be >= 0 and <= 4 294 967 295).\n"
					"name X          -> Change mod name to X (name assasin\'s creed (what))"
					"description X   -> Change description to X (description mod which adds banana peal)"
					"save            -> Saves edited table at the same place.\n"
					"save as, saveas -> Saves edited table wherever you want.\n"
					"help            -> Show this screen.\n"
					"[Empty line]    -> Finish typing and exit from the tool.\n\n";
				continue;
			}
			else if (inpt.find("save") != string::npos && inpt.find("save") < 4)
			{
				if (inpt.find("as") != string::npos)
				{
					cout << "Chose file path.\n";
					save_path = SaveFileWindow("IAMI CIU Mod (*.CIUMod)\0*.CIUMod\0"
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
					cout << "Saving...\n";
					new_file.open(save_path, ios::trunc | ios::binary);
					if (!new_file)
					{
						cerr << "[Error] Can't save the file.\n";
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					//CRC and Hardcode
					//Calculate CRC
					temp_file.seekg(-4, ios::end);
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
					temp_file.seekg(0, ios::end);
					fsize = temp_file.tellg();
					temp_file.seekg(0);
					buffer = new char[fsize];
					temp_file.read(buffer, fsize);
					protection(buffer, fsize, 'e');

					new_file.write(buffer, buffer_size);
					new_file.close();
					delete[] buffer;
					cout << "Saved.\n";
				}
				else
				{
					cout << "Saving...\n";
					new_file.open(file_path, ios::trunc | ios::binary);
					if (!new_file)
					{
						cerr << "[Error] Can't save the file.\n";
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					//CRC and Hardcode
					//Calculate CRC
					temp_file.seekg(-4, ios::end);
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
					temp_file.seekg(0, ios::end);
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
			else if (inpt.find("version") != string::npos && inpt.find("version") < 4)
			{
				uint32_t temp_ver;
				try
				{
					temp_ver = stoul(inpt.substr(inpt.find("n") + 1)); // uint32_t was duplicated -- mish, 09.11.2026
				}
				catch (const std::invalid_argument&)
				{
					cout << "Can't find proper version number!\n";
					continue;
				}
				temp_file.seekp(0x8);
				temp_file.write((char*)&temp_ver, sizeof(uint32_t));
			}
			else if (inpt.find("name") != string::npos && inpt.find("name") < 4)
			{
				string temp_name = inpt.substr(inpt.find("name") + 5);
				while (temp_name.size() < 0x50)
					temp_name += '\0';
				if (temp_name.size() > 0x50)
				{
					temp_name.erase(0x4F);
					temp_name += '\0';
				}
				temp_file.seekp(0x10);
				temp_file.write(temp_name.c_str(), 0x50);
			}
			else if (inpt.find("description") != string::npos && inpt.find("description") < 4)
			{
				string temp_name = inpt.substr(inpt.find("description") + 12);
				while (temp_name.find("\\n") != string::npos)
				{
					temp_name[temp_name.find("\\n")] = '\n';
					temp_name.erase(temp_name.rfind('\n') + 1, 1);
				}
				while (temp_name.size() < 0x100)
					temp_name += '\0';
				if (temp_name.size() > 0x100)
				{
					temp_name.erase(0xFF);
					temp_name += '\0';
				}
				temp_file.seekp(0x60);
				temp_file.write(temp_name.c_str(), 0x100);
			}
		}
	}
	else if (mode == 'm')
	{
		cout << "Mod creation mode.\n";
		/*
		0		- tool path
		1		- "-mc"
		2		- output mod file
		3+(2x)	- paths
		4+(2x)	- files

		make this so you can build mod file out of repack command easily by replacing hq2x with output file and -r with -mc
		*/
		command += " -mc ";
		vector<string> files;
		string mod_file_path;//output
		string game_code;//CIU\0 to ciu
		string mod_name;
		string mod_description;
		uint32_t edit_goto;
		uint32_t mod_version = 0;
		uint32_t famount = 0;//Calculate AFTER doing everything else, to be sure
		uint32_t header_size;//Actually, it's all files size at once, I'm just too lazy to rewrite this
		uint32_t foffset;
		uint32_t fsize;
		uint32_t CRC;
		int end;//for mess
		fstream mod_file;
		fstream t_file;
		fstream temp_files;
		char* buffer;
		char c;//for mess
		char q;//for mess

		//Read files and paths and put them in the vector
		if (wait_for_files == false)
		{
			for (int i = 3; i < argc; i++)
				files.push_back(string(argv[i]));//0, 2 - path; 1, 3 - file
			mod_file_path = argv[2];
		}
		else
		{

			int counter = 1;
			string inpt;
			if (argc < 3)
			{
				cout << "Enter output file path:\n";
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
			else
			{
				mod_file_path = argv[2];
			}
			cout << "\nEnter the file paths you want to work with. Put an empty line when you finish.\n(Click RMB to paste path from buffer)\n\n"
				"\"undo\" - re-enter last pair.\n\"edit X\" - re-enter Xth pair.\n\"txt\" - use text file instead (will clear the ones you wrote before)\n\"folder\" - use folder as the root of paths\n";
			for (;;)
			{
				cout << counter << ". Path in hq2x: ";
				getline(cin, inpt);
				while (inpt.find('\"') != string::npos) {
					inpt.erase(inpt.find('\"'), 1);//delete " symbols
				}
				if (inpt == "")
				{
					cout << "\n\nTyping finished.\n";
					break;
				}
				else if (inpt == "undo")
				{
					if (files.size() >= 2)
					{
						files.pop_back();
						files.pop_back();
						counter--;
						continue;
					}
					else {
						cout << "You've reached the beginning!\n";
						continue;
					}
				}
				else if (inpt.find("edit") == 0 && inpt.size() > 4 && inpt[4] == ' ')
				{
					try
					{
						edit_goto = stoi(inpt.substr(4));
					}
					catch (const std::invalid_argument&)
					{
						cout << "Can't find proper number in the line!\n";
						continue;
					}
					//go edit the given pair of files
					if (edit_goto >= 1 && edit_goto < counter)
					{
						cout << edit_goto << ". Path in hq2x: ";
						getline(cin, inpt);
						while (inpt.find('\"') != string::npos)
						{
							inpt.erase(inpt.find('\"'), 1);//delete " symbols
						}
						edit_goto = (edit_goto - 1) * 2;//1 = [0, 1]
						files[edit_goto] = inpt;
						cout << " | Replace with: ";
						inpt = OpenFileWindow("All Files\0*.*\0Audio Files (*.ogg;*.wav;*.mp3)\0*.OGG;*.WAV;*.MP3\0"
#ifndef IAMT_AUDIO_ONLY
							"Graphics Files (*.tga; *.png; *.jpg; *.jpeg; *.bmp)\0*.TGA;*.PNG;*.JPG;*.JPEG;*.BMP\0Preload Files (*.ini;*.preload)\0*.INI;*.PRELOAD\0"
#endif
						);
						files[edit_goto + 1] = inpt;
						edit_goto = 0;
						continue;
					}
					else
					{
						cout << "Input value is incorrect.\n";
						continue;
					}
				}
				else if (inpt == "txt" || inpt == "TXT")
				{
					inpt = OpenFileWindow("Text File (*.txt)\0*.TXT\0All Files\0*.*\0");
					ifstream txtf(inpt);
					if (!txtf)
					{
						cout << "[Error] Can't open the file!\n";
						txtf.clear();
						continue;
					}
					files.clear();
					files = txt2vec(txtf);
					cout << "Done.\n";
					continue;
				}
				else if (inpt == "folder" || inpt == "FOLDER") {
					inpt = BrowseFolder(mod_file_path.substr(0, mod_file_path.rfind('\\')));
					if (!inpt.empty()) {
						files.clear();
						files = get_paths(inpt);
						cout << "Done.\n";
					}
					continue;
				}
				files.push_back(inpt);
				cout << " | Replace with: ";
				inpt = OpenFileWindow("All Files\0*.*\0Audio Files (*.ogg;*.wav;*.mp3)\0*.OGG;*.WAV;*.MP3\0"
#ifndef IAMT_AUDIO_ONLY
					"Graphics Files (*.tga; *.png; *.jpg; *.jpeg; *.bmp)\0*.TGA;*.PNG;*.JPG;*.JPEG;*.BMP\0"
#endif
				);
				files.push_back(inpt);
				counter++;
			}

		}
		//if SOMEHOW you entered one more file
		if (int(files.size()) % 2 != 0) {
			files.pop_back();
		}

		cout << "\nFinal list:\n";
		for (int i = 0; i < files.size(); i += 2) {
			if (gMakeTablelessMods) {
				char tempstr[10] {};
				sprintf_s(tempstr, "%lx", djb2_hash(reinterpret_cast<unsigned char*>(files[i].data())));
				files[i] = tempstr;
			}
			cout << "@ " << files[i] << " -> " << files[i + 1] << '\n';
		}
#if 0 // I'm not sure if this was ever useful so it's dead from now on -- mish, 09.11.2026
		for (int i = 0; i < bad_words.size(); i++)
		{
			if (mod_file_path.find(bad_words[i]) != string::npos)
			{
				cerr << "[Error] Can't create mod file because no\n";
				cout << "Press Enter to continue...";
				cin.get();
				//cin.ignore();
				exit(70);
			}
		}
#endif
		cout << "Select the game you want to create mod for:\n"
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
		getline(cin, game_code);
		//There is no switch () for string / "CIU\0" "CI5\0" "CI4\0" "CI3\0" "CI3O" "CI2\0" "CI2R" "CI1\0" "PIG\0" "SMVL" "UNI\0"
		if (game_code == "ciu")
#ifndef IAMT_NO_CIU
			game_code = "CIU\0";
#else
			game_code = "\0\0\0\0";
#endif

		else if (game_code == "ci5")
			game_code = "CI5\0";
		else if (game_code == "ci4")
			game_code = "CI4\0";
		else if (game_code == "ci3")
			game_code = "CI3\0";
		else if (game_code == "ci3o")
			game_code = "CI3O";
		else if (game_code == "ci2")
			game_code = "CI2\0";
		else if (game_code == "ci2r")
			game_code = "CI2R";
		else if (game_code == "ci1")
			game_code = "CI1\0";
		else if (game_code == "pi")
			game_code = "PIG\0";
		else if (game_code == "sv")
			game_code = "SMVL";
		else if (game_code == "uni")
			game_code = "UNI\0";


		while (mod_version == 0)
		{
			cout << "\nEnter mod version (integer 1-4\'294\'967\'295): ";
			getline(cin, mod_description);
			try
			{
				mod_version = stoul(mod_description);//X
			}
			catch (const std::invalid_argument&)
			{
				cout << "Can't find proper version number!\n";
				continue;
			}
		}

		cout << "Enter mod name (Limit - 80 symbols): ";
		getline(cin, mod_name);
		while (mod_name.size() < 0x50)
			mod_name += '\0';
		if (mod_name.size() > 0x50)
			mod_name.erase(0x50);
#if 0
		for (int i = 0; i < bad_words.size(); i++)
		{
			if (mod_name.find(bad_words[i]) != string::npos)
			{
				cerr << "[Error] Can't create mod file because no\n";
				cout << "Press Enter to continue...";
				cin.get();
				//cin.ignore();
				exit(70);
			}
		}
#endif
		cout << "\nEnter mod description (Use \\n as a new line symbol).\nIn universal mod you must write compatible games here too.\nLimit - 256 symbols.\n>";
		getline(cin, mod_description);
		while (mod_description.find("\\n") != string::npos)
		{
			mod_description[mod_description.find("\\n")] = '\n';
			mod_description.erase(mod_description.rfind('\n') + 1, 1);
		}
		while (mod_description.size() < 0x100)
			mod_description += '\0';
		if (mod_description.size() > 0x100)
			mod_description.erase(0x100);
#if 0
		for (int i = 0; i < bad_words.size(); i++)
		{
			if (mod_description.find(bad_words[i]) != string::npos)
			{
				cerr << "[Error] Can't create mod file because no\n";
				cout << "Press Enter to continue...";
				cin.get();
				//cin.ignore();
				exit(70);
			}
		}
#endif
		for (;;)
		{
			string inpt;
			cout << "Do you want to start? (y/n): ";
			getline(cin, inpt);
			if (inpt == "y")
				break;
			if (inpt == "n")
			{
				cout << "\nCreation canceled.\n";
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
		//Make mess in description and name
		if (!g_DoNotEncryptMods) {
			srand(static_cast<unsigned int>(time(0)));
			c = 0xff;
			end = -1;
			for (uint64_t j = 0; j < 0x50; j++)
			{
				c = mod_name[j];
				if (uint8_t(c) == 0x00)
				{
					if (end == -1)
					{
						end = j;
						continue;
					}
					else if (j > end)
					{
						q = rand() % 255 + 1;
						mod_name[j] = q;
					}
				}
			}
			c = 0xff;
			end = -1;
			for (uint64_t j = 0; j < 0x100; j++)
			{
				c = mod_description[j];
				if (uint8_t(c) == 0x00)
				{
					if (end == -1)
					{
						end = j;
						continue;
					}
					else if (j > end)
					{
						q = rand() % 255 + 1;
						mod_description[j] = q;
					}
				}
			}
		}

		mod_file.open(mod_file_path, ios::trunc | ios::in | ios::out | ios::binary);
		if (!mod_file)
		{
			cerr << "[Error] Can't create mod file. Try running the tool as administrator.\n";
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
			exit(1);
		}
		temp_files.open(mod_file_path + ".iamtemp", ios::trunc | ios::in | ios::out | ios::binary);
		if (!temp_files)
		{
			cerr << "[Error] Can't create temp file. Try running the tool as administrator.\n";
			cout << "Press Enter to continue...";
			cin.get();
			//cin.ignore();
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
		for (uint32_t i = 0; i < files.size(); i += 2)//0-path, 1-file
		{
			famount++;
			command += '\"' + files[i] + "\" \"" + files[i + 1] + "\" ";
			string temp_path = files[i];
			while (temp_path.find('\\') != string::npos)
				temp_path[temp_path.find('\\')] = '/';//Change all \ to / 'causeyou dum if you write \ here
			while (temp_path.size() < 0x58)
				temp_path += '\0';
			temp_path.erase(0x57);
			temp_path += '\0';
			mod_file.seekp(0x160 + 0x60 * (uint64_t(famount) - 1));
			for (int fill = 0; fill < 0x60; fill++)
				mod_file.put('\0');
			mod_file.seekp(0x160 + 0x60 * (uint64_t(famount) - 1));
			mod_file.write(temp_path.c_str(), 0x58);
			cout << "Opening " << files[i + 1] << '\n';


			t_file.open(files[i + 1], ios::in | ios::binary);
			if (!t_file)
			{
				cerr << "[Error] Can't open the file " << files[i + 1] << ". Try running the tool as administrator.\n";
				for (;;)
				{
					string inpt;
					cout << "Do you want to skip this file? (y/n): ";
					getline(cin, inpt);
					if (inpt == "y")
						break;
					if (inpt == "n")
					{
						cout << "\nMod creation canceled.\n";
						cout << "Press Enter to continue...";
						cin.get();
						//cin.ignore();
						exit(1);
					}
					else
					{
						cout << "\r";
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
			t_file.seekg(0, ios::end);
			fsize = t_file.tellg();
			t_file.seekg(0);
			buffer = new char[fsize];
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
		header_size = temp_files.tellg();//Size of ALL files
		temp_files.seekg(0);
		buffer = new char[header_size];
		temp_files.read(buffer, header_size);
		mod_file.seekg(0x160 + 0x60 * uint64_t(famount));
		mod_file.write(buffer, header_size);
		delete[] buffer;
		temp_files.close();

		if (!g_KeepTempFiles) {
			cout << "Deleting temp file.\n";
			remove((mod_file_path + ".iamtemp").c_str());
		}
		else {
			std::cout << "Temp file is kept per config settings: " << mod_file_path << ".iamtemp" << std::endl;
		}

		//fix offsets
		cout << "Writing offsets.\n";
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
		for (uint64_t i = 0; i < famount; i++)
		{
			foffset = 0x160 + (famount * 0x60) + size_offset;
			mod_file.seekp(0x1B8 + (0x60 * i));
			mod_file.write((char*)&foffset, sizeof(foffset));
			mod_file.seekg(0x1BC + (0x60 * i));
			mod_file.read(reinterpret_cast<char*>(&fsize), sizeof(fsize));
			size_offset += fsize;
	}

		//Make mess
		if (!g_DoNotEncryptMods) {
			cout << "Making mess.\n";
			for (uint64_t i = 0; i < famount; i++)
			{
				c = 0xFF;
				end = -1;
				for (uint64_t j = 0; j < 0x58; j++)
				{
					mod_file.seekg(0x160 + 0x60 * i + j);
					c = mod_file.get();
					if (uint8_t(c) == 0x00)
					{
						if (end == -1)
						{
							end = j;
							continue;
						}
						else if (j > end)
						{
							q = rand() % 255 + 1;
							mod_file.seekp(0x160 + 0x60 * i + j);
							mod_file.put(q);
						}
					}
				}
			}
		}
		//Calculate CRC
		mod_file.seekg(0, ios::end);
		fsize = mod_file.tellg();
		mod_file.seekg(0);
		buffer = new char[fsize];
		mod_file.read(buffer, fsize);
		CRC = CRC32(buffer, fsize);
		CRC += 0x93E56B02;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		delete[] buffer;
		mod_file.seekp(0, ios::end);
		mod_file.write((char*)&CRC, sizeof(CRC));

		//Hardcoded mess
		mod_file.seekg(0, ios::end);
		fsize = mod_file.tellg();
		mod_file.seekg(0);
		buffer = new char[fsize];
		mod_file.read(buffer, fsize);
		if (!g_DoNotEncryptMods) {
			protection(buffer, fsize, 'e');
		}
		mod_file.seekp(0);
		mod_file.write(buffer, fsize);
		delete[] buffer;
		mod_file.close();
}
	cout << "Press Enter to continue...";

	goto MODE_SELECTION;
	return 0;
}