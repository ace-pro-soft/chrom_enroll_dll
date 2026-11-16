// Unit test for the enrollment-key rules (token_rules.h).
// Pure computation: no registry, no admin, safe to run anywhere.
// Exercises the SAME NormalizeToken + IsValidEnrollmentToken the DLL uses.
#include "token_rules.h"
#include <cstdio>

static int g_fail = 0;

// Mirrors EnrollCore's decision: normalize, then validate.
static bool Accepts(const wchar_t* raw) {
    return IsValidEnrollmentToken(NormalizeToken(raw));
}

static void expect(const wchar_t* raw, bool want) {
    bool got = Accepts(raw);
    const char* res = (got == want) ? "ok  " : "FAIL";
    if (got != want) ++g_fail;
    wprintf(L"[%hs] want=%d got=%d  \"%s\"\n", res, (int)want, (int)got, raw);
}

int main() {
    // --- valid UUID enrollment keys ---------------------------------------
    expect(L"a1b2c3d4-5678-90ab-cdef-1234567890ab", true);   // lowercase
    expect(L"A1B2C3D4-5678-90AB-CDEF-1234567890AB", true);   // uppercase
    expect(L"  a1b2c3d4-5678-90ab-cdef-1234567890ab  ", true);// surrounding spaces
    expect(L"\"a1b2c3d4-5678-90ab-cdef-1234567890ab\"", true);// surrounding quotes
    expect(L"{a1b2c3d4-5678-90ab-cdef-1234567890ab}", true);  // surrounding braces

    // --- invalid keys (must be rejected -> FALSE from EnrollChromeBrowser) -
    expect(L"", false);                                       // empty
    expect(L"   ", false);                                    // whitespace only
    expect(L"not-a-valid-token", false);                      // garbage
    expect(L"a1b2c3d4-5678-90ab-cdef-1234567890a", false);    // 35 chars (short)
    expect(L"a1b2c3d4-5678-90ab-cdef-1234567890abc", false);  // 37 chars (long)
    expect(L"g1b2c3d4-5678-90ab-cdef-1234567890ab", false);   // non-hex 'g'
    expect(L"a1b2c3d45678-90ab-cdef--234567890ab0", false);   // hyphens misplaced
    expect(L"a1b2c3d4-5678-90ab-cdef-12345 67890a", false);   // internal space

    if (g_fail == 0) { wprintf(L"\nALL TESTS PASSED\n"); return 0; }
    wprintf(L"\n%d TEST(S) FAILED\n", g_fail);
    return 1;
}
