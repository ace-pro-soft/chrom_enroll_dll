# libche.dll — usage

A small Windows DLL with a single exported function.

## API

```c
BOOL __stdcall Activate(const char* key);
```

| | |
|---|---|
| **Parameter** | `key` — null-terminated UTF-8 / ASCII string. Must be non-empty and well-formed. |
| **Returns** | `TRUE` on success, `FALSE` otherwise. |
| **Calling convention** | `__stdcall` (WINAPI). Export name is undecorated. |
| **Notes** | May show a Windows consent (UAC) prompt; the call blocks until the user responds, so call it off your UI thread. Requires the matching architecture (x64 app → x64 DLL). |

## Load dynamically (recommended)

### Qt C++
```cpp
#include <QLibrary>

typedef BOOL (__stdcall *Fn)(const char*);

bool run(const QString &key) {
    QLibrary lib("libche");                 // libche.dll next to your .exe
    if (!lib.load()) return false;
    auto fn = reinterpret_cast<Fn>(lib.resolve("Activate"));
    if (!fn) return false;
    return fn(key.toUtf8().constData()) != 0;
}
```

### Plain C / C++
```c
#include <windows.h>
typedef BOOL (__stdcall *Fn)(const char*);

HMODULE h = LoadLibraryW(L"libche.dll");
Fn fn = (Fn)GetProcAddress(h, "Activate");
BOOL ok = fn("your-key");
FreeLibrary(h);
```

### C# (P/Invoke)
```csharp
[DllImport("libche.dll", CallingConvention = CallingConvention.Winapi)]
static extern bool Activate([MarshalAs(UnmanagedType.LPStr)] string key);
```

### Python (ctypes)
```python
import ctypes
dll = ctypes.WinDLL(r".\libche.dll")
dll.Activate.argtypes = [ctypes.c_char_p]
dll.Activate.restype  = ctypes.c_int
ok = dll.Activate(b"your-key")
```

## Static link (optional)

Link `libche.lib`, include `che.h`, and call `Activate(...)` directly.
`libche.dll` must still sit next to your `.exe` at runtime.

## Build

```bat
build.bat            REM x64 -> build\x64\libche.dll
build.bat x86        REM x86 -> build\x86\libche.dll
```

Built with the static CRT — no Visual C++ redistributable needed. Prebuilt
binaries are in `dist/`.

## Deploy

Ship `libche.dll` in the same folder as your application `.exe`, matching your
app's architecture (x64 or x86).
