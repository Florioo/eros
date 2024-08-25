cmake -B build -S .
cmake --build build --target run2


cmake -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/g++ --no-warn-unused-cli -S/root/dev/eros/test -B/root/dev/eros/build -G Ninja