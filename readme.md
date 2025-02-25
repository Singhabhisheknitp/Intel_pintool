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



## Key Pin APIs and Data Structures

    PIN_Init: Initializes the Pin system
    PIN_StartProgram: Starts executing the program under Pin
    INS_AddInstrumentFunction: Registers a function to be called for each instruction
    PIN_AddFiniFunction: Registers a function to be called when the program exits
    KNOB: A template class for command-line options
    INS: Represents an instruction
    IPOINT: Specifies when to call analysis routines (before, after, etc.)
    ADDRINT: Address-sized integer type
    THREADID: Thread identifier

## General template for pin framework 
    In the below code, I have added my routines and instrumentation function in the general example for understanding. The code is source/tool/MyPinTool folder
 
 #include "pin.H"
#include <other_headers>

// Global variables and data structures

    // Analysis routines - called during program execution
    VOID MyAnalysisFunction(parameters) {
        // Analysis code here
    }
    VOID RecordMemRead(VOID* ip, VOID* addr, UINT32 size, THREADID tid)
    VOID RecordMemWrite(VOID* ip, VOID* addr, UINT32 size, THREADID tid)


    // Instrumentation routines - determine what to instrument
    VOID MyInstrumentationFunction(parameters, VOID* v) {
        // Instrumentation code here
    }

    VOID Instruction(INS ins, VOID* v)

    // Finalization routine - called when program exits
    VOID Fini(INT32 code, VOID *v) {
        // Cleanup and output results
    }

    // Main function
    int main(int argc, char *argv[]) {
        // Initialize PIN
        if (PIN_Init(argc, argv)) return Usage();
        
        // Register callback functions
        INS_AddInstrumentFunction(MyInstrumentationFunction, 0);
        PIN_AddFiniFunction(Fini, 0);
        
        // Start the program
        PIN_StartProgram();
        
        return 0;
    }


## Instrumentation routine 
    It is basically a function that is called at different scale/granularity and when called it basically calls the analysis routines(what we want to analyse, what sort of data we want to collect).  

    VOID Instruction(INS ins, VOID* v) {
    // Check if instruction performs memory operations
    if (INS_IsMemoryRead(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                      IARG_INST_PTR,
                      IARG_MEMORYREAD_EA,
                      IARG_MEMORYREAD_SIZE,
                      IARG_THREAD_ID,
                      IARG_END);
    }
    
    // Similarly for writes...
}

## Analysis routnes 
    These are function we write for the different kind of analysis we need during the function call.
    VOID RecordMemRead(VOID* ip, VOID* addr, UINT32 size, THREADID tid) {
        // Record and analyze memory reads
    }

    VOID RecordMemWrite(VOID* ip, VOID* addr, UINT32 size, THREADID tid) {
        // Record and analyze memory writes
    }
