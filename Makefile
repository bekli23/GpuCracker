# ============================================================================
# GpuCracker v50.0 - Makefile (Production Edition)
# Supports: CUDA, OpenCL, All 10 Modes (mnemonic, akm, check, scan, xprv-mode, 
#           brainwallet, pubkey, bsgs, rho, hybrid)
# Features: BSGS GPU Acceleration, Pollard's Rho, Hybrid Algorithm Selection,
#           Block Explorer, Direct Blockchain File Reader, Path Files
# ============================================================================

# --- VERSION ---
VERSION = 50.0
BUILD_DATE = $(shell date +%Y-%m-%d)

# --- CUDA DETECTION ---
CUDA_PATH ?= /usr/local/cuda
CUDA_EXISTS := $(shell test -d $(CUDA_PATH) && echo 1 || echo 0)

# --- COMPILERS ---
CXX  := g++
ifeq ($(CUDA_EXISTS),1)
    NVCC := $(CUDA_PATH)/bin/nvcc
    HAS_CUDA := 1
else
    HAS_CUDA := 0
    $(warning CUDA not found at $(CUDA_PATH). Building without CUDA support.)
endif

# --- DETECT BOOST ---
BOOST_EXISTS := $(shell pkg-config --exists boost_system && echo 1 || echo 0)
ifeq ($(BOOST_EXISTS),1)
    BOOST_FLAGS := $(shell pkg-config --cflags boost_system)
    BOOST_LIBS := $(shell pkg-config --libs boost_system)
    HAS_BOOST := 1
else
    # Check if boost headers exist
    BOOST_HDR_EXISTS := $(shell test -d /usr/include/boost && echo 1 || echo 0)
    ifeq ($(BOOST_HDR_EXISTS),1)
        BOOST_FLAGS := -I/usr/include/boost
        BOOST_LIBS := -lboost_system
        HAS_BOOST := 1
    else
        BOOST_FLAGS :=
        BOOST_LIBS :=
        HAS_BOOST := 0
        $(warning Boost not found. Building without Boost support.)
    endif
endif

# --- DETECT OpenCL ---
OPENCL_PKG_EXISTS := $(shell pkg-config --exists OpenCL && echo 1 || echo 0)
OPENCL_HDR_EXISTS := $(shell test -f /usr/include/CL/cl.h && echo 1 || echo 0)
ifeq ($(OPENCL_PKG_EXISTS),1)
    OPENCL_FLAGS := $(shell pkg-config --cflags OpenCL)
    OPENCL_LIBS := $(shell pkg-config --libs OpenCL)
    HAS_OPENCL := 1
else ifeq ($(OPENCL_HDR_EXISTS),1)
    OPENCL_FLAGS := -I/usr/include/CL
    OPENCL_LIBS := -lOpenCL
    HAS_OPENCL := 1
else
    HAS_OPENCL := 0
    OPENCL_FLAGS :=
    OPENCL_LIBS :=
    $(warning OpenCL not found. Building without OpenCL support.)
    $(warning Install with: sudo apt-get install ocl-icd-opencl-dev)
endif

# --- DETECT SECP256K1 ---
SECP256K1_PKG_EXISTS := $(shell pkg-config --exists libsecp256k1 && echo 1 || echo 0)
SECP256K1_HDR_EXISTS := $(shell test -f /usr/include/secp256k1.h && echo 1 || echo 0)
ifeq ($(SECP256K1_PKG_EXISTS),1)
    SECP_FLAGS := $(shell pkg-config --cflags libsecp256k1)
    SECP_LIBS := $(shell pkg-config --libs libsecp256k1)
    HAS_SECP256K1 := 1
else ifeq ($(SECP256K1_HDR_EXISTS),1)
    SECP_FLAGS := -I/usr/include
    SECP_LIBS := -lsecp256k1
    HAS_SECP256K1 := 1
else
    HAS_SECP256K1 := 0
    $(warning libsecp256k1 not found. Building without secp256k1 support.)
    $(warning Install with: sudo apt-get install libsecp256k1-dev)
endif

# --- INCLUDES ---
INCLUDES = -I. -I$(CUDA_PATH)/include $(BOOST_FLAGS) $(OPENCL_FLAGS) $(SECP_FLAGS)

# --- COMPILER FLAGS ---
# C++ Flags
CXXFLAGS_COMMON = -O3 -std=c++17 -fopenmp -pthread -Wall -Wextra $(INCLUDES)
CXXFLAGS = $(CXXFLAGS_COMMON) -DVERSION=\"$(VERSION)\" -DBUILD_DATE=\"$(BUILD_DATE)\" -fpermissive
ifeq ($(HAS_SECP256K1),0)
    CXXFLAGS += -DNO_SECP256K1
endif
ifeq ($(HAS_OPENCL),0)
    CXXFLAGS += -DNO_OPENCL
endif
CXXFLAGS_DEBUG = -g -O0 -std=c++17 -fopenmp -pthread -DDEBUG $(INCLUDES)

# --- CUDA FLAGS ---
ifeq ($(HAS_CUDA),1)
CXXFLAGS += -DUSE_CUDA -DHAS_CUDA
NVCCFLAGS = -O3 -std=c++17 -Xcompiler -fopenmp -Xcompiler -fpermissive -w $(INCLUDES) -DUSE_CUDA -DHAS_CUDA \
            -gencode arch=compute_50,code=sm_50 \
            -gencode arch=compute_52,code=sm_52 \
            -gencode arch=compute_53,code=sm_53 \
            -gencode arch=compute_60,code=sm_60 \
            -gencode arch=compute_61,code=sm_61 \
            -gencode arch=compute_62,code=sm_62 \
            -gencode arch=compute_70,code=sm_70 \
            -gencode arch=compute_72,code=sm_72 \
            -gencode arch=compute_75,code=sm_75 \
            -gencode arch=compute_80,code=sm_80 \
            -gencode arch=compute_86,code=sm_86 \
            -gencode arch=compute_87,code=sm_87 \
            -gencode arch=compute_89,code=sm_89 \
            -gencode arch=compute_90,code=sm_90
endif

# --- LIBRARIES ---
# Detect GCC version for filesystem support
GCC_VERSION := $(shell $(CXX) -dumpversion | cut -f1 -d.)
GCC_MAJOR := $(shell echo $(GCC_VERSION) | cut -f1 -d.)

# Detect GMP
GMP_EXISTS := $(shell test -f /usr/include/gmp.h && echo 1 || echo 0)
ifeq ($(GMP_EXISTS),1)
    GMP_LIBS := -lgmp
else
    GMP_LIBS :=
    $(warning GMP not found. Some features may be unavailable.)
endif

LDFLAGS_COMMON = $(OPENCL_LIBS) -lcrypto -lssl $(SECP_LIBS) $(GMP_LIBS) -pthread -fopenmp $(BOOST_LIBS)

# Filesystem library support
# GCC < 9 needs -lstdc++fs for std::experimental::filesystem
# GCC >= 9 has std::filesystem built-in
ifeq ($(shell test $(GCC_MAJOR) -lt 9 && echo 1 || echo 0),1)
    LDFLAGS_COMMON += -lstdc++fs
    $(info GCC $(GCC_MAJOR) detected, adding -lstdc++fs for filesystem support)
endif
ifeq ($(HAS_CUDA),1)
    LDFLAGS = -L$(CUDA_PATH)/lib64 -lcudart -lcurand $(LDFLAGS_COMMON)
else
    LDFLAGS = $(LDFLAGS_COMMON)
endif

# --- TARGETS ---
TARGET_APP      = GpuCracker
TARGET_BLOOM    = build_bloom
TARGET_RECOVER  = recover_tool
TARGET_SEED2PRIV= seed2priv_tool
TARGET_TEST     = test_runner
TARGET_CHECK    = check_tool

# --- SOURCE FILES ---
SRCS_CPP = $(wildcard *.cpp)
SRCS_CU  = $(wildcard *.cu)

# Exclude list for main app
EXCLUDE_LIST = build_bloom.cpp recover.cpp seed2priv_main.cpp schematic_test_gen.cpp loader_wrapper.cpp test_runner.cpp test_minimal.cpp test_syntax.cpp

# CUDA-dependent files (exclude if no CUDA)
CUDA_DEPENDENT_CPP = gpu_memory_pool.cpp web_dashboard.cpp bsgs_gpu_integration.cpp

ifeq ($(HAS_CUDA),0)
    EXCLUDE_LIST += $(CUDA_DEPENDENT_CPP)
    $(warning Excluding CUDA-dependent files: $(CUDA_DEPENDENT_CPP))
endif

SRCS_APP_CPP = $(filter-out $(EXCLUDE_LIST), $(SRCS_CPP))

# Exclude GpuCore.cu (using GpuCore_fixed.cu instead, matching Windows project)
CU_EXCLUDE_LIST = GpuCore.cu main.cu
SRCS_APP_CU = $(filter-out $(CU_EXCLUDE_LIST), $(SRCS_CU))

# Objects
OBJS_APP = $(SRCS_APP_CPP:.cpp=.o)
ifeq ($(HAS_CUDA),1)
    OBJS_APP += $(SRCS_APP_CU:.cu=.o)
endif

# --- BUILD RULES ---

.PHONY: all clean test debug install check-deps help version copy-paths \
        check-cuda check-boost check-opencl check-secp256k1 print-config distclean

# Default target - exclude tools that require secp256k1 when not available
ifeq ($(HAS_SECP256K1),1)
    ALL_TARGETS = $(TARGET_APP) $(TARGET_BLOOM) $(TARGET_RECOVER) $(TARGET_SEED2PRIV)
else
    ALL_TARGETS = $(TARGET_APP) $(TARGET_BLOOM)
endif

all: check-deps $(ALL_TARGETS)
	@echo ""
	@echo "========================================="
	@echo "Build completed successfully!"
	@echo "========================================="
	@echo "Targets built:"
	@echo "  - $(TARGET_APP)       (Main application)"
	@echo "  - $(TARGET_BLOOM)     (Bloom filter builder)"
ifeq ($(HAS_SECP256K1),1)
	@echo "  - $(TARGET_RECOVER)   (Recovery tool)"
	@echo "  - $(TARGET_SEED2PRIV) (Seed to private key)"
else
	@echo ""
	@echo "Note: recover_tool and seed2priv_tool require secp256k1"
	@echo "      Install with: sudo apt-get install libsecp256k1-dev"
endif
	@echo ""
	@echo "Supported modes: mnemonic, akm, check, scan, xprv-mode, brainwallet, pubkey, bsgs, rho, hybrid"
	@echo ""
	@echo "NEW in v50.0 - ECDLP Solving Suite:"
	@echo "  bsgs   - Baby-Step Giant-Step (GPU accelerated)"
	@echo "  rho    - Pollard's Rho (O(1) memory)"
	@echo "  hybrid - Auto-select optimal algorithm"
	@echo ""
	@echo "Features:"
	@echo "  - BSGS GPU acceleration (10-100x speedup)"
	@echo "  - Multi-target BSGS search (linear speedup)"
	@echo "  - Bloom filter optimization (10x less memory)"
	@echo "  - Distributed computing support"
	@echo ""
	@echo "Path files available for --path-file:"
	@echo "  paths_bitcoin_standard.txt      - Basic BIP44/49/84"
	@echo "  paths_bitcoin_comprehensive.txt - 150+ derivation paths"
	@echo "  paths_deep_scan.txt             - High index ranges"
	@echo "  paths_multicoin.txt             - Multi-currency"
	@echo ""
	@echo "Block Explorer:"
	@echo "  --enable-explorer --blockchain-dir PATH --index-dir PATH"
	@echo "  API: /api/status, /api/check?address=ADDR"
	@echo ""

# --- DEPENDENCY CHECKS ---
check-deps: check-cuda check-boost check-opencl check-secp256k1

check-cuda:
ifeq ($(HAS_CUDA),1)
	@echo "[OK] CUDA found at $(CUDA_PATH)"
else
	@echo "[WARN] CUDA not found - building without CUDA support"
	@echo "       BSGS GPU acceleration will be disabled"
endif

check-boost:
ifeq ($(HAS_BOOST),1)
	@echo "[OK] Boost found"
else
	@echo "[WARN] Boost not found via pkg-config, using defaults"
endif

check-opencl:
ifeq ($(HAS_OPENCL),1)
	@echo "[OK] OpenCL found"
else
	@echo "[WARN] OpenCL not found - OpenCL GPU backend disabled"
endif

check-secp256k1:
ifeq ($(HAS_SECP256K1),1)
	@echo "[OK] secp256k1 found"
else
	@echo "[WARN] secp256k1 not found - some features disabled"
endif

# --- MAIN APPLICATION ---
$(TARGET_APP): $(OBJS_APP)
	@echo "[LINK] $@"
	$(CXX) $(OBJS_APP) -o $@ $(LDFLAGS)
	@echo "[OK] Built $@"

# --- TOOLS ---

# Build Bloom Tool
$(TARGET_BLOOM): build_bloom.o
	@echo "[LINK] $@"
	$(CXX) build_bloom.o -o $@ $(LDFLAGS)
	@echo "[OK] Built $@"

# Recover Tool
$(TARGET_RECOVER): recover.o
	@echo "[LINK] $@"
	$(CXX) recover.o -o $@ $(LDFLAGS)
	@echo "[OK] Built $@"

# Seed2Priv Tool
$(TARGET_SEED2PRIV): seed2priv_main.o
	@echo "[LINK] $@"
	$(CXX) seed2priv_main.o -o $@ $(LDFLAGS)
	@echo "[OK] Built $@"

# Check Tool (for address verification)
$(TARGET_CHECK): check_tool.o
	@echo "[LINK] $@"
	$(CXX) check_tool.o -o $@ $(LDFLAGS)
	@echo "[OK] Built $@"

# --- COMPILE RULES ---

%.o: %.cpp
	@echo "[CXX]  $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

ifeq ($(HAS_CUDA),1)
%.o: %.cu
	@echo "[NVCC] $<"
	$(NVCC) $(NVCCFLAGS) -c $< -o $@
endif

# --- SPECIAL TARGETS ---

# Test runner
test: $(TARGET_TEST)
	@echo "[TEST] Running tests..."
	./$(TARGET_TEST)

$(TARGET_TEST): tests.cpp logger.cpp config_manager.cpp checkpoint_manager.cpp two_stage_filter.cpp
	@echo "[BUILD TEST] $@"
	$(CXX) $(CXXFLAGS) -DTEST_MODE tests.cpp logger.cpp config_manager.cpp checkpoint_manager.cpp two_stage_filter.cpp -o $@ $(LDFLAGS)

# Debug build
debug: CXXFLAGS = $(CXXFLAGS_DEBUG)
debug: all
	@echo "[DEBUG] Debug build completed"

# Benchmark
benchmark: $(TARGET_APP)
	@echo "[BENCHMARK] Running benchmarks..."
	./$(TARGET_APP) --benchmark

# BSGS-specific benchmark
benchmark-bsgs: $(TARGET_APP)
	@echo "[BENCHMARK] BSGS performance test..."
	./$(TARGET_APP) --mode bsgs --benchmark --bsgs-range 40

# Rho-specific benchmark
benchmark-rho: $(TARGET_APP)
	@echo "[BENCHMARK] Rho performance test..."
	./$(TARGET_APP) --mode rho --benchmark --rho-range 60

# Install to system
install: all
	@echo "[INSTALL] Installing to /usr/local/bin..."
	install -m 755 $(TARGET_APP) /usr/local/bin/
	install -m 755 $(TARGET_BLOOM) /usr/local/bin/
ifeq ($(HAS_SECP256K1),1)
	install -m 755 $(TARGET_RECOVER) /usr/local/bin/
	install -m 755 $(TARGET_SEED2PRIV) /usr/local/bin/
endif
	mkdir -p /etc/gpucracker
	mkdir -p /usr/share/gpucracker/paths
	cp -n gpucracker.conf /etc/gpucracker/ 2>/dev/null || true
	@echo "[INSTALL] Copying path files..."
	cp -n paths_*.txt /usr/share/gpucracker/paths/ 2>/dev/null || true
	cp -n PATH_FILES_README.md /usr/share/gpucracker/ 2>/dev/null || true
	cp -n readme/*.md /usr/share/gpucracker/ 2>/dev/null || true
	@echo "[OK] Installation complete"
	@echo ""
	@echo "Configuration: /etc/gpucracker/gpucracker.conf"
	@echo "Path files: /usr/share/gpucracker/paths/"
	@echo "Documentation: /usr/share/gpucracker/"

# Uninstall
uninstall:
	@echo "[UNINSTALL] Removing from /usr/local/bin..."
	rm -f /usr/local/bin/$(TARGET_APP)
	rm -f /usr/local/bin/$(TARGET_BLOOM)
	rm -f /usr/local/bin/$(TARGET_RECOVER)
	rm -f /usr/local/bin/$(TARGET_SEED2PRIV)
	@echo "[UNINSTALL] Removing configuration..."
	rm -rf /etc/gpucracker/
	rm -rf /usr/share/gpucracker/
	@echo "[OK] Uninstall complete"

# Clean build files
clean:
	@echo "[CLEAN] Removing build files..."
	rm -f *.o $(TARGET_APP) $(TARGET_BLOOM) $(TARGET_RECOVER) $(TARGET_SEED2PRIV) $(TARGET_TEST) $(TARGET_CHECK)
	@echo "[OK] Clean complete"

# Copy path files to build directory
copy-paths:
	@echo "[COPY] Copying path files to current directory..."
	@cp -v paths_*.txt . 2>/dev/null || echo "Path files already in directory"
	@echo "[OK] Path files ready"
	@echo ""
	@echo "Usage examples:"
	@echo "  ./GpuCracker --mode mnemonic --words 12 --path-file paths_bitcoin_standard.txt"
	@echo "  ./GpuCracker --mode bsgs --bsgs-pub PUBKEY --bsgs-range 60 --bsgs-gpu"
	@echo "  ./GpuCracker --mode rho --rho-pub PUBKEY --rho-range 90 --rho-gpu"
	@echo "  ./GpuCracker --mode hybrid --target-pub PUBKEY --search-range 70"

# Deep clean (includes backups)
distclean: clean
	@echo "[DISTCLEAN] Removing all generated files..."
	rm -f *.blf *.log *~ *.bak
	rm -rf build/
	rm -f bsgs_baby_steps_*.cache
	@echo "[OK] Deep clean complete"

# Show version
version:
	@echo "GpuCracker v$(VERSION) (Build: $(BUILD_DATE))"
	@echo "Compiler: $(CXX) $(shell $(CXX) --version | head -1)"
ifeq ($(HAS_CUDA),1)
	@echo "CUDA: $(NVCC) $(shell $(NVCC) --version | grep release)"
else
	@echo "CUDA: Not available"
endif
	@echo "Boost: $(if $(filter 1,$(HAS_BOOST)),Found,Not found)"
	@echo "OpenCL: $(if $(filter 1,$(HAS_OPENCL)),Found,Not found)"
	@echo "secp256k1: $(if $(filter 1,$(HAS_SECP256K1)),Found,Not found)"
	@echo ""
	@echo "Supported modes: 10"
	@echo "  mnemonic, akm, check, scan, xprv-mode, brainwallet, pubkey, bsgs, rho, hybrid"

# Show help
help:
	@echo "GpuCracker v$(VERSION) - Build System"
	@echo "======================================="
	@echo ""
	@echo "Build targets:"
	@echo "  make              - Build all targets (default)"
	@echo "  make all          - Same as above"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make test         - Build and run tests"
	@echo "  make benchmark    - Run performance benchmarks"
	@echo "  make benchmark-bsgs  - BSGS-specific benchmark"
	@echo "  make benchmark-rho   - Rho-specific benchmark"
	@echo ""
	@echo "Maintenance:"
	@echo "  make clean        - Remove build files"
	@echo "  make distclean    - Deep clean (includes backups, cache files)"
	@echo "  make install      - Install to /usr/local/bin"
	@echo "  make uninstall    - Remove from /usr/local/bin"
	@echo ""
	@echo "Info:"
	@echo "  make version      - Show version and config"
	@echo "  make check-deps   - Check dependencies"
	@echo "  make print-config - Show build configuration"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "Environment variables:"
	@echo "  CUDA_PATH         - CUDA installation path (default: /usr/local/cuda)"
	@echo "  CXX               - C++ compiler (default: g++)"
	@echo ""
	@echo "Supported modes in v$(VERSION):"
	@echo "  1. mnemonic    - BIP39 seed recovery"
	@echo "  2. akm         - Advanced Key Mapping for puzzles"
	@echo "  3. check       - Address verification"
	@echo "  4. scan        - Blockchain scanning"
	@echo "  5. xprv-mode   - Extended private key derivation"
	@echo "  6. brainwallet - Password-based wallets"
	@echo "  7. pubkey      - Public key brute force"
	@echo "  8. bsgs        - Baby-Step Giant-Step (ECDLP solver, GPU accelerated)"
	@echo "  9. rho         - Pollard's Rho (O(1) memory ECDLP solver)"
	@echo "  10. hybrid     - Auto-select optimal ECDLP algorithm"
	@echo ""
	@echo "NEW in v50.0 - ECDLP Revolution:"
	@echo "  - BSGS with GPU acceleration (10-100x speedup)"
	@echo "  - Multi-target BSGS (linear speedup with target count)"
	@echo "  - Bloom filter BSGS (10x less memory)"
	@echo "  - Pollard's Rho for unlimited ranges"
	@echo "  - Hybrid auto-selection algorithm"
	@echo "  - Distributed computing support"
	@echo ""
	@echo "Path files available:"
	@echo "  paths_bitcoin_standard.txt      - Basic BIP44/49/84"
	@echo "  paths_bitcoin_comprehensive.txt - 150+ paths"
	@echo "  paths_deep_scan.txt             - High index ranges"
	@echo "  paths_multicoin.txt             - Multi-currency"
	@echo ""
	@echo "BSGS Examples:"
	@echo "  # GPU-accelerated BSGS"
	@echo "  ./GpuCracker --mode bsgs --bsgs-pub PUBKEY --bsgs-range 60 --bsgs-gpu"
	@echo ""
	@echo "  # Multi-target with bloom filter"
	@echo "  ./GpuCracker --mode bsgs --bsgs-targets targets.txt --bsgs-bloom"
	@echo ""
	@echo "  # Distributed across 4 nodes"
	@echo "  ./GpuCracker --mode bsgs --bsgs-pub PUBKEY --bsgs-node-id 0 --bsgs-total-nodes 4"
	@echo ""
	@echo "RHO Examples:"
	@echo "  # Parallel Rho walks"
	@echo "  ./GpuCracker --mode rho --rho-pub PUBKEY --rho-instances 256 --rho-gpu"
	@echo ""
	@echo "HYBRID Examples:"
	@echo "  # Auto-select algorithm"
	@echo "  ./GpuCracker --mode hybrid --target-pub PUBKEY --search-range 70"
	@echo ""

# Print configuration
print-config:
	@echo "Build Configuration:"
	@echo "  CXX: $(CXX)"
	@echo "  NVCC: $(NVCC)"
	@echo "  CUDA_PATH: $(CUDA_PATH)"
	@echo "  HAS_CUDA: $(HAS_CUDA)"
	@echo "  HAS_BOOST: $(HAS_BOOST)"
	@echo "  HAS_OPENCL: $(HAS_OPENCL)"
	@echo "  HAS_SECP256K1: $(HAS_SECP256K1)"
	@echo "  CXXFLAGS: $(CXXFLAGS)"
	@echo "  LDFLAGS: $(LDFLAGS)"
	@echo "  SRCS_APP_CPP: $(words $(SRCS_APP_CPP)) files"
	@echo "  SRCS_APP_CU: $(words $(SRCS_APP_CU)) files"
	@echo "  OBJS_APP: $(words $(OBJS_APP)) objects"

# Cache management targets
cache-clean:
	@echo "[CACHE] Removing BSGS cache files..."
	rm -f bsgs_baby_steps_*.cache
	@echo "[OK] Cache cleaned"

cache-list:
	@echo "[CACHE] Listing BSGS cache files..."
	@ls -lh bsgs_baby_steps_*.cache 2>/dev/null || echo "No cache files found"

# Quick test targets
test-bsgs: $(TARGET_APP)
	@echo "[TEST] Testing BSGS mode..."
	./$(TARGET_APP) --mode bsgs --bsgs-range 20 --bsgs-threads 4

test-rho: $(TARGET_APP)
	@echo "[TEST] Testing RHO mode..."
	./$(TARGET_APP) --mode rho --rho-range 30 --rho-instances 16

test-hybrid: $(TARGET_APP)
	@echo "[TEST] Testing HYBRID mode..."
	./$(TARGET_APP) --mode hybrid --search-range 40
