# Intel pin binary instrumentation framework usage 

For details on building or basics check the pin-external-3.31-98869-gfa6f126a8-gcc-linux/readme.md

## How to use with your custom pintools

1. Write your pin tool script using pin.h. I added matrix_memory_analyzer.cpp.

2. Add the framework in the following folder for easy compilation with pin internal make file.
    /data/marg/abhishek/pinTool/pin-external-3.31-98869-gfa6f126a8-gcc-linux/source/tools/MyPinTool/matrix_memory_analyzer.cpp

3. modify the makefile.rules in the same folder to track the added new file during build process.

    TEST_TOOL_ROOTS := MyPinTool
    TEST_TOOL_ROOTS += matrix_memory_analyzer

4. go to the same folder and build the .so/.o file by make command

5. use the .so file now to trace the binary 

    ./pin -t source/tools/MyPinTool/obj-intel64/matrix_memory_analyzer.so -o mat_analysis.out -- ../matmul