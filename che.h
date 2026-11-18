/*
 * che.h -- public interface.
 */
#ifndef CHE_H
#define CHE_H

#include <windows.h>

#ifdef CHE_EXPORTS
#define CHE_API __declspec(dllexport)
#else
#define CHE_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Activate
 * --------
 * key : null-terminated UTF-8 (ASCII) string. Must be non-empty and well-formed.
 *
 * Checks for admin rights; if absent, shows the system consent prompt and, when
 * accepted, completes the operation from an elevated context. The result is
 * verified before returning.
 *
 * Returns TRUE  only if the key is valid AND admin rights were obtained AND the
 *               operation was verified.
 *         FALSE on an invalid key, a declined prompt, or any failure.
 *
 * __stdcall (WINAPI); the export name is undecorated.
 */
CHE_API BOOL __stdcall Activate(const char* key);

#ifdef __cplusplus
}
#endif

#endif /* CHE_H */
