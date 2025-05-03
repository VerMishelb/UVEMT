#include "Modes.h"

int nmain(int argc, char* argv[]) {
	g_argc = argc;
	g_argv = argv;
	readIamtConfig();
	
	readArguments();
	modeSelection();

	return 0;
}