# GPIO blink

Discovery: PD14 red LED, PD12 green LED. Output toggle after `bdk_gpio_init`.

## Probe

- Channel 1: PD14 (or the LED pad) vs GND
- Expect a square wave; period is the software delay loop, not a timer

## Captures

Drop scope screenshots in `captures/` (PNG or JPEG) and link them here:

```markdown
![PD14 toggle](captures/pd14_toggle.png)
```
