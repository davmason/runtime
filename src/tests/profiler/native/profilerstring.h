// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#pragma once

#include <iostream>
#include <assert.h>
#include <cstring>
#include <string>
#include <algorithm>

#ifdef _WIN32
#define U(str) L##str
#define CAST_CHAR(ch) ch

#else // defined(_WIN32)

// Definitely won't work for non-ascii characters so hopefully we never start using
// them in the tests
#define CAST_CHAR(ch) static_cast<wchar_t>(ch)

// On linux the runtime uses 16 bit strings but the native platform wchar_t is 32 bit.
// This means there aren't c runtime functions like wcslen for 16 bit strings. The idea
// here is to provide the easy ones to avoid all the copying and transforming. If more complex
// string operations become necessary we should either write them in C++ or convert the string to
// 32 bit and call the c runtime ones.

#define U(str) u##str
size_t wcslen(const char16_t* str);
int wcscmp(const char16_t* lhs, const char16_t* rhs);
#endif // defined(__WIN32)

class String;
class StringView;

std::wostream& operator<<(std::wostream& os, const String& obj);
std::wostream& operator<<(std::wostream& os, const StringView& obj);
bool EndsWith(const StringView& lhs, const StringView& rhs);
bool EndsWith(const char *lhs, const char *rhs);

class StringView
{
    friend class String;
private:
    const WCHAR* _data;
    size_t _len;

public:
    StringView();
    StringView(const String &str);
    StringView(const WCHAR* pChar);
    StringView(const WCHAR* pChar, size_t len);
    ~StringView() = default;

    const WCHAR& operator[] (size_t pos) const;
    bool operator==(const StringView& other) const;
    bool operator!=(const StringView& other) const;

    size_t Size() const;
    bool Empty() const;

    String ToString();
};

// 16 bit string type that works cross plat and doesn't require changing widths
// on non-windows platforms
class String
{
    friend class StringView;
    friend std::wostream& operator<<(std::wostream& os, const String& obj);
private:
    WCHAR* _buffer;
    size_t _bufferLen;
    wchar_t* _printBuffer;
    size_t _printBufferLen;
    const size_t DefaultStringLength = 1024;

    void CopyBuffer(const WCHAR* other);

public:
    explicit String(const WCHAR* s = U(""));
    explicit String(const StringView& other);
    String(const String& other);
    String(String&& other) noexcept;
    ~String();

    String& operator=(const String& other);
    String& operator=(String&& other) noexcept;
    String& operator=(const StringView& other);
    bool operator==(const String& other) const;
    bool operator!=(const String& other) const;
    bool operator==(const StringView& other) const;
    bool operator!=(const StringView& other) const;
    String& operator+=(const String& other);
    String& operator+=(const StringView& other);
    WCHAR& operator[] (size_t pos);
    const WCHAR& operator[] (size_t pos) const;
    void Clear();
    const wchar_t* ToCStr();
    std::wstring ToWString();
    size_t Length() const;
};
