#ifndef _Modes_h_
#define _Modes_h_

#include <string>

// Structures

struct TableEntry {
	std::string name;
	uint32_t hash = 0;
};

struct IamtConfig {
	bool fullBackup = true;
	bool lastBackup = false;
	bool makeTablelessMods = false;
	int loadingTimeout = 10;
	std::string toolFolder = "./";
};

// Globals

extern IamtConfig g_iamt_config;
extern int g_argc;
extern char** g_argv;

// Functions

void readIamtConfig();
void readArguments();
void modeSelection();

void modeCredits();
void modeDecode();
void modeEncode();
void modeHeaderInfo(bool wait_for_files = true);
void modeWadRepack(bool wait_for_files = true);
void modeWadExtract(bool wait_for_files = true);
void modeTableUpdate();
void modeTableInfo();
void modeTableEdit(bool wait_for_files = true);
void modeTableConvert();
void modeModCreate(bool wait_for_files = true);
void modeModEdit(bool wait_for_files = true);


#endif
