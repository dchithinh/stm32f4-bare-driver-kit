# UART ring_drop test

USART2 **PA2/PA3**, **115200**, RX IRQ on. **5 s countdown** — main does not call `poll_in`; ISR fills the 64-byte ring.

**Tera Term only:** paste 128+ characters (avoid Enter if you want exactly 128 bytes).

Build: `cmake --build build --target uart_ring_drop`

After countdown: `ring_drop=…` (about **sent − 63** for one fast paste; `ore` should stay **0**).
