# Building GpuCracker on Linux

This guide provides detailed instructions for building GpuCracker v50.0 on Linux systems.

## Supported Distributions

- Ubuntu 20.04/22.04/24.04 LTS (Recommended)
- Debian 11/12
- Fedora 38/39/40
- Arch Linux
- CentOS/RHEL 8/9

## Prerequisites

### Minimum Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| CPU | x86_64, 4 cores | x86_64, 16+ cores |
| RAM | 8 GB | 64+ GB |
| Disk | 10 GB | 100+ GB (for blockchain data) |
| GPU | Optional | NVIDIA RTX 30/40 series |

### Required Packages

#### Ubuntu/Debian

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    git \
    cmake \
    pkg-config \
    libssl-dev \
    libgmp-dev \
    libsecp256k1-dev \
    ocl-icd-opencl-dev \
    libomp-dev \
    libboost-all-dev \
    nlohmann-json3-dev
```

#### Fedora

```bash
sudo dnf install -y \
    gcc-c++ \
    git \
    cmake \
    pkgconfig \
    openssl-devel \
    gmp-devel \
    libsecp256k1-devel \
    ocl-icd-devel \
    libomp-devel \
    boost-devel \
    json-devel
```

#### Arch Linux

```bash
sudo pacman -S \
    base-devel \
    git \
    cmake \
    openssl \
    gmp \
    secp256k1 \
    opencl-headers \
    ocl-icd \
    openmp \
    boost \
    nlohmann-json
```

## CUDA Installation (Optional, for NVIDIA GPUs)

### Method 1: Using NVIDIA Repositories (Recommended)

```bash
# Add NVIDIA package repositories
wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-keyring_1.0-1_all.deb
sudo dpkg -i cuda-keyring_1.0-1_all.deb
sudo apt update

# Install CUDA 12.4
sudo apt install -y cuda-toolkit-12-4

# Add to PATH
echo 'export PATH=/usr/local/cuda-12.4/bin${PATH:+:${PATH}}' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda-12.4/lib64${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}' >> ~/.bashrc
source ~/.bashrc
```

### Method 2: Using Runfile Installer

```bash
# Download from https://developer.nvidia.com/cuda-downloads
wget https://developer.download.nvidia.com/compute/cuda/12.4.0/local_installers/cuda_12.4.0_550.54.14_linux.run
sudo sh cuda_12.4.0_550.54.14_linux.run --toolkit --silent
```

### Verify CUDA Installation

```bash
nvcc --version
nvidia-smi
```

## Building from Source

### Step 1: Clone Repository

```bash
cd /opt
git clone https://github.com/bekli23/GpuCracker.git
cd GpuCracker
```

### Step 2: Build with CUDA Support

```bash
# Clean previous builds
make clean

# Build with CUDA and OpenCL support
make -j$(nproc) CUDA=1 OPENCL=1

# Or for maximum optimization
make -j$(nproc) CUDA=1 OPENCL=1 OPTIMIZE=3
```

### Step 3: Build Without CUDA (CPU/OpenCL only)

```bash
make clean
make -j$(nproc) CUDA=0 OPENCL=1
```

### Step 4: Build Minimal Version

```bash
make clean
make -j$(nproc) MINIMAL=1
```

## Makefile Options

| Option | Values | Description |
|--------|--------|-------------|
| `CUDA` | 0, 1 | Enable CUDA support |
| `OPENCL` | 0, 1 | Enable OpenCL support |
| `VULKAN` | 0, 1 | Enable Vulkan support |
| `DEBUG` | 0, 1 | Debug build |
| `OPTIMIZE` | 0-3 | Optimization level |
| `STATIC` | 0, 1 | Static linking |

## Troubleshooting

### Issue: CUDA not found

**Symptom:**
```
nvcc: command not found
```

**Solution:**
```bash
# Check CUDA installation
ls /usr/local/cuda/bin/nvcc

# Add to PATH
export PATH=/usr/local/cuda/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/cuda/lib64:$LD_LIBRARY_PATH
```

### Issue: libsecp256k1 not found

**Symptom:**
```
error: secp256k1.h: No such file
```

**Solution:**
```bash
# Build from source if package not available
git clone https://github.com/bitcoin-core/secp256k1.git
cd secp256k1
./autogen.sh
./configure --enable-module-recovery
make
sudo make install
sudo ldconfig
```

### Issue: OpenCL headers missing

**Symptom:**
```
error: CL/cl.h: No such file
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt install opencl-headers ocl-icd-opencl-dev

# Or install Khronos headers
sudo mkdir -p /usr/local/include/CL
sudo wget -O /usr/local/include/CL/cl.h https://raw.githubusercontent.com/KhronosGroup/OpenCL-Headers/main/CL/cl.h
```

### Issue: Boost multiprecision not found

**Symptom:**
```
error: boost/multiprecision/cpp_int.hpp: No such file
```

**Solution:**
```bash
# Ubuntu/Debian
sudo apt install libboost-all-dev

# Or install minimal boost
wget https://boostorg.jfrog.io/artifactory/main/release/1.84.0/source/boost_1_84_0.tar.gz
tar xzf boost_1_84_0.tar.gz
cd boost_1_84_0
./bootstrap.sh
./b2 --with-system --with-filesystem --with-thread
sudo ./b2 install
```

### Issue: Linking errors with CUDA

**Symptom:**
```
undefined reference to `cudaMalloc'
```

**Solution:**
```bash
# Check CUDA libraries
ls /usr/local/cuda/lib64/libcudart.so*

# Add to linker path
echo '/usr/local/cuda/lib64' | sudo tee /etc/ld.so.conf.d/cuda.conf
sudo ldconfig
```

## Performance Optimization

### CPU Optimization

```bash
# Set CPU governor to performance
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Disable CPU hyperthreading (for some workloads)
echo 0 | sudo tee /sys/devices/system/cpu/cpu*/online  # for odd-numbered cores

# Enable huge pages
echo 1024 | sudo tee /proc/sys/vm/nr_hugepages
```

### GPU Optimization

```bash
# Enable persistence mode (NVIDIA)
sudo nvidia-smi -pm 1

# Set power limit (e.g., 300W for RTX 4090)
sudo nvidia-smi -pl 300

# Enable ECC (if supported)
sudo nvidia-smi -e 1
```

### Memory Optimization

```bash
# Increase virtual memory
sudo sysctl -w vm.swappiness=10
sudo sysctl -w vm.vfs_cache_pressure=50

# Increase file descriptors
ulimit -n 65536
```

## Installation

### System-wide Installation

```bash
sudo cp GpuCracker /usr/local/bin/
sudo chmod +x /usr/local/bin/GpuCracker

# Create config directory
sudo mkdir -p /etc/GpuCracker
sudo cp gpucracker.conf /etc/GpuCracker/

# Create data directory
sudo mkdir -p /var/lib/GpuCracker
sudo chown $USER:$USER /var/lib/GpuCracker
```

### User Installation

```bash
# Add to local bin
mkdir -p ~/.local/bin
cp GpuCracker ~/.local/bin/

# Add to PATH
echo 'export PATH=$HOME/.local/bin:$PATH' >> ~/.bashrc
source ~/.bashrc

# Create config in home
mkdir -p ~/.config/GpuCracker
cp gpucracker.conf ~/.config/GpuCracker/
```

## Verification

### Test Basic Functionality

```bash
# Check version
./GpuCracker --version

# List available GPUs
./GpuCracker --list-gpus

# Run self-test
./GpuCracker --self-test

# Benchmark
./GpuCracker --benchmark
```

### Test Specific Modes

```bash
# Test mnemonic mode
./GpuCracker --mode mnemonic --help

# Test BSGS mode
./GpuCracker --mode bsgs --help

# Test with small range
./GpuCracker --mode bsgs --bsgs-pub 0279BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798 --bsgs-range 20
```

## Docker Build (Alternative)

### Using Provided Dockerfile

```bash
# Build image
docker build -t gpucracker:latest .

# Run with GPU support
docker run --gpus all -it gpucracker:latest

# Run specific mode
docker run --gpus all gpucracker:latest --mode bsgs --bsgs-range 40
```

### Docker Compose

```yaml
version: '3.8'
services:
  gpucracker:
    build: .
    runtime: nvidia
    environment:
      - NVIDIA_VISIBLE_DEVICES=all
    volumes:
      - ./data:/data
      - ./results:/results
    command: --mode bsgs --bsgs-range 50
```

## Uninstallation

```bash
# Remove binary
sudo rm -f /usr/local/bin/GpuCracker

# Remove config
sudo rm -rf /etc/GpuCracker

# Remove data (optional)
sudo rm -rf /var/lib/GpuCracker

# Clean build files
make clean
```

## Getting Help

If you encounter issues:

1. Check `README_TROUBLESHOOTING.md`
2. Run with verbose output: `./GpuCracker --verbose --debug`
3. Check system logs: `dmesg | tail -50`
4. Verify GPU status: `nvidia-smi` or `clinfo`

## See Also

- `README_BUILD_WIN.md` - Windows build instructions
- `README_MODE_*.md` - Mode-specific documentation
- `README_COMMAND_*.md` - Command reference
