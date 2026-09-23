# UART TX

Polling USART2 TX on PA2 (Discovery / USB–UART). Sends `HELLO!` at 9600 8N1.

## Probe

- PA2 (TX) vs GND, or USB–UART RX ← PA2, Tera Term 9600 8N1
- PicoScope: UART decode 9600, 8N1, LSB first; ~200 µs/div per character

## Captures

```markdown
![HELLO](captures/hello_uart.png)
```
