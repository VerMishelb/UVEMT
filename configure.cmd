: # This script is cross-platform: line 10 is a Windows command (VS solution), line 7 is a Linux command (Ninja multi-config)
:<<"::CMDLITERAL"
@ECHO OFF
GOTO :CMDSCRIPT
::CMDLITERAL

cmake -S . -B build -D CMAKE_BUILD_TYPE=Release -G "Ninja Multi-Config"

:CMDSCRIPT
cmake -S . -B build -D CMAKE_BUILD_TYPE=Release -A Win32

: # -D simdutf_DIR="E:\VSLibs\simdutf_install"
