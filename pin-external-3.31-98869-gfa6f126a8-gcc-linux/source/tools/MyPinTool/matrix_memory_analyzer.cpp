#include "pin.H"
#include <iostream>
#include <fstream>
#include <map>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iomanip>

// Output file stream
std::ofstream OutFile;

// Data structures for tracking memory accesses
struct MemAccess {
    ADDRINT addr;
    UINT32 size;
    UINT64 timestamp;
    bool isRead;
    THREADID tid;
};

std::vector<MemAccess> memAccesses;
std::unordered_map<THREADID, ADDRINT> lastAccessAddr;
std::unordered_map<ADDRINT, UINT64> lastAccessTime;
std::map<ADDRINT, UINT64> strideHistogram;

// Statistics
UINT64 totalAccesses = 0;
UINT64 totalReads = 0;
UINT64 totalWrites = 0;
UINT64 maxRecordedAccesses = 1000000;  // Limit to avoid excessive memory usage

// Command-line options
KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", 
    "o", "memtrace.out", "specify output file name");
KNOB<UINT64> KnobMaxAccesses(KNOB_MODE_WRITEONCE, "pintool", 
    "max", "1000000", "maximum number of memory accesses to record");

// Record a memory read
VOID RecordMemRead(VOID* ip, VOID* addr, UINT32 size, THREADID tid) {
    if (totalAccesses >= maxRecordedAccesses) return;
    
    ADDRINT address = reinterpret_cast<ADDRINT>(addr);
    
    // Record this access
    MemAccess access;
    access.addr = address;
    access.size = size;
    access.timestamp = totalAccesses;
    access.isRead = true;
    access.tid = tid;
    memAccesses.push_back(access);
    
    // Update statistics
    totalAccesses++;
    totalReads++;
    
    // Calculate stride if we have a previous access for this thread
    if (lastAccessAddr.count(tid) > 0) {
        ADDRINT lastAddr = lastAccessAddr[tid];
        ADDRINT stride = (address > lastAddr) ? (address - lastAddr) : (lastAddr - address);
        // Update the stride histogram
        strideHistogram[stride]++;
    }
    
    // Update last access information
    lastAccessAddr[tid] = address;
    lastAccessTime[address] = totalAccesses;
}

// Record a memory write
VOID RecordMemWrite(VOID* ip, VOID* addr, UINT32 size, THREADID tid) {
    if (totalAccesses >= maxRecordedAccesses) return;
    
    ADDRINT address = reinterpret_cast<ADDRINT>(addr);
    
    // Record this access
    MemAccess access;
    access.addr = address;
    access.size = size;
    access.timestamp = totalAccesses;
    access.isRead = false;
    access.tid = tid;
    memAccesses.push_back(access);
    
    // Update statistics
    totalAccesses++;
    totalWrites++;
    
    // Calculate stride if we have a previous access for this thread
    if (lastAccessAddr.count(tid) > 0) {
        ADDRINT lastAddr = lastAccessAddr[tid];
        ADDRINT stride = (address > lastAddr) ? (address - lastAddr) : (lastAddr - address);
        // Update the stride histogram
        strideHistogram[stride]++;
    }
    
    // Update last access information
    lastAccessAddr[tid] = address;
    lastAccessTime[address] = totalAccesses;
}

// Instrument each memory instruction
VOID Instruction(INS ins, VOID* v) {
    // Instrument memory reads
    if (INS_IsMemoryRead(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                       IARG_INST_PTR,
                       IARG_MEMORYREAD_EA,
                       IARG_MEMORYREAD_SIZE,
                       IARG_THREAD_ID,
                       IARG_END);
    }
    
    // Some instructions perform two memory reads
    if (INS_HasMemoryRead2(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                       IARG_INST_PTR,
                       IARG_MEMORYREAD2_EA,
                       IARG_MEMORYREAD_SIZE,
                       IARG_THREAD_ID,
                       IARG_END);
    }
    
    // Instrument memory writes
    if (INS_IsMemoryWrite(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                       IARG_INST_PTR,
                       IARG_MEMORYWRITE_EA,
                       IARG_MEMORYWRITE_SIZE,
                       IARG_THREAD_ID,
                       IARG_END);
    }
}

// Analyze spatial locality by examining address distances
VOID AnalyzeSpatialLocality() {
    OutFile << "\n==== SPATIAL LOCALITY ANALYSIS ====\n";
    
    // Sort accesses by address
    std::vector<MemAccess> sortedAccesses = memAccesses;
    std::sort(sortedAccesses.begin(), sortedAccesses.end(), 
              [](const MemAccess& a, const MemAccess& b) { return a.addr < b.addr; });
    
    // Calculate distances between consecutive accesses
    std::map<UINT64, UINT64> distanceHist;
    for (size_t i = 1; i < sortedAccesses.size(); i++) {
        UINT64 distance = sortedAccesses[i].addr - sortedAccesses[i-1].addr;
        if (distance < 4096) {  // Focus on smaller distances for clarity
            distanceHist[distance]++;
        }
    }
    
    // Print the most common distances
    std::vector<std::pair<UINT64, UINT64>> distances(distanceHist.begin(), distanceHist.end());
    std::sort(distances.begin(), distances.end(), 
              [](const std::pair<UINT64, UINT64>& a, const std::pair<UINT64, UINT64>& b) {
                  return a.second > b.second;
              });
    
    OutFile << "Most common address distances:\n";
    for (size_t i = 0; i < std::min(size_t(10), distances.size()); i++) {
        double percentage = 100.0 * distances[i].second / sortedAccesses.size();
        OutFile << std::setw(8) << distances[i].first << " bytes: " 
                << std::setw(10) << distances[i].second << " occurrences (" 
                << std::fixed << std::setprecision(2) << percentage << "%)\n";
    }
}

// Analyze temporal locality by examining reuse distances
VOID AnalyzeTemporalLocality() {
    OutFile << "\n==== TEMPORAL LOCALITY ANALYSIS ====\n";
    
    std::map<UINT64, UINT64> reuseDistHist;
    std::unordered_map<ADDRINT, UINT64> lastSeen;
    
    // Calculate reuse distances
    for (size_t i = 0; i < memAccesses.size(); i++) {
        ADDRINT addr = memAccesses[i].addr;
        
        if (lastSeen.count(addr) > 0) {
            UINT64 reuseDistance = i - lastSeen[addr];
            reuseDistHist[reuseDistance]++;
        }
        
        lastSeen[addr] = i;
    }
    
    // Print the most common reuse distances
    std::vector<std::pair<UINT64, UINT64>> distances(reuseDistHist.begin(), reuseDistHist.end());
    std::sort(distances.begin(), distances.end(), 
              [](const std::pair<UINT64, UINT64>& a, const std::pair<UINT64, UINT64>& b) {
                  return a.second > b.second;
              });
    
    OutFile << "Most common reuse distances (smaller is better for cache):\n";
    for (size_t i = 0; i < std::min(size_t(10), distances.size()); i++) {
        double percentage = 100.0 * distances[i].second / memAccesses.size();
        OutFile << std::setw(8) << distances[i].first << " accesses: " 
                << std::setw(10) << distances[i].second << " occurrences (" 
                << std::fixed << std::setprecision(2) << percentage << "%)\n";
    }
}

// Analyze memory access strides
VOID AnalyzeStrides() {
    OutFile << "\n==== STRIDE ANALYSIS ====\n";
    
    // Sort strides by frequency
    std::vector<std::pair<ADDRINT, UINT64>> strides(strideHistogram.begin(), strideHistogram.end());
    std::sort(strides.begin(), strides.end(), 
              [](const std::pair<ADDRINT, UINT64>& a, const std::pair<ADDRINT, UINT64>& b) {
                  return a.second > b.second;
              });
    
    // Count strides in different categories
    UINT64 smallStrides = 0;   // <= 64 bytes (typically cache-friendly)
    UINT64 mediumStrides = 0;  // 65-512 bytes
    UINT64 largeStrides = 0;   // > 512 bytes (typically cache-unfriendly)
    
    for (const auto& stride : strideHistogram) {
        if (stride.first <= 64) smallStrides += stride.second;
        else if (stride.first <= 512) mediumStrides += stride.second;
        else largeStrides += stride.second;
    }
    
    UINT64 totalStrides = smallStrides + mediumStrides + largeStrides;
    
    // Print stride categories
    OutFile << "Stride distribution:\n";
    if (totalStrides > 0) {
        OutFile << "  Small strides (<=64 bytes): " << smallStrides 
                << " (" << (100.0 * smallStrides / totalStrides) << "%)\n";
        OutFile << "  Medium strides (65-512 bytes): " << mediumStrides 
                << " (" << (100.0 * mediumStrides / totalStrides) << "%)\n";
        OutFile << "  Large strides (>512 bytes): " << largeStrides 
                << " (" << (100.0 * largeStrides / totalStrides) << "%)\n\n";
        
        // Print most common specific strides
        OutFile << "Most common specific strides:\n";
        for (size_t i = 0; i < std::min(size_t(10), strides.size()); i++) {
            double percentage = 100.0 * strides[i].second / totalStrides;
            OutFile << std::setw(8) << strides[i].first << " bytes: " 
                    << std::setw(10) << strides[i].second << " occurrences (" 
                    << std::fixed << std::setprecision(2) << percentage << "%)\n";
        }
        
        // Cache-friendliness assessment
        OutFile << "\nCache-friendliness assessment:\n";
        double smallPercentage = 100.0 * smallStrides / totalStrides;
        if (smallPercentage > 80) {
            OutFile << "  GOOD: Access pattern is primarily small strides, likely cache-friendly.\n";
        } else if (smallPercentage > 50) {
            OutFile << "  MODERATE: Mixed stride patterns detected, somewhat cache-friendly.\n";
        } else {
            OutFile << "  POOR: Large strides dominate, likely cache-unfriendly pattern.\n";
        }
    } else {
        OutFile << "  No stride data collected.\n";
    }
}

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v) {
    OutFile << "================================\n";
    OutFile << "MEMORY ACCESS PATTERN ANALYSIS\n";
    OutFile << "================================\n\n";
    
    OutFile << "Total memory accesses: " << totalAccesses << "\n";
    OutFile << "Memory reads: " << totalReads 
            << " (" << (100.0 * totalReads / (totalAccesses > 0 ? totalAccesses : 1)) << "%)\n";
    OutFile << "Memory writes: " << totalWrites 
            << " (" << (100.0 * totalWrites / (totalAccesses > 0 ? totalAccesses : 1)) << "%)\n";
    
    if (!memAccesses.empty()) {
        AnalyzeSpatialLocality();
        AnalyzeTemporalLocality();
        AnalyzeStrides();
    }
    
    OutFile.close();
}

// Print help information
INT32 Usage() {
    std::cerr << "This tool analyzes memory access patterns for matrix operations\n";
    std::cerr << "Usage: pin -t matrix_memory_analyzer.so [-o <output>] [-max <count>] -- <command>\n";
    std::cerr << "       -o <output>: specify output file name (default: memtrace.out)\n";
    std::cerr << "       -max <count>: maximum number of memory accesses to record (default: 1000000)\n";
    return -1;
}

// Main function
int main(int argc, char *argv[]) {
    // Initialize PIN
    if (PIN_Init(argc, argv)) return Usage();
    
    // Set maximum accesses to record
    maxRecordedAccesses = KnobMaxAccesses.Value();
    
    // Open output file
    OutFile.open(KnobOutputFile.Value().c_str());
    if (!OutFile) {
        std::cerr << "ERROR: Could not open output file: " << KnobOutputFile.Value() << std::endl;
        return 1;
    }
    
    // Register function to be called for instruction instrumentation
    INS_AddInstrumentFunction(Instruction, 0);
    
    // Register function to be called when application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program
    PIN_StartProgram();
    
    return 0;
}