# DMA notes

RM0090: DMA chapter (`SxCR`, `SxNDTR`, `SxPAR`, `SxM0AR`, `LISR`/`HISR`,
`LIFCR`/`HIFCR`). DMA1/DMA2 **request mapping** tables (stream + CHSEL per
peripheral). Code: `inc/bdk_dma.h`, `src/bdk_dma.c`. Lab app: `examples/uart/dma/`
(USART2 PA2/PA3, 115200, F407 Discovery).

## Terms


| Term                            | Meaning                                                                             |
| ------------------------------- | ----------------------------------------------------------------------------------- |
| DMA1 / DMA2                     | Controllers; RCC `AHB1ENR` **DMA1EN** / **DMA2EN**                                  |
| Stream 0–7                      | Per controller (`DMA1_Stream0` … in CMSIS)                                          |
| `channel` in `bdk_dma_config_t` | **CHSEL** — peripheral request on that stream (RM table)                            |
| `bdk_dma_stream_t`              | `BDK_DMA1_STREAM(n)` / `BDK_DMA2_STREAM(n)` or `BDK_DMA_STREAM(ctrl, n)`; not CHSEL |


USART2 (check RM table):


| Direction | controller | stream | CHSEL |
| --------- | ---------- | ------ | ----- |
| TX        | `BDK_DMA1` | 6      | 4     |
| RX        | `BDK_DMA1` | 5      | 4     |


UART peripheral address: `&USARTx->DR`, byte **PSIZE** / **MSIZE**.

RCC: no `bdk_dma_init`. Set **DMA1EN** or **DMA2EN** in `bdk_dma_config` from
`cfg->stream.controller`.

## `bdk_dma_config_t` → registers


| Field                       | `SxCR` / other |
| --------------------------- | -------------- |
| `channel`                   | CHSEL          |
| `direction`                 | DIR            |
| `periph_addr`               | PAR            |
| `periph_inc`                | PINC           |
| `mem_inc`                   | MINC           |
| `periph_width`, `mem_width` | PSIZE, MSIZE   |
| `circular`                  | CIRC           |


Configure with **EN = 0** (RM). **EN** and `M0AR` / `NDTR` in `bdk_dma_start`.

## Implementation order



### 1. `bdk_dma_config`

- Map `(controller, stream)` → `DMA_Stream_TypeDef *` (index 0–7 per controller);
use `bdk_dma_stream_valid`; → `BDK_ERR_NULL` / `BDK_ERR_RANGE`.
- Enable **DMA1EN** or **DMA2EN** for that stream’s controller.
- Program `CR` / `PAR` from `cfg`. Return `BDK_ERR_BUSY` if stream already **EN**.

Verify in debugger: e.g. `DMA1_Stream6->PAR == &USART2->DR`, CHSEL = 4, **EN** = 0.

### 2. `bdk_dma_start`, `bdk_dma_stop`, `bdk_dma_busy`

- `start`: `M0AR`, `NDTR`, then **EN**; `BDK_ERR_BUSY` if active.
- `busy`: non-zero while transfer in progress (**EN** and/or `NDTR` — document which).
- `stop`: clear **EN** per RM.

First hardware test (USART2 TX, polling only):

1. App: GPIO + `bdk_uart_init`, `bdk_uart_dma_bind` (table above).
2. `bdk_dma_config` mem→periph, `PAR = &USART2->DR`, MINC=1, PINC=0.
3. USART **DMAT** (`CR3`).
4. `bdk_dma_start(BDK_DMA1_STREAM(6), buf, len)`; wait on `bdk_dma_busy` or poll **TC** in `LISR`/`HISR`.

No USART **TXEIE**, no DMA IRQ yet. Serial should show the buffer.

### 3. `bdk_dma_irq_enable`, `bdk_dma_irq_handler`

- `bdk_dma_irq_enable(&tx_stream, BDK_DMA_IT_TC)` (add `BDK_DMA_IT_TE` if you want error IRQs); NVIC e.g. `DMA1_Stream6_IRQn` (lab TX).
- App: `DMA1_Stream6_IRQHandler` → `bdk_dma_irq_handler(&tx_stream)`.
- Handler: **TEIF** first → clear **EN**, then clear all stream flags in `LIFCR`/`HIFCR` (TC/HT/TE/DME/FE). **TCIF** → clear **CTCIF** only; completion state for the app (no `stop` on normal TC).



### 4. `bdk_uart_write_dma`, `dma_tx_active`

- Use bound TX stream/channel; `bdk_dma_config` + `bdk_dma_start`; **DMAT**.
- `dma_tx_active` ↔ TX stream busy; `BDK_ERR_BUSY` on overlap; buffer alive until inactive (same as `write_async`).



### M2M lab (`examples/dma/m2m`)

- `direction = BDK_DMA_MEM_TO_MEM`; **PAR** = source, **M0AR** = destination in `bdk_dma_start`.
- **PINC** + **MINC**, byte **PSIZE**/**MSIZE**; no peripheral **DMAT**.
- Example uses **DMA2 stream 0** so it does not share DMA1 stream 6 with USART2 TX labs.
- TC IRQ: `DMA2_Stream0_IRQHandler` → `bdk_dma_irq_handler`; main waits on `dma_m2m_done`.

### 5. RX (after TX)

- Stream5, periph→mem, **DMAR**; `DMA1_Stream5_IRQHandler`; `read_dma` / `dma_rx_active`. See `uart.md` for **ORE**.



## Where code lives


| Symbol                              | File             |
| ----------------------------------- | ---------------- |
| `bdk_dma_*`                         | `src/bdk_dma.c`  |
| `bdk_uart_dma_bind`, `write_dma`, … | `src/bdk_uart.c` |
| `DMAx_StreamN_IRQHandler`           | App              |




## Failures


| Symptom         | Likely cause                                                            |
| --------------- | ----------------------------------------------------------------------- |
| No output       | `UE`/`TE`, GPIO AF, baud, **DMAT**, DMA **EN**, `NDTR` not decrementing |
| Garbage         | Wrong PSIZE/MSIZE, `PAR`, CHSEL/stream                                  |
| Stuck in `busy` | **TC** not cleared, **EN** stuck, wrong request mapping                 |
| IRQ loop        | Wrong `CTCIFx` in `LIFCR`/`HIFCR`                                       |
| Fault           | Bad `M0AR`, buffer freed before complete                                |


