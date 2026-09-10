#!/usr/bin/env bash
# ==============================================================================
# Samsung Galaxy A06 (SM-A065F) Custom Kernel Build Script
# Linux 4.19.191 | MediaTek Helio G85 (MT6769V) | Android 14 One UI Core 6.1
# ==============================================================================

set -e

KERNEL_DIR="$(pwd)"
OUT_DIR="${KERNEL_DIR}/out"
DEFCONFIG="a06_defconfig"
JOBS="$(nproc --all)"

echo "=== Samsung Galaxy A06 (SM-A065F) Kernel Build Pipeline ==="

# Handle clean argument
if [ "$1" = "clean" ]; then
    echo "[*] Cleaning output directory: ${OUT_DIR}"
    rm -rf "${OUT_DIR}"
    echo "[+] Clean complete."
    exit 0
fi

# Toolchain Auto-Detection
if ! command -v clang &> /dev/null; then
    # Look for Android NDK toolchain
    NDK_PATHS=(
        "$HOME/Android/Sdk/ndk"/*/toolchains/llvm/prebuilt/linux-x86_64/bin
        "/opt/android-ndk"/*/toolchains/llvm/prebuilt/linux-x86_64/bin
        "/usr/lib/llvm-"*/bin
    )
    for p in "${NDK_PATHS[@]}"; do
        if [ -f "${p}/clang" ]; then
            echo "[+] Found Clang in: ${p}"
            export PATH="${p}:${PATH}"
            break
        fi
    done
fi

if ! command -v clang &> /dev/null; then
    echo "[-] Error: Clang compiler not found in PATH or standard NDK locations."
    echo "    Please install clang or add your toolchain bin/ to PATH."
    exit 1
fi

# Detect cross compiler binutils
if command -v aarch64-linux-gnu-gcc &> /dev/null; then
    CROSS_PREFIX="aarch64-linux-gnu-"
elif command -v aarch64-linux-android-gcc &> /dev/null; then
    CROSS_PREFIX="aarch64-linux-android-"
elif [ -f "$(dirname $(command -v clang))/aarch64-linux-gnu-as" ]; then
    CROSS_PREFIX="$(dirname $(command -v clang))/aarch64-linux-gnu-"
else
    CROSS_PREFIX="aarch64-linux-gnu-"
fi

# Linker and LLVM binutils detection
CLANG_BIN_DIR="$(dirname "$(command -v clang)")"
if [ -f "${CLANG_BIN_DIR}/ld.lld" ] || command -v ld.lld &> /dev/null; then
    echo "[+] Found LLD: enabling compact RELR relocation packing"
    LLD_OPTS="LD=ld.lld NM=llvm-nm OBJCOPY=llvm-objcopy"
else
    echo "[!] LLD not found, falling back to GNU ld"
    LLD_OPTS=""
fi

export ARCH=arm64
export SUBARCH=arm64
export CROSS_COMPILE="${CROSS_PREFIX}"
export CC=clang
export CLANG_TRIPLE=aarch64-linux-gnu-
export ANDROID_MAJOR_VERSION=r

# Anti-Detection Spoofing (Matching stock Samsung factory firmware)
export KBUILD_BUILD_USER="dpi"
export KBUILD_BUILD_HOST="21DKGB11"
export LOCALVERSION=""

mkdir -p "${OUT_DIR}"

echo "[*] Compiler: $(clang --version | head -n 1)"
if [ -n "${LLD_OPTS}" ]; then
    echo "[*] Linker: $(ld.lld --version 2>/dev/null || ${CLANG_BIN_DIR}/ld.lld --version | head -n 1)"
fi
echo "[*] Target Defconfig: ${DEFCONFIG}"
echo "[*] Parallel Jobs: ${JOBS}"

# Step 1: Generate .config
if [ ! -f "${OUT_DIR}/.config" ]; then
    echo "[*] Generating .config from ${DEFCONFIG}..."
    make -C "${KERNEL_DIR}" O="${OUT_DIR}" \
        CC=clang ${LLD_OPTS} CLANG_TRIPLE="${CLANG_TRIPLE}" \
        KCFLAGS=-w CONFIG_SECTION_MISMATCH_WARN_ONLY=y \
        "${DEFCONFIG}"
fi

# Step 2: Compile Image.gz
echo "[*] Compiling Linux 4.19.191 Kernel Image.gz..."
make -C "${KERNEL_DIR}" O="${OUT_DIR}" \
    CC=clang ${LLD_OPTS} CLANG_TRIPLE="${CLANG_TRIPLE}" \
    KCFLAGS=-w CONFIG_SECTION_MISMATCH_WARN_ONLY=y \
    Image.gz -j"${JOBS}"

if [ -f "${OUT_DIR}/arch/arm64/boot/Image.gz" ]; then
    cp "${OUT_DIR}/arch/arm64/boot/Image.gz" "${KERNEL_DIR}/Image.gz"
    echo ""
    echo "=========================================================="
    echo "[+] BUILD SUCCESSFUL!"
    echo "    Output Binary: ${KERNEL_DIR}/Image.gz"
    echo "    Size: $(stat -c%s "${KERNEL_DIR}/Image.gz") bytes"
    echo "=========================================================="
else
    echo "[-] Error: Compilation finished but Image.gz was not found."
    exit 1
fi
