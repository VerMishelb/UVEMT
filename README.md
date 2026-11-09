# UVEMT
Ultra VGA Engine Mod Tool.
The forever unfinished CLI version rewrite that was supposed to be done ages ago.

IAMT and IAMI code can be found in src with a "Legacy" suffix.
Anything below this line is NOT finished and will not work. This includes the CMakeLists found in Legacy folders,
although you can still compile the code if you fix it manually (the original project used SLN).

# Building
## Windows
Prerequesites: Visual Studio or MSVC Build tools, CMake 3.30+, git.
### Simple
1. Clone the source code:
```bash
git clone https://github.com/VerMishelb/UVEMT
cd ./UVEMT
```
2. Run `configure.cmd`.
3. Run `build.cmd`
The resulting executable will be in `./UVEMT/build/Release`. Edit `configure.cmd`, `build.cmd` and `install.cmd` with your values if necessary.
### Manual
NOTE: This section may change because I don't know yet whether I'll need simdutf or not.  
`.../UVEMT` is a directory with UVEMT source.  
`.../simdutf-src` is a directory with simdutf sources. `.../simdutf` is a directory with installed simdutf library.
1. Download, build and install a copy of simdutf library. This library is required for UTF-32 ↔ UTF-8 conversions as <codecvt> is deprecated and will be removed in C++26.
```bash
git clone https://github.com/simdutf/simdutf.git ".../simdutf-src"
cd simdutf-src
cmake -B build -S . -A Win32 # non visual studio generators can't use -A.
cmake --build build --config Release -j # use -D CMAKE_BUILD_TYPE=Release in previous step for non-multiconfig generators.
cmake --install build --config Release --prefix ".../simdutf" # Default is /usr/local for Linux, C:/Program Files [(x86)]/${PROJECT_NAME} for Windows.
```
Note that if you don't specify a `--prefix`, the library will be installed to `C:\Program Files (x86)\simdutf` which is likely undesirable.
2. Compile UVEMT
```bash
cmake -S . -B build -A Win32 -D simdutf_DIR="/simdutf"
cmake --build build --config Release -j
cmake --install build --config Release --prefix install
```

3. I have no idea whether this works because there's no compilable code yet.
## Linux
1. Clone the source code:
```bash
git clone https://github.com/VerMishelb/UVEMT
cd ./UVEMT
```
2. Run `configure.cmd`. If it cannot be run, allow execution with `chmod +x configure.cmd`.
3. Run `build.cmd`. The resulting executable will be in `./UVEMT/build/Release`. Edit `configure.cmd`, `build.cmd` and `install.cmd` with your values if necessary.
