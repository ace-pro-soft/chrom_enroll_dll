// Unit test for the key rules (keyrules.h). Pure computation: no registry,
// no admin, safe to run anywhere. Exercises the same NormalizeKey + IsValidKey
// the library uses.
#include "keyrules.h"
#include <cstdio>

static int g_fail = 0;

static bool Accepts(const wchar_t* raw) {
    return IsValidKey(NormalizeKey(raw));
}

static void expect(const wchar_t* raw, bool want) {
    bool got = Accepts(raw);
    const char* res = (got == want) ? "ok  " : "FAIL";
    if (got != want) ++g_fail;
    wprintf(L"[%hs] want=%d got=%d  \"%s\"\n", res, (int)want, (int)got, raw);
}

int main() {
    // valid
    expect(L"a1b2c3d4-5678-90ab-cdef-1234567890ab", true);
    expect(L"A1B2C3D4-5678-90AB-CDEF-1234567890AB", true);
    expect(L"  a1b2c3d4-5678-90ab-cdef-1234567890ab  ", true);
    expect(L"\"a1b2c3d4-5678-90ab-cdef-1234567890ab\"", true);
    expect(L"{a1b2c3d4-5678-90ab-cdef-1234567890ab}", true);
    // invalid
    expect(L"", false);
    expect(L"   ", false);
    expect(L"not-a-valid-key", false);
    expect(L"a1b2c3d4-5678-90ab-cdef-1234567890a", false);     // short
    expect(L"a1b2c3d4-5678-90ab-cdef-1234567890abc", false);   // long
    expect(L"g1b2c3d4-5678-90ab-cdef-1234567890ab", false);    // non-hex
    expect(L"a1b2c3d45678-90ab-cdef--234567890ab0", false);    // misplaced
    expect(L"a1b2c3d4-5678-90ab-cdef-12345 67890a", false);    // internal space

    if (g_fail == 0) { wprintf(L"\nALL TESTS PASSED\n"); return 0; }
    wprintf(L"\n%d TEST(S) FAILED\n", g_fail);
    return 1;
}
