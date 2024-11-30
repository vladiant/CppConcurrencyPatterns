# Build instructions
```
mkdir build
cd build
conan install .. --build=missing
cd Release/
cmake ../.. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=./generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
make
```

