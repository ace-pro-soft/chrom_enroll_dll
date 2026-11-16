/*
 * chrome_enroll.cpp -- CBCM enrollment DLL implementation.
 *
 * Writes:  HKLM\SOFTWARE\Policies\Google\Chrome!CloudManagementEnrollmentToken
 *          (REG_SZ)
 *
 * Elevation strategy for the non-admin case:
 *   The DLL re-launches itself through rundll32.exe with the ShellExecute
 *   "runas" verb (which raises the UAC consent dialog). The elevated instance
 *   calls the exported rundll32 entry point EnrollElevatedW, which performs the
 *   registry write. We then verify success by reading the value back (a read of
 *   HKLM policies does not require elevation), so we never rely on rundll32
 *   propagating an exit code.
 */
#define WIN32_LEAN_AND_MEAN
#define CHROMEENROLL_EXPORTS
#include "chrome_enroll.h"

#include <windows.h>
#include <shellapi.h>
#include <string>

#include "token_rules.h"   // NormalizeToken / IsValidEnrollmentToken (shared, testable)

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

static const wchar_t* kPolicyKey   = L"SOFTWARE\\Policies\\Google\\Chrome";
static const wchar_t* kValueName   = L"CloudManagementEnrollmentToken";
static const size_t   kMaxTokenLen = 4096; // generous upper bound

/* ---- helpers ----------------------------------------------------------- */

// Returns TRUE if the current process token is elevated (full admin rights).
static BOOL IsProcessElevated()
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

// Writes the enrollment token to the Chrome policy key. Requires elevation.
static BOOL WriteEnrollmentToken(const wchar_t* token)
{
    HKEY hKey = NULL;
    LONG rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, kPolicyKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_SET_VALUE | KEY_WOW64_64KEY,
                              NULL, &hKey, NULL);
    if (rc != ERROR_SUCCESS)
    {
        SetLastError((DWORD)rc);
        return FALSE;
    }

    const DWORD cb = (DWORD)((wcslen(token) + 1) * sizeof(wchar_t));
    rc = RegSetValueExW(hKey, kValueName, 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(token), cb);
    RegCloseKey(hKey);

    if (rc != ERROR_SUCCESS)
    {
        SetLastError((DWORD)rc);
        return FALSE;
    }
    return TRUE;
}

// Reads the token back and compares; confirms the write regardless of who did it.
static BOOL VerifyEnrollmentToken(const wchar_t* expected)
{
    HKEY hKey = NULL;
    LONG rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE, kPolicyKey, 0,
                            KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKey);
    if (rc != ERROR_SUCCESS)
        return FALSE;

    wchar_t buf[kMaxTokenLen + 1];
    DWORD   cb   = sizeof(buf) - sizeof(wchar_t); // leave room to NUL-terminate
    DWORD   type = 0;
    rc = RegQueryValueExW(hKey, kValueName, NULL, &type,
                          reinterpret_cast<BYTE*>(buf), &cb);
    RegCloseKey(hKey);

    if (rc != ERROR_SUCCESS || type != REG_SZ)
        return FALSE;

    buf[cb / sizeof(wchar_t)] = L'\0'; // guarantee termination
    return wcscmp(buf, expected) == 0;
}

// Full path to this DLL on disk.
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

// Re-launch this DLL elevated via rundll32 + "runas"; raises the UAC prompt.
static BOOL RunElevatedWrite(const wchar_t* token)
{
    std::wstring dll = ThisModulePath();
    if (dll.empty())
        return FALSE;

    // rundll32 syntax: rundll32.exe "<dll>",EnrollElevatedW <token>
    std::wstring params = L"\"" + dll + L"\",EnrollElevatedW " + token;

    wchar_t sysdir[MAX_PATH] = {0};
    GetSystemDirectoryW(sysdir, MAX_PATH);
    std::wstring rundll = std::wstring(sysdir) + L"\\rundll32.exe";

    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize       = sizeof(sei);
    sei.fMask        = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    sei.lpVerb       = L"runas";              // triggers UAC consent dialog
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

    // Confirm the elevated helper actually wrote the token.
    return VerifyEnrollmentToken(token);
}

/* ---- exported API ------------------------------------------------------ */

// Shared core. Returns TRUE only if the key is valid AND admin rights are
// obtained (already elevated, or UAC accepted) AND the write is verified.
// Returns FALSE on an invalid key, a declined UAC prompt, or a failed write.
static BOOL EnrollCore(const wchar_t* enrollmentToken)
{
    if (!enrollmentToken)
        return FALSE;

    // 1) The enrollment key must be valid. Reject before prompting for UAC.
    const std::wstring token = NormalizeToken(enrollmentToken);
    if (!IsValidEnrollmentToken(token))
        return FALSE;

    // 2) Obtain admin rights and write. If already elevated, write directly;
    //    otherwise raise the UAC prompt. A declined prompt -> FALSE.
    if (IsProcessElevated())
        return WriteEnrollmentToken(token.c_str());

    return RunElevatedWrite(token.c_str());
}

// Primary (wide / UTF-16) entry point.
extern "C" CHROMEENROLL_API BOOL __stdcall EnrollChromeBrowser(const wchar_t* enrollmentToken)
{
    return EnrollCore(enrollmentToken);
}

// Win32-convention alias of the wide entry point (for FFIs that auto-append 'W').
extern "C" CHROMEENROLL_API BOOL __stdcall EnrollChromeBrowserW(const wchar_t* enrollmentToken)
{
    return EnrollCore(enrollmentToken);
}

// Narrow (UTF-8 / ASCII) entry point for callers that don't use wide strings.
extern "C" CHROMEENROLL_API BOOL __stdcall EnrollChromeBrowserA(const char* enrollmentTokenUtf8)
{
    if (!enrollmentTokenUtf8 || enrollmentTokenUtf8[0] == '\0')
        return FALSE;

    wchar_t wbuf[kMaxTokenLen + 1];
    int n = MultiByteToWideChar(CP_UTF8, 0, enrollmentTokenUtf8, -1,
                                wbuf, (int)(kMaxTokenLen + 1));
    if (n <= 0)
        return FALSE;                 // invalid/oversized input
    return EnrollCore(wbuf);
}

/*
 * EnrollElevatedW -- rundll32 entry point used by the elevated helper instance.
 * Signature required by rundll32; the trailing 'W' makes rundll32 pass a
 * Unicode command line (the token). Not intended to be called by host apps.
 */
extern "C" CHROMEENROLL_API void CALLBACK EnrollElevatedW(
    HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow)
{
    (void)hwnd; (void)hinst; (void)nCmdShow;

    std::wstring token = NormalizeToken(lpszCmdLine);
    if (!IsValidEnrollmentToken(token))
        ExitProcess(2);

    BOOL ok = WriteEnrollmentToken(token.c_str());
    ExitProcess(ok ? 0u : 1u);
}

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    (void)reason;
    return TRUE;
}
