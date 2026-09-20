# Examples

Grouped by **driver**, then by **app**. Each app can keep scope / logic-analyzer
shots in `captures/` and embed them in that app’s `README.md`.

```
examples/
  <driver>/              # gpio, uart, i2c, spi, timer, …
    CMakeLists.txt       # add_subdirectory(<app>) for every app
    <app>/
      CMakeLists.txt
      main.c
      README.md          # setup, probe points, measurements
      captures/          # .png / .jpg from the scope or analyzer
```

Add a driver: create `examples/<driver>/`, add `add_subdirectory(<driver>)` in
`examples/CMakeLists.txt`, then add apps the same way under that driver.

New app CMakeLists is the usual ~10 lines (`bdk_sdk_import.cmake` is three
levels up: `examples/<driver>/<app>/`).
