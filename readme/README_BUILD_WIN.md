# Building GpuCracker on Windows

This guide provides detailed instructions for building GpuCracker v50.0 on Windows systems.

## Supported Platforms

- Windows 10 (64-bit) - Version 1903 or later
- Windows 11 (64-bit)
- Windows Server 2019/2022

## Prerequisites

### Required Software

| Component | Minimum Version | Recommended |
|-----------|-----------------|-------------|
| Visual Studio | 2019 | 2022 (v17.8+) |
| CUDA Toolkit | 11.8 | 12.4 |
| vcpkg | Latest | Latest |
| CMake | 3.20 | 3.28+ |
| Python | 3.8 | 3.11+ |

### Hardware Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| CPU | x86_64, 4 cores | x86_64, 16+ cores |
| RAM | 8 GB | 32+ GB |
| Disk | 20 GB SSD | 100+ GB NVMe |
| GPU | Optional | NVIDIA RTX 30/40 series |

## Step 1: Install Visual Studio 2022

### Download

Download from: https://visualstudio.microsoft.com/downloads/

### Required Workloads

During installation, select:

- [x] **Desktop development with C++**
  - MSVC v143 - VS 2022 C++ x64/x86 build tools
  - Windows 10/11 SDK
  - C++ CMake tools for Windows
  - C++ AddressSanitizer

- [x] **Linux development with C++** (optional, for WSL)

### Individual Components

Also select:
- [x] C++ ATL for latest v143 build tools
- [x] C++ MFC for latest v143 build tools
- [x] Windows Universal CRT SDK
- [x] C++ Clang Compiler for Windows

## Step 2: Install CUDA Toolkit 12.4

### Download

Download from: https://developer.nvidia.com/cuda-downloads

Select:
- **Operating System**: Windows
- **Architecture**: x86_64
- **Version**: 11/10
- **Installer Type**: exe (local)

### Installation

1. Run the installer as Administrator
2. Choose **Custom (Advanced)** installation
3. Select components:
   - [x] CUDA > Development
   - [x] CUDA > Runtime
   - [x] CUDA > Documentation
   - [x] Driver Components

4. Verify installation:
```cmd
nvcc --version
nvidia-smi
```

### Environment Variables

Add to System Environment Variables:

```
CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4
CUDA_PATH_V12_4=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4
PATH=%CUDA_PATH%\bin;%PATH%
```

## Step 3: Install vcpkg

### Installation

```cmd
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

### Install Dependencies

```cmd
cd C:\vcpkg

# Core dependencies
.\vcpkg install openssl:x64-windows
.\vcpkg install boost-multiprecision:x64-windows
.\vcpkg install boost-system:x64-windows
.\vcpkg install boost-filesystem:x64-windows
.\vcpkg install boost-thread:x64-windows

# Optional dependencies
.\vcpkg install secp256k1:x64-windows
.\vcpkg install nlohmann-json:x64-windows
```

### Verify Installation

```cmd
.\vcpkg list
```

## Step 4: Install Vulkan SDK (Optional)

For Vulkan compute support:

1. Download from: https://vulkan.lunarg.com/sdk/home
2. Install Vulkan SDK 1.3.x
3. Set environment variable:
   ```
   VULKAN_SDK=C:\VulkanSDK\1.3.x.x
   ```

## Step 5: Build GpuCracker

### Method 1: Using Visual Studio IDE

1. **Open Solution**
   - Open `GpuCracker.sln` in Visual Studio 2022
   - Or open `GpuCracker - all video card cuda 12.4.vcxproj`

2. **Select Configuration**
   - Configuration: **Release**
   - Platform: **x64**

3. **Build**
   - Build → Build Solution (Ctrl+Shift+B)
   - Or: Build → Rebuild Solution

4. **Verify Output**
   - Check `bin\x64\Release\GpuCracker.exe`

### Method 2: Using MSBuild (Command Line)

```cmd
cd "C:\path\to\GpuCracker"

# Set environment
"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

# Build
msbuild GpuCracker.sln /p:Configuration=Release /p:Platform=x64

# Or specific project
msbuild "GpuCracker - all video card cuda 12.4.vcxproj" /p:Configuration=Release /p:Platform=x64
```

### Method 3: Using CMake

```cmd
cd "C:\path\to\GpuCracker"

# Create build directory
mkdir build
cd build

# Configure
cmake .. -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake `
  -DCUDA_TOOLKIT_ROOT_DIR="C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4"

# Build
cmake --build . --config Release --parallel

# Or open in VS
cmake --open .
```

## Build Configurations

### Debug Build

```cmd
msbuild GpuCracker.sln /p:Configuration=Debug /p:Platform=x64
```

### Release with Debug Info

```cmd
msbuild GpuCracker.sln /p:Configuration=RelWithDebInfo /p:Platform=x64
```

### Static Linking

```cmd
msbuild GpuCracker.sln /p:Configuration=Release /p:Platform=x64 /p:RuntimeLibrary=MultiThreaded
```

## Post-Build Setup

### Copy Dependencies

The project includes automatic post-build copying. If needed, manually copy:

```cmd
cd C:\path\to\GpuCracker\bin\x64\Release

# Copy vcpkg DLLs
xcopy /y "C:\vcpkg\installed\x64-windows\bin\*.dll" .

# Copy CUDA DLLs (if not in PATH)
xcopy /y "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4\bin\cudart64_12.dll" .
xcopy /y "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4\bin\curand64_10.dll" .
```

### Copy Data Files

```cmd
# Copy wordlist
copy "C:\path\to\GpuCracker\bip39\english.txt" .

# Copy AKM profiles
xcopy /s /i "C:\path\to\GpuCracker\akm" akm\

# Copy configuration
copy "C:\path\to\GpuCracker\gpucracker.conf" .
```

## Troubleshooting

### Issue: CUDA not found

**Symptom:**
```
The CUDA toolset is not installed
```

**Solution:**
1. Verify CUDA installation:
   ```cmd
   nvcc --version
   ```
2. Check Visual Studio integration:
   - Open VS Installer
   - Modify → Individual Components
   - Search "CUDA" and install CUDA 12.4 VS integration

### Issue: vcpkg packages not found

**Symptom:**
```
Cannot find package openssl
```

**Solution:**
```cmd
cd C:\vcpkg
.\vcpkg integrate install
.\vcpkg install openssl:x64-windows --recurse
```

### Issue: Boost multiprecision errors

**Symptom:**
```
boost/multiprecision/cpp_int.hpp not found
```

**Solution:**
```cmd
cd C:\vcpkg
.\vcpkg install boost-multiprecision:x64-windows
.\vcpkg install boost:x64-windows
```

### Issue: Linker errors (LNK2019)

**Symptom:**
```
unresolved external symbol secp256k1_context_create
```

**Solution:**
1. Build libsecp256k1 manually:
   ```cmd
   git clone https://github.com/bitcoin-core/secp256k1.git
   cd secp256k1
   msbuild build\secp256k1.sln /p:Configuration=Release
   ```

2. Or use NuGet package:
   ```cmd
   nuget install secp256k1_vc143
   ```

### Issue: CUDA architecture mismatch

**Symptom:**
```
no kernel image is available for execution on the device
```

**Solution:**
Edit project properties:
- CUDA C/C++ → Device → Code Generation
- Add your GPU architecture (e.g., `compute_86,sm_86` for RTX 30 series)

### Issue: OpenCL headers not found

**Symptom:**
```
CL/cl.h not found
```

**Solution:**
1. Install OpenCL SDK from GPU vendor
2. Or download Khronos headers:
   ```cmd
   git clone https://github.com/KhronosGroup/OpenCL-Headers.git
   set INCLUDE=%INCLUDE%;C:\OpenCL-Headers
   ```

## Performance Optimization

### Visual Studio Settings

1. **C/C++ → Optimization**
   - Optimization: Maximum Optimization (Favor Speed) (/O2)
   - Inline Function Expansion: Any Suitable (/Ob2)
   - Enable Intrinsic Functions: Yes (/Oi)
   - Favor Size Or Speed: Favor fast code (/Ot)

2. **C/C++ → Code Generation**
   - Enable Enhanced Instruction Set: Advanced Vector Extensions 2 (/arch:AVX2)
   - Floating Point Model: Fast (/fp:fast)

3. **Linker → Optimization**
   - Link Time Code Generation: Use Link Time Code Generation (/LTCG)
   - Optimize for Windows 98: No
   - Profile Guided Optimization: Generate Profile

### Windows Optimization

```cmd
# Disable Windows Defender real-time scanning for build folder
Add-MpPreference -ExclusionPath "C:\path\to\GpuCracker\bin"

# Set high performance power plan
powercfg /setactive 8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c

# Disable Windows Search indexing
sc config WSearch start= disabled
```

### GPU Optimization

```cmd
# Enable persistence mode
nvidia-smi -pm 1

# Set power limit (e.g., 350W for RTX 4090)
nvidia-smi -pl 350

# Enable compute-only mode (Tesla/Quadro)
nvidia-smi -c EXCLUSIVE_THREAD
```

## Verification

### Test Installation

```cmd
cd C:\path\to\GpuCracker\bin\x64\Release

# Check version
GpuCracker.exe --version

# List GPUs
GpuCracker.exe --list-gpus

# Run benchmark
GpuCracker.exe --benchmark

# Test BSGS mode
GpuCracker.exe --mode bsgs --bsgs-range 30
```

### Test Modes

```cmd
# Test mnemonic
GpuCracker.exe --mode mnemonic --help

# Test with small range
GpuCracker.exe --mode bsgs --bsgs-pub 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798 --bsgs-range 20
```

## Distribution

### Create Installer

Using Inno Setup:

```pascal
[Setup]
AppName=GpuCracker
AppVersion=50.0
DefaultDirName={pf}\GpuCracker

[Files]
Source: "bin\x64\Release\GpuCracker.exe"; DestDir: "{app}"
Source: "bin\x64\Release\*.dll"; DestDir: "{app}"
Source: "bip39\english.txt"; DestDir: "{app}\bip39"
Source: "gpucracker.conf"; DestDir: "{app}"
```

### Create ZIP Package

```cmd
cd C:\path\to\GpuCracker\bin\x64\Release
7z a GpuCracker-v50.0-win64.zip GpuCracker.exe *.dll *.txt *.conf
```

## Uninstallation

```cmd
# Remove installation folder
rmdir /s /q "C:\Program Files\GpuCracker"

# Remove user data
rmdir /s /q "%LOCALAPPDATA%\GpuCracker"

# Remove from PATH (if added manually)
```

## See Also

- `README_BUILD_LINUX.md` - Linux build instructions
- `README_MODE_*.md` - Mode-specific documentation
- `README_COMMAND_*.md` - Command reference
