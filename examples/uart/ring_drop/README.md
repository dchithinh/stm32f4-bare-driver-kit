# UART ring_drop test

## Purpose

Stress the **IRQ RX ring** (64 bytes): bytes arrive faster than main reads them.

## What it proves

- **`ring_drop_count`** increases when more than 64 bytes arrive before `poll_in` consumes them (drops, not UART **ORE** if the ring is the bottleneck).
- **`ore_count`** should stay 0 for a clean paste test; driver reports both via `bdk_uart_rx_stats_get`.

## How to run

**115200** 8N1. After a **5 s** countdown, paste **128+** characters quickly in Tera Term (avoid extra Enter if you want a fixed byte count).

## Expected

After countdown, one line like `ring_drop=N  (ore=M)` — **N** should be **> 0** for a long fast paste; **M** stays 0 if the UART itself did not overrun.

## Build

`cmake --build build --target uart_ring_drop`
