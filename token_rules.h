/*
 * token_rules.h -- normalization and validity rules for the CBCM enrollment key.
 *
 * This is the ONE place that defines what counts as a "valid enrollment key".
 * It is pure (no Windows APIs), so it is shared by the DLL and by the unit test.
 * To accept a different token format, change IsValidEnrollmentToken() here;
 * nothing else needs to change.
 */
#ifndef TOKEN_RULES_H
#define TOKEN_RULES_H

#include <string>

// Trim surrounding whitespace and double quotes from a wide string.
inline std::wstring TrimToken(const wchar_t* s)
{
    std::wstring t(s ? s : L"");
    size_t b = t.find_first_not_of(L" \t\r\n\"");
    size_t e = t.find_last_not_of(L" \t\r\n\"");
    if (b == std::wstring::npos) return L"";
    return t.substr(b, e - b + 1);
}

// Normalize an incoming token: trim whitespace/quotes, then strip one pair of
// surrounding braces if present ("{uuid}" -> "uuid"). Returns the bare token.
inline std::wstring NormalizeToken(const wchar_t* s)
{
    std::wstring t = TrimToken(s);
    if (t.size() >= 2 && t.front() == L'{' && t.back() == L'}')
        t = t.substr(1, t.size() - 2);
    return t;
}

// Is this a valid CBCM enrollment key?
//
// CBCM enrollment tokens are UUIDs in 8-4-4-4-12 form, e.g.
//     a1b2c3d4-5678-90ab-cdef-1234567890ab
// We validate that exact shape (hex digits, hyphens in the right places,
// case-insensitive). This rejects empty, truncated, or garbled keys before we
// prompt for elevation or touch the registry.
//
// >>> If your environment ever issues tokens in a different format, relax or
//     replace the body of THIS function; nothing else needs to change. <<<
inline bool IsValidEnrollmentToken(const std::wstring& s)
{
    if (s.size() != 36)
        return false;
    for (size_t i = 0; i < 36; ++i)
    {
        const wchar_t c = s[i];
        if (i == 8 || i == 13 || i == 18 || i == 23)
        {
            if (c != L'-') return false;
        }
        else
        {
            const bool hex = (c >= L'0' && c <= L'9') ||
                             (c >= L'a' && c <= L'f') ||
                             (c >= L'A' && c <= L'F');
            if (!hex) return false;
        }
    }
    return true;
}

#endif /* TOKEN_RULES_H */
