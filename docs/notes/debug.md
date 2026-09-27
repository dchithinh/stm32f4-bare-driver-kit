# Debug with OpenOCD + GDB (no CubeIDE)

Stack: **ST-Link** on the board → **OpenOCD** (SWD server) → **arm-none-eabi-gdb** (CLI or Cursor).

Build with debug symbols (default top-level `CMAKE_BUILD_TYPE=Debug` adds **`-g3 -Og`**):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target dma_m2m
```

ELF path example: `build/examples/dma/m2m/dma_m2m.elf`.

## 1. Install tools

| Tool | Role |
|------|------|
| `arm-none-eabi-gcc` / `arm-none-eabi-gdb` | You already use GCC for build; GDB must match the toolchain. |
| `openocd` | Talks ST-Link SWD; exposes GDB on **port 3333**. |

### GDB on WSL / Ubuntu 24.04

**Symptom:** `arm-none-eabi-gcc` works, `arm-none-eabi-gdb: command not found`, and
`apt install gdb-arm-none-eabi` installs **`gdb-multiarch`** instead (normal on 24.04).

Pick one approach:

**A — Same folder as your `.exe` GCC (common on WSL)**

Tab-complete shows `arm-none-eabi-gdb.exe`. Run it explicitly:

```bash
arm-none-eabi-gdb.exe -q build/examples/dma/m2m/dma_m2m.elf
```

Or find the bin directory:

```bash
dirname "$(which arm-none-eabi-gcc)"
ls "$(dirname "$(which arm-none-eabi-gcc)")"/arm-none-eabi-gdb*
```

Optional alias in `~/.bashrc`:

```bash
alias arm-none-eabi-gdb='arm-none-eabi-gdb.exe'
```

**B — `gdb-multiarch` (apt package you already have)**

```bash
gdb-multiarch -q build/examples/dma/m2m/dma_m2m.elf
```

Inside GDB, before `target extended-remote`:

```gdb
set architecture arm
set arm force-mode thumb
```

**C — Linux-native Arm GNU Toolchain (no `.exe`)**

Install the **aarch64 linux** host package from [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads), unpack under e.g. `~/toolchains/`, add `.../bin` to `PATH` ahead of Windows tools. That tree includes `arm-none-eabi-gdb` without `.exe`.

OpenOCD:

```bash
sudo apt install openocd
```

Check:

```bash
arm-none-eabi-gdb --version
openocd --version
```

## 2. WSL2 + USB (Discovery ST-Link)

WSL does not see USB by default. On **Windows** (Admin PowerShell), with [usbipd](https://github.com/dorssel/usbipd-win):

```powershell
usbipd list
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

In WSL, `lsusb` should show **STMicroelectronics ST-LINK**. Re-attach after replug if needed.

Alternative: run **OpenOCD on Windows** and in WSL GDB use `target extended-remote <Windows-IP>:3333` (firewall must allow 3333).

## 3. Manual session (two terminals)

**Terminal A — OpenOCD** (leave running):

```bash
openocd -f cmake/openocd_stm32f407_stlink.cfg
```

Wait for: `Listening on port 3333 for gdb connections`.

**Terminal B — GDB**:

```bash
arm-none-eabi-gdb -q build/examples/dma/m2m/dma_m2m.elf
```

```gdb
target extended-remote :3333
monitor reset halt
load
break main
continue
```

After halt at `main`, inspect hardware:

```gdb
info registers
p/x $sp
p/x $pc
p/x DMA2_Stream0->CR
p/x DMA2_Stream0->NDTR
p/x DMA2_Stream0->PAR
p/x DMA2_Stream0->M0AR
p/x USART2->SR
p/x USART2->CR3
x/16xb dst
```

Peripheral names match CMSIS (`DMA1_Stream6`, `RCC->AHB1ENR`, …). Use `p/x` for hex.

Useful GDB:

| Command | Use |
|---------|-----|
| `next` / `step` | Line step / into function |
| `finish` | Run until current function returns |
| `continue` | Run until breakpoint |
| `bt` | Backtrace |
| `info breakpoints` | List breakpoints |
| `watch dma_m2m_done` | Break on variable change (if in scope) |
| `monitor reset halt` | Halt again via OpenOCD |
| `monit` | Shorthand for `monitor` |

Flash without GDB `load`: `cmake --build build --target dma_m2m` then `arm-none-eabi-objcopy` is already done by the kit; GDB `load` programs the same ELF.

## 4. Cursor / VS Code

1. Install extension **C/C++** (Microsoft).
2. Edit `.vscode/launch.json` **`program`** to your `.elf` (e.g. `gpio_blink.elf`, `dma_m2m.elf`).
3. **Run and Debug** → **BDK: OpenOCD + GDB**.
4. Set breakpoints in `main.c` or driver `.c` before starting.

OpenOCD is started by the debug adapter (`debugServerPath`). Only one debug session should own the ST-Link.

Optional: **Cortex-Debug** extension can load an **SVD** file for register trees in the UI; not required for `p/x DMA1_Stream6->CR` in the GDB console.

## 5. When things fail

| Symptom | Check |
|---------|--------|
| `Error: open failed` / no ST-Link | USB attach in WSL, cable, board powered |
| `Cannot identify target` | Wrong `target/stm32f4x.cfg`; use repo `openocd_stm32f407_stlink.cfg` |
| Breakpoints never hit | Build **Debug** with `-g`; `load` after change; break on a line in `main` |
| Optimized vars “optimized out” | Use **Debug** (`-Og`), or `volatile` / inspect registers directly |
| `p DMA...` unknown | Include path not needed in GDB — symbols come from ELF; use exact CMSIS name |

## 6. Without a debugger

You still have **serial** (UART examples), **scope/LA**, and **`BKPT` / infinite loop** in `main` for “stop here” during bring-up. GDB is for register-level checks (DMA `NDTR`, `USART->SR`, NVIC), not a replacement for serial proof.
