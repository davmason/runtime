// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#pragma once

#include <assert.h>
#include <cwctype>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <stdexcept>

class Logger
{
    class LoggerException
    {
    private:
        std::wstring _message;

    public:
        LoggerException(std::wstring message) :
            _message(message)
        {

        }

        std::wstring &what()
        {
            return _message;
        }
    };

private:
    std::mutex _lock;

    bool Equals(std::wstring_view lhs, std::wstring_view rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }

        for (int i = 0; i < lhs.size(); ++i)
        {
            if (std::towlower(lhs[i]) != std::towlower(rhs[i]))
            {
                return false;
            }
        }

        return true;
    }

    std::wstring_view GetFormatSpecifier(std::wstring_view fmtSpecifier)
    {
        if (fmtSpecifier[0] != '{')
        {
            throw LoggerException(std::wstring(L"illegal start to format specifier ") += fmtSpecifier);
        }

        for (auto it = fmtSpecifier.begin(); it != fmtSpecifier.end(); ++it)
        {
            if (*it == L'}')
            {
                std::wstring_view specifier(fmtSpecifier.begin() + 1, it);
                return specifier;
            }
        }

        throw LoggerException(L"unterminated format specifier");
    }

    std::wstring_view WriteUntilNextFormat(std::wstring_view str)
    {
        bool maybeFormat = false;
        for (auto it = str.begin(); it != str.end(); ++it)
        {
            if (maybeFormat)
            {
                if (*it != L'{')
                {
                    std::wstring_view tailWithFmt(--it, str.end());
                    if (tailWithFmt.size() < 2)
                    {
                        throw LoggerException(L"invalid or missing format string");
                    }

                    return tailWithFmt;
                }

                maybeFormat = false;
            }
            else if (*it == L'{')
            {
                maybeFormat = true;
                continue;
            }

            std::wcout << *it;
        }
    }

    template<class ...Args>
    void WriteLineHelper(std::wstring_view fmtString, std::wstring arg1, Args... args)
    {
        std::wstring_view format = WriteUntilNextFormat(fmtString);

        // Any malformed format specifiered should have been handled by WriteUntilNextFormat
        assert(format.size() > 2);

        std::wstring_view fmtSpecifier = GetFormatSpecifier(format);
        assert(fmtSpecifier.size() > 0);

        if (Equals(fmtSpecifier, L"s")
            || Equals(fmtSpecifier, L"str"))
        {
            std::wcout << arg1;
            std::wstring_view tail(format.begin() + fmtSpecifier.size(), format.end());
            WriteLineHelper(tail, args...);
            return;
        }

        throw LoggerException(std::wstring(L"invalid format string ") += fmtSpecifier);
    }

public:
    template<class ...Args>
    void WriteLine(std::wstring_view fmtString, Args... args)
    {
        // So we can use cout without interleaving output
        std::lock_guard guard(_lock);

        WriteLineHelper(fmtString, args...);

        std::wcout << std::endl;
    }
};
