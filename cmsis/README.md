# cmsis

Unmodified CMSIS headers, used as-is (the only vendor headers this kit allows).
Not HAL, not LL, not a general third-party library dump.

| Path | Origin | Role |
|------|--------|------|
| `cmsis_core/` | [STMicroelectronics/cmsis_core](https://github.com/STMicroelectronics/cmsis_core) (Apache-2.0) | Cortex-M4 core (`core_cm4.h`, compiler helpers) |
| `cmsis_device_f4/` | [STMicroelectronics/cmsis_device_f4](https://github.com/STMicroelectronics/cmsis_device_f4) (Apache-2.0) | `stm32f407xx.h` register map and base addresses |

Only the F407 / Cortex-M4 files needed to compile are kept. Do not add HAL or LL.
