#include "Modes.hpp"
#include <clocale>

#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
    #include <VersionHelpers.h>
void setUnicodeOutputIfSupported() {
    if (IsWindows10OrGreater()) {
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    }
}
#else
void consoleOutputSupportsUnicode() { return true; }
#endif

int main(int argc, char* argv[]) {
    std::setlocale(LC_ALL, ".UTF8"); // According to Internet legends must be called before
                                     // SetConsoleOutputCP, so here we are.
    setUnicodeOutputIfSupported();
    
    g_argc = argc;
    g_argv = argv;
    readIamtConfig();

    readArguments();
    modeSelection();

    return 0;
}