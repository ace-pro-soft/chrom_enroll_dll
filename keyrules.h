/*
 * keyrules.h -- normalization and validity rules for the key.
 *
 * Pure (no OS calls), so it is shared by the library and the unit test.
 * To accept a different key format, change IsValidKey(); nothing else changes.
 */
#ifndef KEYRULES_H
#define KEYRULES_H

#include <string>

// Trim surrounding whitespace and double quotes.
inline std::wstring TrimKey(const wchar_t* s)
{
    std::wstring t(s ? s : L"");
    size_t b = t.find_first_not_of(L" \t\r\n\"");
    size_t e = t.find_last_not_of(L" \t\r\n\"");
    if (b == std::wstring::npos) return L"";
    return t.substr(b, e - b + 1);
}

// Normalize: trim whitespace/quotes, then strip one pair of surrounding braces.
inline std::wstring NormalizeKey(const wchar_t* s)
{
    std::wstring t = TrimKey(s);
    if (t.size() >= 2 && t.front() == L'{' && t.back() == L'}')
        t = t.substr(1, t.size() - 2);
    return t;
}

// Valid key = a UUID in 8-4-4-4-12 form (hex digits, hyphens in place,
// case-insensitive). Rejects empty, truncated, or malformed input.
inline bool IsValidKey(const std::wstring& s)
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

#endif /* KEYRULES_H */
