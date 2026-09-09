# Custom Kernel for Samsung Galaxy A06 (`SM-A065F`)
[![Kernel](https://img.shields.io/badge/Kernel-4.19.191-blue.svg)](https://kernel.org)
[![Android](https://img.shields.io/badge/Android-14%20(One%20UI%20Core%206.1)-green.svg)](https://source.android.com)
[![SoC](https://img.shields.io/badge/SoC-MediaTek%20Helio%20G85%20(MT6769V)-orange.svg)](https://www.mediatek.com)
[![License](https://img.shields.io/badge/License-GPL%20v2-lightgrey.svg)](LICENSE)

An optimized, production-grade custom Linux kernel tree for the **Samsung Galaxy A06 (`SM-A065F` / `SM-A065M`)**, built from official Samsung Open Source code with community performance modifications inspired by developer **physwizz**.

---

## Device Specifications

| Attribute | Specification |
| :--- | :--- |
| **Device Model** | Samsung Galaxy A06 (`SM-A065F` / `SM-A065M`) |
| **Chipset** | MediaTek MT6769V/CZ (Helio G85) |
| **CPU Configuration** | 2x ARM Cortex-A75 @ 2.0 GHz + 6x ARM Cortex-A55 @ 1.8 GHz |
| **GPU** | ARM Mali-G52 2EEMC2 |
| **Architecture** | AArch64 (`arm64`) |
| **Kernel Version** | Linux `4.19.191` (Non-GKI architecture, ramdisk in `boot.img`) |
| **Target Firmware** | `A065FXXS4AYE2` (One UI Core 6.1 / Android 14) |
| **Filesystem Layout** | EROFS dynamic partitions inside `super.img` |

---

## Features & Optimizations

### 1. Stripped Samsung DEFEX Anti-Tamper
* **Config:** `# CONFIG_SECURITY_DEFEX is not set`
* **Benefit:** Completely eliminates Samsung's kernel-level process interception driver (`fs/exec.c`). Stops the kernel from killing unauthorized root daemons, background shell scripts, or custom binaries, reducing context-switching latency.

### 2. Full Kprobes & APatch KPM Support
* **Config:** `CONFIG_KPROBES=y`, `CONFIG_KPROBE_EVENTS=y`, `CONFIG_BPF_SYSCALL=y`, `CONFIG_KALLSYMS_ALL=y`
* **Benefit:** Unlocks native runtime kernel symbol tracing and dynamic function hooking. Enables full support for **APatch KernelPatch Modules (KPM)** without needing to recompile the kernel.

### 3. Fast In-RAM zRAM (Fixing eMMC 5.1 Bottleneck)
* **Config:** `CONFIG_CRYPTO_LZ4=y`, `CONFIG_CRYPTO_ZSTD=y`, `CONFIG_ZRAM=y`
* **Benefit:** Replaces slow stock LZO compression with high-speed LZ4 and ZSTD. Allows keeping multitasking swap entirely compressed in physical RAM, completely bypassing slow eMMC 5.1 disk-swap freezes (Samsung RAM Plus).

### 4. Google BBR TCP Congestion Control
* **Config:** `CONFIG_TCP_CONG_BBR=y`, `CONFIG_DEFAULT_BBR=y`, `CONFIG_NET_SCH_FQ=y`
* **Benefit:** Replaces legacy Cubic TCP with Google's model-based BBR algorithm. Improves network throughput, reduces packet loss, and delivers lower latency over Wi-Fi and LTE/4G mobile data.

### 5. In-Kernel WireGuard VPN
* **Config:** `CONFIG_WIREGUARD=y`
* **Benefit:** Cryptographic VPN tunneling handled directly in C inside kernel space rather than slow userspace Go daemons, maximizing bandwidth and minimizing battery drain.

### 6. Modern Toolchain Compatibility
* Includes upstream patches to resolve pointer-conversion syntax in MediaTek touch common drivers (`drivers/input/touchscreen/mediatek/O8/touch_common/touch_feature.c`), enabling clean builds with modern LLVM/Clang (Clang 11 through Clang 17+).

---

## Repository Structure

```text
├── arch/arm64/configs/
│   └── a06_defconfig         # Master optimized kernel configuration
├── build.sh                  # Turnkey compilation script
├── .github/workflows/
│   └── build-kernel.yml      # Automated GitHub Actions CI workflow
├── drivers/                  # Hardware device drivers (MTK MT6769V)
├── fs/                       # Filesystems (EROFS, F2FS, Ext4, OverlayFS)
├── include/                  # Kernel header definitions
├── net/                      # Networking stack (BBR, WireGuard)
└── tools/                    # Repack and verification utilities
```

---

## How to Build

### 1. Install Build Dependencies (Ubuntu / Debian)
```bash
sudo apt update
sudo apt install -y build-essential gcc-aarch64-linux-gnu bison flex libssl-dev bc python3 git
```

### 2. Obtain a Toolchain
This kernel is designed to build with Google AOSP Clang or Android NDK LLVM:
* **Option A (Android NDK):** Use Clang from NDK r25+ (`toolchains/llvm/prebuilt/linux-x86_64/bin`).
* **Option B (AOSP Clang):** Download `clang-r383902` or `clang-r416183b`.

### 3. Build Using the Automated Script
```bash
chmod +x build.sh
./build.sh
```

### 4. Manual Compilation
```bash
export ARCH=arm64
export SUBARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
export CC=clang
export CLANG_TRIPLE=aarch64-linux-gnu-
export ANDROID_MAJOR_VERSION=r

# 1. Load defconfig
make O=out a06_defconfig

# 2. Compile kernel Image.gz
make O=out -j$(nproc) Image.gz
```

Output binary: `out/arch/arm64/boot/Image.gz`

---

## Flashing & Deployment

### 1. Repack into `boot.img`
Replace the stock kernel inside your firmware's `boot.img` using `tools/repack_boot.py`:
```bash
python3 tools/repack_boot.py --stock boot_stock.img --kernel out/arch/arm64/boot/Image.gz -o boot_custom.img
```

### 2. Package for Odin
Combine `boot_custom.img` with a disabled `vbmeta.img` (flags `0x02`):
```bash
tar -cvf AP_custom_kernel_AYE2.tar boot.img vbmeta.img
```

### 3. Flash to Device
1. Boot phone into **Download Mode** (Hold **Vol Up + Vol Down** while connecting USB $\rightarrow$ press **Vol Up**).
2. Flash `AP_custom_kernel_AYE2.tar` via Odin (Windows) in the **AP** slot, or on Linux via `odin4`:
   ```bash
   sudo ./odin4 -a AP_custom_kernel_AYE2.tar
   ```
3. Reboot normally.

> **Note**: If your device is already bootloader-unlocked with disabled vbmeta, flashing this custom kernel **will NOT wipe your data**.

---

## Post-Install Performance Tuning

To experience the maximum fluidity of this custom kernel:

1. **Disable Samsung RAM Plus:**
   * Go to *Settings $\rightarrow$ Device Care $\rightarrow$ Memory $\rightarrow$ RAM Plus $\rightarrow$ Toggle OFF* $\rightarrow$ Restart.
2. **Configure In-RAM zRAM (via SmartPack Kernel Manager):**
   * Set algorithm to **LZ4** or **ZSTD**.
   * Set zRAM size to **2048 MB** (50% of total RAM).
   * Set swappiness to **100**.
3. **Schedutil Big-Core Boost:**
   * Set `up_rate_limit_us` to `500` on CPU policy 6 for instantaneous touch response.

---

## Credits & Acknowledgements

* **Samsung Open Source Release Center (OSRC)** for the base kernel source.
* **physwizz** for pioneering custom kernel research across budget Samsung MTK devices.
* **The Android Open Source Project (AOSP)** & Linux Kernel contributors.
* **bmax121 & APatch Team** for KernelPatch and APatch.

---

## License

This project is licensed under the terms of the **GNU General Public License v2.0** ([GPL-2.0](LICENSE)).
