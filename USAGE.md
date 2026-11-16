# libche.dll — Chrome Browser Cloud Management enrollment

A single Windows DLL that enrolls the machine's Chrome into **Chrome Browser
Cloud Management (CBCM)** from one function call. It takes only the enrollment
token, handles the admin/UAC elevation itself, performs the privileged registry
write, verifies it, and returns a boolean.

> **This is a plain Win32 C-ABI DLL — not Qt-specific.** It has no Qt
> dependency, no C++ runtime dependency (static CRT), and exports undecorated C
> functions. It can be called from **any Windows language**: C, C++, C#/.NET,
> Python, Rust, Go, Delphi, VBScript/AutoIt, PowerShell, etc. The Qt example
> below is just one of several — see **Calling it from any language**.

---

## What it actually does

Chrome enrolls into CBCM when this machine-wide policy value is present and
Chrome is (re)launched:

```
HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Google\Chrome
    CloudManagementEnrollmentToken  (REG_SZ) = <your enrollment token>
```

Writing under `HKLM` requires administrator rights. The DLL:

1. Checks whether the calling process is **elevated**.
2. **Elevated** → writes the value directly.
3. **Not elevated** → raises the **UAC consent prompt**. If the user accepts, an
   elevated helper (a hidden `rundll32.exe` instance of this same DLL) writes the
   value. If the user cancels, nothing is written.
4. Reads the value back to confirm, and returns the result.

Enrollment completes the next time Chrome starts; Chrome consumes the token and
registers the device with your Google Admin console. No reboot is required.

---

## API

Three equivalent exports — pick whichever string type your language uses:

```c
// chrome_enroll.h   (all __stdcall, all return BOOL)
BOOL EnrollChromeBrowser (const wchar_t* enrollmentToken);      // wide (UTF-16)
BOOL EnrollChromeBrowserW(const wchar_t* enrollmentToken);      // identical alias
BOOL EnrollChromeBrowserA(const char*    enrollmentTokenUtf8);  // narrow (UTF-8/ASCII)
```

| | |
|---|---|
| **Parameter** | The CBCM enrollment token from the Google Admin console (non-empty). Wide entry points take UTF-16; the `A` entry point takes UTF-8 (plain ASCII works, since tokens are ASCII). |
| **Returns `TRUE`** | **only if** the enrollment key is valid **and** admin rights were obtained (already elevated, or the UAC prompt was accepted) **and** the token was written to the registry and verified. |
| **Returns `FALSE`** | if the key is invalid/malformed, **or** the UAC prompt was declined, **or** the registry write/verify failed. Nothing is written in any of these cases. |
| **Calling convention** | `__stdcall` (WINAPI). This is the default most Windows FFIs assume (.NET `CallingConvention.Winapi`, Python `ctypes.WinDLL`). Keep it in any manual typedef — it matters on 32-bit. |
| **Exported names** | Undecorated on both x86 and x64 (a `.def` file keeps them clean), so `GetProcAddress`/`resolve` by name works everywhere. |
| **Blocking** | When a UAC prompt is shown, the call blocks until the user responds. Call it from a worker thread if you don't want to block a UI thread. |

> There is one more export, `EnrollElevatedW`. It is the internal `rundll32`
> entry point used by the elevated helper. **Do not call it from your app.**

---

## Files

| File | Purpose |
|------|---------|
| `chrome_enroll.h`   | Public header for your app. |
| `chrome_enroll.cpp` | Implementation. |
| `token_rules.h`     | Enrollment-key normalization + validity rule (the one place to tweak what "valid" means). |
| `chrome_enroll.def` | Export list. |
| `build.bat`         | MSVC build script. |
| `CMakeLists.txt`    | CMake build (alternative). |
| `tests_token.cpp`   | Unit test for the key-validity rule (no admin/registry needed). |
| `test_enroll.cpp`   | Tiny standalone tester (loads the DLL and calls the export). |

Build output lands in `build\<arch>\`:
`libche.dll`, `libche.lib` (import lib), `libche.exp`.

---

## Building the DLL

The DLL is built with the static CRT (`/MT`), so **it has no Visual C++
redistributable dependency** — just ship the `.dll`.

### Option A — `build.bat` (MSVC Build Tools)

```bat
build.bat            REM 64-bit  -> build\x64\libche.dll
build.bat x86        REM 32-bit  -> build\x86\libche.dll
```

### Option B — CMake

```bat
cmake -A x64 -S . -B build-cmake        REM or: -A Win32
cmake --build build-cmake --config Release
```

> **Architecture must match your Qt app.** A 64-bit Qt process can only load the
> x64 DLL; a 32-bit Qt process needs the x86 DLL. Build the one(s) you need.

---

## Calling it from any language

The DLL exports plain C functions, so the pattern is identical everywhere: load
the DLL, resolve `EnrollChromeBrowser` (wide) or `EnrollChromeBrowserA` (UTF-8),
call it with the `__stdcall` convention, check the boolean. Ship the DLL next to
your executable and match the architecture (x64 app → x64 DLL).

### Plain C / C++ (no Qt, dynamic load)

```c
#include <windows.h>
#include <stdio.h>

typedef BOOL (__stdcall *EnrollFn)(const wchar_t*);

int main(void) {
    HMODULE h = LoadLibraryW(L"libche.dll");
    EnrollFn enroll = (EnrollFn)GetProcAddress(h, "EnrollChromeBrowser");
    BOOL ok = enroll(L"YOUR-ENROLLMENT-TOKEN");
    printf(ok ? "enrolled\n" : "failed/cancelled\n");
    FreeLibrary(h);
    return ok ? 0 : 1;
}
```

Or link against `libche.lib` and just `#include "chrome_enroll.h"` and
call `EnrollChromeBrowser(...)` directly.

### C# / .NET (P/Invoke)

```csharp
using System.Runtime.InteropServices;

static class ChromeEnroll {
    // Unicode => P/Invoke binds to EnrollChromeBrowserW; Winapi => __stdcall.
    [DllImport("libche.dll", CharSet = CharSet.Unicode,
               CallingConvention = CallingConvention.Winapi)]
    public static extern bool EnrollChromeBrowser(string enrollmentToken);
}

bool ok = ChromeEnroll.EnrollChromeBrowser("YOUR-ENROLLMENT-TOKEN");
```

### Python (ctypes)

```python
import ctypes
# WinDLL = __stdcall. Use the A entry point so a normal str works directly.
dll = ctypes.WinDLL(r".\libche.dll")
dll.EnrollChromeBrowserA.argtypes = [ctypes.c_char_p]
dll.EnrollChromeBrowserA.restype  = ctypes.c_int
ok = dll.EnrollChromeBrowserA(b"YOUR-ENROLLMENT-TOKEN")
print("enrolled" if ok else "failed/cancelled")
```

> Any other language that can call a Windows DLL (Rust `extern "stdcall"`, Go
> `syscall`, Delphi `stdcall`, AutoIt `DllCall`, PowerShell `Add-Type`) works
> the same way — resolve the export, pass the token, read the BOOL.

---

## Using it from Qt C++

Both approaches are standard; **dynamic loading is recommended** so your app
still starts if the DLL is missing, and you can report that cleanly.

### 1. Dynamic load with `QLibrary` (recommended)

```cpp
#include <QLibrary>
#include <QString>

// Must match the DLL's calling convention and signature exactly.
typedef int (__stdcall *EnrollFn)(const wchar_t*);

bool enrollChrome(const QString &token)
{
    // Looks for libche.dll next to your .exe (and on PATH).
    QLibrary lib("chrome_enroll");
    if (!lib.load())
        return false;   // lib.errorString() tells you why

    auto enroll = reinterpret_cast<EnrollFn>(lib.resolve("EnrollChromeBrowser"));
    if (!enroll)
        return false;

    // QString is UTF-16 internally; utf16() gives a null-terminated wchar_t*.
    const bool ok = enroll(reinterpret_cast<const wchar_t*>(token.utf16())) != 0;

    lib.unload();
    return ok;
}
```

Because the call can show a UAC dialog and block, run it off the UI thread:

```cpp
#include <QtConcurrent>
#include <QFutureWatcher>

auto *watcher = new QFutureWatcher<bool>(this);
connect(watcher, &QFutureWatcher<bool>::finished, this, [=]{
    const bool ok = watcher->result();
    // update UI: enrollment succeeded / was cancelled or failed
    watcher->deleteLater();
});
watcher->setFuture(QtConcurrent::run(enrollChrome, myToken));
```

### 2. Static (load-time) link

Put `chrome_enroll.h` and `libche.lib` where your build can find them.

**qmake (`.pro`):**
```pro
INCLUDEPATH += $$PWD/chrome_enroll
LIBS        += -L$$PWD/chrome_enroll/build/x64 -llibche
```

**CMake:**
```cmake
target_include_directories(your_app PRIVATE path/to/chrome_enroll)
target_link_libraries(your_app PRIVATE path/to/build/x64/libche.lib)
```

**Code:**
```cpp
#include "chrome_enroll.h"

const bool ok =
    EnrollChromeBrowser(reinterpret_cast<const wchar_t*>(token.utf16())) != 0;
```

`libche.dll` must still sit next to your `.exe` at runtime.

> **MSVC vs MinGW:** link-time (`.lib`) linking is easiest from an MSVC-built Qt.
> From a **MinGW** Qt, prefer the dynamic `QLibrary` approach — MinGW can load
> the MSVC-built DLL fine, it just can't consume the MSVC import `.lib`.

---

## Deployment

- Ship `libche.dll` in the **same folder as your application `.exe`**.
- **Renaming the DLL is safe.** It resolves its own path at runtime for the
  self-elevation step, so it works under any filename. Just make sure the name
  your app loads (`LoadLibrary`/`QLibrary`/`DllImport`) matches the file on disk.
  If you link statically via the import `.lib`, rebuild the DLL under the new
  name so the `.lib` records it.
- No VC++ redistributable is needed (static CRT).
- The matching architecture (x64/x86) must equal your app's.
- Your app does **not** need an elevated manifest — the DLL elevates on demand.
  If your app is already running elevated, no UAC prompt appears and the write is
  silent.

---

## Testing it manually

`test_enroll.cpp` loads the DLL and calls the export. Build it from an MSVC
"x64 Native Tools" prompt in this folder:

```bat
cl /EHsc /MT /DUNICODE /D_UNICODE test_enroll.cpp /Fe:build\x64\test_enroll.exe
```

Run it from `build\x64` (so it finds the DLL):

```bat
cd build\x64
test_enroll.exe YOUR-ENROLLMENT-TOKEN
```

- Run it **non-elevated** to see the UAC prompt and the accept/cancel behavior.
- Verify the result:
  ```
  reg query "HKLM\SOFTWARE\Policies\Google\Chrome" /v CloudManagementEnrollmentToken
  ```
- In Chrome, visit `chrome://policy` (reload policies) and
  `chrome://management` to confirm the browser is cloud-managed after a restart.

### Reverting a test enrollment

```
reg delete "HKLM\SOFTWARE\Policies\Google\Chrome" /v CloudManagementEnrollmentToken /f
```

(If this was the only value you added and the key was created by the test, you
can also delete the empty `...\Google\Chrome` key.)

---

## Behavior & edge cases

| Situation | Result |
|-----------|--------|
| Invalid / malformed / empty key | Returns `FALSE` immediately — **no UAC prompt, nothing written**. |
| Valid key, caller already elevated | Writes directly, no prompt → `TRUE`. |
| Valid key, not elevated, user **accepts** UAC | Helper writes the token, verified → `TRUE`. |
| Valid key, not elevated, user **cancels** UAC | Nothing written → `FALSE`. |
| Valid key written but read-back mismatch/failure | `FALSE`. |
| Re-enrolling with a new valid key | Overwrites the existing value → `TRUE`. |

### What counts as a valid key

The key is validated **before** any elevation prompt or registry write. A valid
CBCM enrollment token is a **UUID** in `8-4-4-4-12` form, e.g.
`a1b2c3d4-5678-90ab-cdef-1234567890ab` (case-insensitive; surrounding spaces,
quotes, or `{ }` braces are trimmed automatically). Anything else returns
`FALSE`.

> This rule lives in one place — `token_rules.h`, function
> `IsValidEnrollmentToken()`. If your organization ever issues tokens in a
> different format, relax or replace that function and rebuild; nothing else
> changes. It is covered by `tests_token.cpp` (`cl /EHsc /MT tests_token.cpp`).

---

## Security notes

- The enrollment token is a **machine-wide** policy (`HKLM`); every Chrome
  profile/user on the machine is covered. This is the intended CBCM model.
- During the non-elevated path, the token is passed to the elevated helper on
  the `rundll32` command line. It exists only for the brief lifetime of that
  hidden helper process. An enrollment token is an onboarding secret (it lets a
  browser join your org's CBCM), so treat it with the same care as other config
  secrets — avoid logging it. If your threat model forbids tokens ever appearing
  on a command line, run your app elevated so the direct-write path is used.
- The DLL writes exactly one value and reads it back; it makes no network calls.
  The actual enrollment handshake is performed by Chrome itself on next launch.

---

## Troubleshooting

| Symptom | Likely cause / fix |
|---------|--------------------|
| `QLibrary::load()` fails | DLL not next to the exe, or architecture mismatch (x64 app ↔ x86 DLL). |
| `resolve("EnrollChromeBrowser")` returns null | Wrong DLL, or name decorated — rebuild with the provided `.def`. |
| Always returns `FALSE` without a prompt | App is running non-elevated but UAC is disabled/blocked by policy, so elevation can't be obtained. Run the app elevated. |
| Returns `TRUE` but Chrome isn't managed | Chrome hasn't been restarted yet, or the token is invalid/expired in the Admin console. Check `chrome://policy`. |
