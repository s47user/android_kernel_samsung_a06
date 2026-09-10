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

### 1. Integrated KernelSU-Next (Manual Source Hooks)
* **Config:** `CONFIG_KSU=y`, `CONFIG_KSU_MANUAL_HOOK=y`
* **Benefit:** Next-generation kernel-based root solution integrated directly into kernel source calls (`fs/exec.c`, `fs/open.c`, `fs/stat.c`, `fs/read_write.c`, `drivers/input/input.c`, `kernel/reboot.c`). Grants root privileges without modifying system partitions or ramdisk, completely invisible to standard userspace root detectors.

### 2. Anti-Detection Kernel Identity Spoofing
* **Config / Env:** `KBUILD_BUILD_USER=dpi`, `KBUILD_BUILD_HOST=21DKGB11`, `CONFIG_LOCALVERSION="-29401052-abA065FXXS4AYE2"`, `# CONFIG_LOCALVERSION_AUTO is not set`
* **Benefit:** Matches the exact stock Samsung production build banner (`dpi@21DKGB11`), eliminating the `-dirty` git flag and any custom commit hashes. Banking, enterprise, and integrity detection apps reading `/proc/version` see an authentic factory Samsung kernel.

### 3. Stripped Kernel Debugging Symbols
* **Config:** `# CONFIG_DEBUG_INFO is not set`
* **Benefit:** Removes all unneeded DWARF debugging tables from the kernel elf binary, drastically reducing `vmlinux` size from **276 MB down to 37 MB** while speeding up linking and reducing memory overhead.

### 4. Stripped Samsung DEFEX Anti-Tamper
* **Config:** `# CONFIG_SECURITY_DEFEX is not set`
* **Benefit:** Completely eliminates Samsung's kernel-level process interception driver (`fs/exec.c`). Stops the kernel from killing unauthorized root daemons, background shell scripts, or custom binaries, reducing context-switching latency.

### 5. Full Kprobes & APatch KPM Support
* **Config:** `CONFIG_KPROBES=y`, `CONFIG_KPROBE_EVENTS=y`, `CONFIG_BPF_SYSCALL=y`, `CONFIG_KALLSYMS_ALL=y`
* **Benefit:** Unlocks native runtime kernel symbol tracing and dynamic function hooking. Enables full support for **APatch KernelPatch Modules (KPM)** without needing to recompile the kernel.

### 6. Fast In-RAM zRAM (Defaulting to LZ4)
* **Config:** `CONFIG_CRYPTO_LZ4=y`, `CONFIG_CRYPTO_ZSTD=y`, `CONFIG_ZRAM=y`
* **Benefit:** Configured with ultra-fast LZ4 compression engine by default. Allows keeping multitasking swap entirely compressed in physical RAM, completely bypassing slow eMMC 5.1 disk-swap freezes (Samsung RAM Plus).

### 7. Google BBR TCP Congestion Control
* **Config:** `CONFIG_TCP_CONG_BBR=y`, `CONFIG_DEFAULT_BBR=y`, `CONFIG_NET_SCH_FQ=y`
* **Benefit:** Replaces legacy Cubic TCP with Google's model-based BBR algorithm. Improves network throughput, reduces packet loss, and delivers lower latency over Wi-Fi and LTE/4G mobile data.

### 8. In-Kernel WireGuard VPN
* **Config:** `CONFIG_WIREGUARD=y`
* **Benefit:** Cryptographic VPN tunneling handled directly in C inside kernel space rather than slow userspace Go daemons, maximizing bandwidth and minimizing battery drain.

### 9. Compact LLD 17 Linking & RELR Relocation Packing
* **Config:** `CONFIG_LD_IS_LLD=y`, `CONFIG_TOOLS_SUPPORT_RELR=y`, `CONFIG_RELR=y`
* **Benefit:** Links the kernel using LLVM LLD 17.0.2 with experimental relative relocation (`RELR`) packing. Shrinks uncompressed kernel memory footprint from **36.0 MB to 30.1 MB** (saving **~6.0 MB** of physical RAM), matching stock Samsung memory density while using modern Clang 17.

### 10. Modern Toolchain Compatibility
* Includes upstream patches to resolve pointer-conversion syntax in MediaTek touch common drivers (`drivers/input/touchscreen/mediatek/O8/touch_common/touch_feature.c`), enabling clean builds with modern LLVM/Clang (Clang 11 through Clang 17+).

---

## Repository Structure

```text
├── arch/arm64/configs/
│   └── a06_defconfig         # Master optimized kernel configuration (LLD + RELR)
├── build.sh                  # Turnkey compilation script with LLD auto-detection
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
sudo apt install -y build-essential gcc-aarch64-linux-gnu bison flex libssl-dev bc python3 git lld clang
```

### 2. Obtain a Toolchain
This kernel is designed to build with Google AOSP Clang or Android NDK LLVM:
* **Option A (Android NDK):** Use Clang and LLD from NDK r26+ (`toolchains/llvm/prebuilt/linux-x86_64/bin`).
* **Option B (AOSP Clang):** Download `clang-r416183b` or `clang-r487747e`.

### 3. Build Using the Automated Script
```bash
chmod +x build.sh
./build.sh
```

---

## Flashing & Deployment

### ⚠️ Clean Ramdisk Notice (Magisk Coexistence Warning)
KernelSU lives directly inside the kernel binary, whereas Magisk patches the ramdisk. If migrating from Magisk, **do not** flash KernelSU over a Magisk-patched boot image! `boot_custom.img` is packaged with the clean factory stock Samsung ramdisk (`boot.img`), preventing permission deadlocks.

### Method 1: Direct On-Device Flashing (Terminal / ADB)
Because the device has an unlocked bootloader and disabled dm-verity (`vbmeta_disabled`), you can flash directly:
```bash
adb push boot_custom.img /sdcard/
adb shell
su
dd if=/sdcard/boot_custom.img of=/dev/block/by-name/boot bs=4096
reboot
```

### Method 2: Odin4 / Download Mode (PC)
Flash the repacked tar package containing `boot.img` and `vbmeta.img`:
```bash
./odin4 -a AP_custom_kernel_AYE2.tar
```

### Installing the KernelSU Manager
After booting into system:
1. Download and install **[KernelSU-Next Manager](https://github.com/KernelSU-Next/KernelSU-Next/releases)** (`KernelSU_Next_v3.3.0.apk`).
2. Open KernelSU Next Manager to confirm:
   * Status: **Working**
   * Kernel Version: `4.19.191-29401052-abA065FXXS4AYE2`
   * Hook mode: **Manual**
3. Complete the uninstallation of Magisk through the Magisk app (Uninstall $\rightarrow$ Complete Uninstall) if migrating.

---

## Post-Install Performance Tuning

To experience the maximum fluidity of this custom kernel:

1. **Disable Samsung RAM Plus:**
   * Go to *Settings $\rightarrow$ Device Care $\rightarrow$ Memory $\rightarrow$ RAM Plus $\rightarrow$ Toggle OFF* $\rightarrow$ Restart.
2. **Configure via [SmartPack Kernel Manager (Galaxy A06 Edition)](https://github.com/s47user/SmartPack-Kernel-Manager-A06):**
   * Install the custom manager from [Releases](https://github.com/s47user/SmartPack-Kernel-Manager-A06/releases/tag/v17.7-A06).
   * **1-Tap Fluidity:** Toggle *⚡ Galaxy A06 Fluidity Mode* in the CPU tab (schedutil `up_rate_limit_us` $\rightarrow$ 500µs on policy 6, 512KB eMMC readahead).
   * **In-RAM zRAM:** Set algorithm to **LZ4** and size to **2048 MB**.

---

## Credits & Acknowledgements

Special thanks and sincere credit to the developers and communities whose open-source work made this kernel possible:

* **Samsung Open Source Release Center (OSRC)** for releasing the stock BSP kernel source code (`SM-A065F_14_Opensource_A065FXXS4AYE2`).
* **tiann & The KernelSU Team** for inventing KernelSU and kernel-level su-domain permission design.
* **rifsxd & pershoot (KernelSU-Next Team)** for KernelSU-Next legacy architecture, manual hooking pipelines, and non-GKI backports.
* **physwizz** for pioneering custom kernel research, governor tuning, and optimization principles on MediaTek Helio G85 devices.
* **topjohnwu & The Magisk Team** for Magisk, `magiskboot`, and the sepolicy live-injection architecture enabling root under `Enforcing` SELinux.
* **bmax121 & The APatch Team** for KernelPatch, dynamic Kprobes symbol hooking, and the APatch ecosystem.
* **Jason A. Donenfeld & The WireGuard Project** for the in-kernel cryptographic VPN tunneling protocol.
* **Google AOSP & Linux Kernel Maintainers** for BBR congestion control, Fair Queuing pacing (`sch_fq`), and LLVM Clang toolchain support.
* **Willi Ye (Grarak) & The SmartPack Team** for Kernel Adiutor and SmartPack Kernel Manager.

---

## License

This project is licensed under the terms of the **GNU General Public License v2.0** ([GPL-2.0](LICENSE)).
