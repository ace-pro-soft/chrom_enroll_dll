// Minimal harness: load chrome_enroll.dll dynamically and call the export,
// mirroring how a Qt app would resolve it at runtime.
#include <windows.h>
#include <stdio.h>

typedef BOOL (__stdcall *EnrollFn)(const wchar_t*);

int wmain(int argc, wchar_t** argv)
{
    const wchar_t* token = (argc > 1) ? argv[1] : L"TEST-TOKEN-chrom-enroll-DELETE";

    HMODULE h = LoadLibraryW(L"libche.dll");
    if (!h) { wprintf(L"LoadLibrary failed: %lu\n", GetLastError()); return 10; }

    EnrollFn fn = (EnrollFn)GetProcAddress(h, "EnrollChromeBrowser");
    if (!fn) { wprintf(L"GetProcAddress failed: %lu\n", GetLastError()); return 11; }

    BOOL ok = fn(token);
    wprintf(L"EnrollChromeBrowser(\"%s\") returned %s\n", token, ok ? L"TRUE" : L"FALSE");

    FreeLibrary(h);
    return ok ? 0 : 1;
}
