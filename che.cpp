/*
 * che.cpp -- implementation.
 *
 * For the non-admin case the module re-launches itself through rundll32.exe
 * with the ShellExecute "runas" verb (which raises the system consent dialog).
 * The elevated instance calls the private rundll32 entry point ApplyW, which
 * performs the write. Success is confirmed by reading the value back (a read
 * does not require elevation), so we never depend on rundll32's exit code.
 */
#define WIN32_LEAN_AND_MEAN
#define CHE_EXPORTS
#include "che.h"

#include <windows.h>
#include <shellapi.h>
#include <string>

#include "keyrules.h"   // NormalizeKey / IsValidKey (shared, testable)

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

static const wchar_t* kPath     = L"SOFTWARE\\Policies\\Google\\Chrome";
static const wchar_t* kValName  = L"CloudManagementEnrollmentToken";
static const size_t   kMaxLen   = 4096;

/* ---- helpers ----------------------------------------------------------- */

// TRUE if the current process token is elevated (full admin rights).
static BOOL IsElevated()
{
    BOOL   elevated = FALSE;
    HANDLE hToken   = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION te;
        DWORD cb = sizeof(te);
        if (GetTokenInformation(hToken, TokenElevation, &te, sizeof(te), &cb))
            elevated = te.TokenIsElevated;
        CloseHandle(hToken);
    }
    return elevated;
}

// Writes the value. Requires elevation.
static BOOL WriteValue(const wchar_t* value)
{
    HKEY hKey = NULL;
    LONG rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, kPath, 0, NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_SET_VALUE | KEY_WOW64_64KEY,
                              NULL, &hKey, NULL);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError((DWORD)rc);
        return FALSE;
    }

    const DWORD cb = (DWORD)((wcslen(value) + 1) * sizeof(wchar_t));
    rc = RegSetValueExW(hKey, kValName, 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(value), cb);
    RegCloseKey(hKey);

    if (rc != ERROR_SUCCESS)
    {
        SetLastError((DWORD)rc);
        return FALSE;
    }
    return TRUE;
}

// Reads the value back and compares; confirms the write regardless of who did it.
static BOOL VerifyValue(const wchar_t* expected)
{
    HKEY hKey = NULL;
    LONG rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, kPath, 0,
                            KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey);
    if (rc != ERROR_SUCCESS)
        return FALSE;

    wchar_t buf[kMaxLen + 1];
    DWORD   cb   = sizeof(buf) - sizeof(wchar_t); // leave room to NUL-terminate
    DWORD   type = 0;
    rc = RegQueryValueExW(hKey, kValName, NULL, &type,
                          reinterpret_cast<BYTE*>(buf), &cb);
    RegCloseKey(hKey);

    if (rc != ERROR_SUCCESS || type != REG_SZ)
        return FALSE;

    buf[cb / sizeof(wchar_t)] = L'\0'; // guarantee termination
    return wcscmp(buf, expected) == 0;
}

// Full path to this module on disk.
static std::wstring ThisModulePath()
{
    HMODULE hMod = NULL;
    GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&ThisModulePath), &hMod);

    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(hMod, path, MAX_PATH);
    return std::wstring(path);
}

// Re-launch elevated via rundll32 + "runas"; raises the consent prompt.
static BOOL RunElevated(const wchar_t* value)
{
    std::wstring dll = ThisModulePath();
    if (dll.empty())
        return FALSE;

    // rundll32 syntax: rundll32.exe "<dll>",ApplyW <value>
    std::wstring params = L"\"" + dll + L"\",ApplyW " + value;

    wchar_t sysdir[MAX_PATH] = {0};
    GetSystemDirectoryW(sysdir, MAX_PATH);
    std::wstring rundll = std::wstring(sysdir) + L"\\rundll32.exe";

    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    sei.lpVerb       = L"runas";              // triggers consent dialog
    sei.lpFile       = rundll.c_str();
    sei.lpParameters = params.c_str();
    sei.nShow        = SW_HIDE;

    if (!ShellExecuteExW(&sei))
        return FALSE;                          // GetLastError()==ERROR_CANCELLED on decline

    if (sei.hProcess)
    {
        WaitForSingleObject(sei.hProcess, INFINITE);
        CloseHandle(sei.hProcess);
    }

    return VerifyValue(value);
}

// Shared core (wide). Valid key AND admin AND verified write -> TRUE; else FALSE.
static BOOL Core(const wchar_t* in)
{
    if (!in)
        return FALSE;

    const std::wstring key = NormalizeKey(in);
    if (!IsValidKey(key))
        return FALSE;                          // reject before any prompt/write

    if (IsElevated())
        return WriteValue(key.c_str());

    return RunElevated(key.c_str());
}

/* ---- exports ----------------------------------------------------------- */

extern "C" CHE_API BOOL __stdcall Activate(const char* key)
{
    if (!key || key[0] == '\0')
        return FALSE;

    wchar_t wbuf[kMaxLen + 1];
    int n = MultiByteToWideChar(CP_UTF8, 0, key, -1, wbuf, (int)(kMaxLen + 1));
    if (n <= 0)
        return FALSE;
    return Core(wbuf);
}

// Private rundll32 entry point (elevated instance). Trailing 'W' => Unicode
// command line. Not part of the public interface.
extern "C" CHE_API void CALLBACK ApplyW(
    HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow)
{
    (void)hwnd; (void)hinst; (void)nCmdShow;

    std::wstring key = NormalizeKey(lpszCmdLine);
    if (!IsValidKey(key))
        ExitProcess(2);

    BOOL ok = WriteValue(key.c_str());
    ExitProcess(ok ? 0u : 1u);
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    (void)reason;
    return TRUE;
}
