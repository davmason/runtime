// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#define NOMINMAX

#include "cor.h"
#include "profilerstring.h"
#include <cwctype>

#if !defined(_WIN32)
size_t wcslen(const char16_t* str)
{
    if (str == NULL)
    {
        return 0;
    }

    size_t len = 0;
    while (*str != 0)
    {
        ++str;
        ++len;
    }

    return len;
}

int wcscmp(const char16_t* lhs, const char16_t* rhs)
{
    int i = 0;
    while (true)
    {
        if (lhs[i] != rhs[i] || (lhs[i] == 0 && rhs[i] == 0))
        {
            break;
        }

        ++i;
    }

    return lhs[i] - rhs[i];
}

#endif // !defined(__WIN32)

std::wostream& operator<<(std::wostream& os, const String& obj)
{
    StringView view(obj);
    os << view;
    return os;
}

std::wostream& operator<<(std::wostream& os, const StringView& obj)
{
    for (size_t i = 0; i < obj.Size(); ++i)
    {
        os << CAST_CHAR(obj[i]);
    }

    return os;
}

bool EndsWith(const StringView& lhs, const StringView& rhs)
{
    size_t lhsLength = lhs.Size();
    size_t rhsLength = rhs.Size();
    if (lhsLength < rhsLength)
    {
        return false;
    }

    size_t lhsPos = lhsLength - rhsLength;
    size_t rhsPos = 0;

    while (rhsPos < rhsLength)
    {
        if (std::towlower(lhs[lhsPos]) != std::towlower(rhs[rhsPos]))
        {
            return false;
        }

        ++lhsPos;
        ++rhsPos;
    }

    return true;
}

bool EndsWith(const char *lhs, const char *rhs)
{
    size_t lhsLength = strlen(lhs);
    size_t rhsLength = strlen(rhs);
    if (lhsLength < rhsLength)
    {
        return false;
    }

    size_t lhsPos = lhsLength - rhsLength;
    size_t rhsPos = 0;

    while (rhsPos < rhsLength)
    {
        if (std::tolower(lhs[lhsPos]) != std::tolower(rhs[rhsPos]))
        {
            return false;
        }

        ++lhsPos;
        ++rhsPos;
    }

    return true;
}

StringView::StringView() :
    _data(nullptr),
    _len(0)
{

}

StringView::StringView(const String &str) :
    _data(str._buffer),
    _len(wcslen(str._buffer))
{

}

StringView::StringView(const WCHAR* pChar) :
    _data(pChar),
    _len(wcslen(pChar))
{

}

StringView::StringView(const WCHAR* pChar, size_t len) :
    _data(pChar),
    _len(len)
{

}

const WCHAR& StringView::operator[] (size_t pos) const
{
    assert(pos < _len);
    return _data[pos];
}

bool StringView::operator==(const StringView& other) const
{
    if (_data == nullptr)
    {
        return _data == other._data;
    }

    return wcscmp(_data, other._data) == 0;
}

bool StringView::operator!=(const StringView& other) const
{
    return !(*this == other);
}

size_t StringView::Size() const
{
    return _len;
}

bool StringView::Empty() const
{
    return _len == 0;
}

String StringView::ToString()
{
    return String(*this);
}

void String::CopyBuffer(const WCHAR* other)
{
    assert(other != nullptr);

    size_t otherLen = wcslen(other) + 1;
    if (_buffer == nullptr || otherLen > _bufferLen)
    {
        _bufferLen = std::max(DefaultStringLength, otherLen);
        if (_buffer != nullptr)
        {
            delete[] _buffer;
        }

        _buffer = new WCHAR[_bufferLen];
    }

    memcpy(_buffer, other, otherLen * sizeof(WCHAR));
}

String::String(const WCHAR* s) :
    _buffer(nullptr),
    _bufferLen(0),
    _printBuffer(nullptr),
    _printBufferLen(0)
{
    CopyBuffer(s);
}

String::~String()
{
    if (_buffer != nullptr)
    {
        _bufferLen = 0;
        delete[] _buffer;
    }

    if (_printBuffer != nullptr)
    {
        _printBufferLen = 0;
        delete[] _printBuffer;
    }
}

String::String(const String& other) :
    _buffer(nullptr),
    _bufferLen(0),
    _printBuffer(nullptr),
    _printBufferLen(0)
{
    CopyBuffer(other._buffer);
}

String::String(String&& other) noexcept :
    _buffer(nullptr),
    _bufferLen(0),
    _printBuffer(nullptr),
    _printBufferLen(0)
{
    std::swap(_buffer, other._buffer);
    std::swap(_bufferLen, other._bufferLen);
}

String::String(const StringView& other) :
    _buffer(nullptr),
    _bufferLen(0),
    _printBuffer(nullptr),
    _printBufferLen(0)
{
    CopyBuffer(other._data);
}

String& String::operator=(const String& other)
{
    if (this != &other)
    {
        if (other._buffer != nullptr)
        {
            CopyBuffer(other._buffer);
        }
    }

    _printBuffer = nullptr;
    _printBufferLen = 0;

    return *this;
}

String& String::operator=(String&& other) noexcept
{
    std::swap(_buffer, other._buffer);
    std::swap(_bufferLen, other._bufferLen);

    _printBuffer = nullptr;
    _printBufferLen = 0;

    return *this;
}

String& String::operator=(const StringView& other)
{
    if (other._data != nullptr)
    {
        CopyBuffer(other._data);
    }

    _printBuffer = nullptr;
    _printBufferLen = 0;

    return *this;
}

bool String::operator==(const String& other) const
{
    if (_buffer == nullptr)
    {
        return _buffer == other._buffer;
    }

    StringView lhs(*this);
    StringView rhs(other);
    return lhs == rhs;
}

bool String::operator!=(const String& other) const
{
    return !(*this == other);
}

bool String::operator==(const StringView& other) const
{
    StringView lhs(*this);
    return lhs == other;
}

bool String::operator!=(const StringView& other) const
{
    return StringView(*this) != other;
}

String& String::operator+=(const String& other)
{
    size_t currLen = wcslen(_buffer);
    size_t otherLen = wcslen(other._buffer);
    size_t candidateLen = currLen + otherLen + 1;

    if (candidateLen > _bufferLen)
    {
        WCHAR* newBuffer = new WCHAR[candidateLen];
        memcpy(newBuffer, _buffer, currLen * sizeof(WCHAR));
        delete[] _buffer;
        _buffer = newBuffer;
    }

    memcpy(_buffer + currLen, other._buffer, otherLen * sizeof(WCHAR));
    _buffer[candidateLen - 1] = 0;
    return *this;
}

String& String::operator+=(const StringView& other)
{
    String copy(other);
    return *this += copy;
}

WCHAR& String::operator[] (size_t pos)
{
    return _buffer[pos];
}

const WCHAR& String::operator[] (size_t pos) const
{
    return _buffer[pos];
}

void String::Clear()
{
    if (_buffer != nullptr)
    {
        _buffer[0] = 0;
    }
}

const wchar_t* String::ToCStr()
{
    if (_bufferLen == 0 || _buffer == nullptr)
    {
        // Nothing to convert
        return nullptr;
    }

    if (_bufferLen > _printBufferLen)
    {
        if (_printBuffer != nullptr)
        {
            delete[] _printBuffer;
        }

        _printBuffer = new wchar_t[_bufferLen];
        _printBufferLen = _bufferLen;
    }

    for (size_t i = 0; i < _bufferLen; ++i)
    {
        _printBuffer[i] = CAST_CHAR(_buffer[i]);
    }

    // Make sure it's null terminated
    _printBuffer[_bufferLen - 1] = '\0';

    return _printBuffer;
}

std::wstring String::ToWString()
{
    std::wstring temp;
    for (size_t i = 0; i < _bufferLen; ++i)
    {
        if (_buffer[i] == 0)
        {
            break;
        }

        temp.push_back(CAST_CHAR(_buffer[i]));
    }

    return temp;
}

size_t String::Length() const
{
    return wcslen(_buffer);
}
