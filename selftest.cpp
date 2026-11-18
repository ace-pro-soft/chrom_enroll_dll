// Minimal harness: load libche.dll dynamically and call the export.
#include <windows.h>
#include <stdio.h>

typedef BOOL (__stdcall *Fn)(const char*);

int main(int argc, char** argv)
{
    const char* key = (argc > 1) ? argv[1] : "00000000-0000-0000-0000-000000000000";

    HMODULE h = LoadLibraryW(L"libche.dll");
    if (!h) { printf("LoadLibrary failed: %lu\n", GetLastError()); return 10; }

    Fn fn = (Fn)GetProcAddress(h, "Activate");
    if (!fn) { printf("GetProcAddress failed: %lu\n", GetLastError()); return 11; }

    BOOL ok = fn(key);
    printf("Activate(\"%s\") returned %s\n", key, ok ? "TRUE" : "FALSE");

    FreeLibrary(h);
    return ok ? 0 : 1;
}
