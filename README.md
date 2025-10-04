# OpenMP_Mini1_Project

Minimal C++ project scaffold using CMake and clang++.

Structure:
- src/       : C++ source files
- interface/ : public headers for the project
- build/     : out-of-source build directory (created by CMake)
- data/      : sample or runtime data files

Quick build (macOS / zsh) using clang++:

```bash
mkdir -p build
cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build -- -j
```

Run:

```bash
./build/OpenMP_Mini1_Project_app
```

Notes:
- You can also set the compiler by exporting CXX or by passing a different path to `-DCMAKE_CXX_COMPILER`.
- The top-level `CMakeLists.txt` will try to prefer `clang++` if it can find it.
