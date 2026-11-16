/*
 * chrome_enroll.h  --  Chrome Browser Cloud Management (CBCM) enrollment DLL
 *
 * Public C API. Link against chrome_enroll.lib or load dynamically with
 * LoadLibrary / QLibrary and resolve "EnrollChromeBrowser" by name.
 *
 * The DLL writes the CBCM enrollment token to the machine-wide Chrome policy:
 *     HKLM\SOFTWARE\Policies\Google\Chrome!CloudManagementEnrollmentToken
 * which requires administrator rights. If the calling process is not elevated,
 * the DLL triggers a UAC consent prompt and performs the write from an elevated
 * helper instance of itself.
 */
#ifndef CHROME_ENROLL_H
#define CHROME_ENROLL_H

#include <windows.h>

#ifdef CHROMEENROLL_EXPORTS
#define CHROMEENROLL_API __declspec(dllexport)
#else
#define CHROMEENROLL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * EnrollChromeBrowser
 * -------------------
 * enrollmentToken : null-terminated UTF-16 (wide) string, the CBCM enrollment
 *                   token from the Google Admin console. Must be non-empty.
 *
 * Behaviour:
 *   1. Checks whether the current process is running elevated (admin).
 *   2. If elevated, writes the enrollment token to the Chrome policy key.
 *   3. If NOT elevated, shows a UAC consent prompt. If the user accepts, an
 *      elevated helper writes the token; if the user cancels, nothing happens.
 *   4. The result is verified by reading the value back from the registry.
 *
 * Returns TRUE  (1) if the token is present in the registry on return.
 *         FALSE (0) on bad input, UAC cancellation, or any failure.
 *
 * Calling convention is __stdcall (WINAPI). Thread-safe; performs no global
 * state changes other than the registry write.
 *
 * Three equivalent exports are provided so any language can use whichever
 * string type is natural:
 *   EnrollChromeBrowser   - wide (UTF-16) token; the primary entry point.
 *   EnrollChromeBrowserW  - identical alias (for FFIs that auto-append 'W').
 *   EnrollChromeBrowserA  - narrow token, interpreted as UTF-8 (ASCII is a
 *                           subset, so plain ASCII tokens work unchanged).
 */
CHROMEENROLL_API BOOL __stdcall EnrollChromeBrowser(const wchar_t* enrollmentToken);
CHROMEENROLL_API BOOL __stdcall EnrollChromeBrowserW(const wchar_t* enrollmentToken);
CHROMEENROLL_API BOOL __stdcall EnrollChromeBrowserA(const char* enrollmentTokenUtf8);

#ifdef __cplusplus
}
#endif

#endif /* CHROME_ENROLL_H */
