# Prebuilt `libche.dll`

Pick the folder matching your app's architecture and ship `libche.dll` next to
your `.exe`.

```
x64/libche.dll      x64/libche.lib      64-bit app
x86/libche.dll      x86/libche.lib      32-bit app
include/che.h                           public header (same for both)
```

- Built with the static CRT — no Visual C++ redistributable required.
- `*.lib` is the import library (only needed for load-time linking; dynamic
  `LoadLibrary`/`QLibrary` loading does not need it).
- Single export: `BOOL __stdcall Activate(const char* key)` — returns `TRUE`
  only on success, `FALSE` otherwise.

Rebuild from source any time with `build.bat`.
