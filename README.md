# UVEMT
Ultra VGA Engine Mod Tool.
The forever unfinished CLI version rewrite that was supposed to be done ages ago.

# Building
## Windows
Requires CMake 3.30+. You may artificially lower it in the root CMakeLists but it's not guaranteed to work.
`.../simdutf-src` is a directory with simdutf sources. `.../simdutf` is a directory with installed simdutf library.
`.../UVEMT` is a directory with UVEMT source.
1. Download, build and install a copy of simdutf library. This library is required for UTF-32 ↔ UTF-8 conversions as <codecvt> is deprecated and will be removed in C++26.
```
git clone https://github.com/simdutf/simdutf.git ".../simdutf-src"
cd simdutf-src
cmake -B build -S . -A Win32 # non visual studio generators can't use -A.
cmake --build build --config Release -j # use -D CMAKE_BUILD_TYPE=Release in previous step for non-multiconfig generators.
cmake --install build --config Release --prefix ".../simdutf"
```
Note that if you don't specify a `--prefix`, the library will be installed to `C:\Program Files (x86)\simdutf` which is likely undesirable.
2. Compile UVEMT
```
cmake -S . -B build -A Win32 -D simdutf_DIR="/simdutf"
cmake --build build --config Release -j
cmake --install build --config Release --prefix install
```
Alternatively, you can edit `configure.bat`, `build.bat` and `install.bat` with your values.
3. I have no idea whether this works because there's no compilable code yet.