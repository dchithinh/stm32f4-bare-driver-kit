# UART echo

USART2 polling: **read PA3, write PA2**. Type in Tera Term; the same character should come back.

## USB–UART (PC TX → MCU RX → MCU TX → PC)

| Adapter | STM32 |
|---------|--------|
| TX      | **PA3** (USART2 RX) |
| RX      | **PA2** (USART2 TX) |
| GND     | GND |

Tera Term: **9600 8N1**. Do **not** jumper PA2 to PA3 while the adapter TX is on PA3 (two transmitters shorted).

## MCU loopback (TX → RX on the chip)

Jumper **PA2 to PA3**, adapter **disconnected** from those pins. Then this program waits on RXNE until something drives PA3 — so loopback echo still needs a **sender**. For a jumper-only test, use `tx` to send and a second program, or send then read in one app. This `echo` example is for the **PC adapter** path.

## Captures

```markdown
![echo](captures/echo.png)
```
