# Prebuilt `libche.dll`

Drop-in binaries. Pick the folder that matches your app's architecture and ship
`libche.dll` next to your `.exe`.

```
x64/libche.dll      x64/libche.lib      64-bit app
x86/libche.dll      x86/libche.lib      32-bit app
include/chrome_enroll.h                 public header (same for both)
```

- Built with the static CRT — no Visual C++ redistributable required.
- `*.lib` is the import library (only needed for load-time linking; dynamic
  `LoadLibrary`/`QLibrary` loading does not need it).
- API, integration samples (C / C# / Python / Qt), and behavior: see
  [`../USAGE.md`](../USAGE.md).

These are a convenience snapshot; rebuild from source any time with `build.bat`.
