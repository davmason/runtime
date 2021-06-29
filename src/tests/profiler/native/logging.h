// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#pragma once

#include <assert.h>
#include <cwctype>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <type_traits>

#include "profilerstring.h"

class Logger
{
    class LoggerException
    {
    private:
        String _message;

    public:
        LoggerException(String message) :
            _message(message)
        {

        }

        String& what()
        {
            return _message;
        }
    };

private:
    std::mutex _lock;

    bool Equals(StringView lhs, StringView rhs)
    {
        if (lhs.Size() != rhs.Size())
        {
            return false;
        }

        for (size_t i = 0; i < lhs.Size(); ++i)
        {
            if (std::towlower(lhs[i]) != std::towlower(rhs[i]))
            {
                return false;
            }
        }

        return true;
    }

    StringView GetFormatSpecifier(StringView fmtSpecifier)
    {
        if (fmtSpecifier[0] != '{')
        {
            throw LoggerException(String(U("illegal start to format specifier ")) += fmtSpecifier);
        }
        else if (fmtSpecifier.Size() <= 2)
        {
            throw LoggerException(String(U("empty fmt specifier")));
        }

        for (size_t i = 1; i < fmtSpecifier.Size(); ++i)
        {
            if (fmtSpecifier[i] == L'}')
            {
                StringView specifier(&fmtSpecifier[1], i - 1);
                return specifier;
            }
        }

        throw LoggerException(String(U("unterminated format specifier")));
    }

    StringView WriteUntilNextFormat(StringView str)
    {
        bool maybeFormat = false;
        for (size_t i = 0; i < str.Size(); ++i)
        {
            if (maybeFormat)
            {
                if (str[i] != L'{')
                {
                    StringView tailWithFmt(&str[i - 1]);
                    if (tailWithFmt.Size() < 2)
                    {
                        throw LoggerException(String(U("invalid or missing format string")));
                    }

                    return tailWithFmt;
                }

                maybeFormat = false;
            }
            else if (str[i] == L'{')
            {
                maybeFormat = true;
                continue;
            }

            std::wcout << str[i];
        }

        return StringView();
    }

    template<class ...Args>
    void WriteLineHelper(StringView fmtString)
    {
        StringView fmtSpecifier = WriteUntilNextFormat(fmtString);
        if (!fmtSpecifier.Empty())
        {
            throw LoggerException(String(U("The following format specifier has no argument: ")) += fmtSpecifier);
        }
    }

    template<class ...Args>
    void WriteLineHelper(StringView fmtString, String arg1, Args... args)
    {
        return WriteLineHelper(fmtString, StringView(arg1), args...);
    }

    template<class ...Args>
    void WriteLineHelper(StringView fmtString, const WCHAR *arg1, Args... args)
    {
        return WriteLineHelper(fmtString, StringView(arg1), args...);
    }

    template<class ...Args>
    void WriteLineHelper(StringView fmtString, StringView arg1, Args... args)
    {
        StringView format = WriteUntilNextFormat(fmtString);
        StringView fmtSpecifier = GetFormatSpecifier(format);

        if (Equals(fmtSpecifier, U("s"))
            || Equals(fmtSpecifier, U("str")))
        {
            std::wcout << arg1;
        }
        else
        {
            throw LoggerException(String(U("invalid format string ")) += fmtSpecifier);
        }

        size_t index = fmtSpecifier.Size() + 2;
        StringView tail(&format[index]);
        WriteLineHelper(tail, args...);
    }

    template<class T, class ...Args>
    void WriteLineHelper(StringView fmtString, T arg1, Args... args)
    {

        StringView format = WriteUntilNextFormat(fmtString);
        StringView fmtSpecifier = GetFormatSpecifier(format);

        if (Equals(fmtSpecifier, U("i"))
            || Equals(fmtSpecifier, U("int")))
        {
            if (!std::is_integral<T>::value)
            {
                throw LoggerException(String(U("Format specifier is invalid ")) += fmtSpecifier);
            }

            std::wcout << arg1;
        }
        else if (Equals(fmtSpecifier, U("x"))
                 || Equals(fmtSpecifier, U("hex"))
                 || Equals(fmtSpecifier, U("p"))
                 || Equals(fmtSpecifier, U("ptr"))
                 || Equals(fmtSpecifier, U("hr")))
        {
            if (!std::is_integral<T>::value)
            {
                throw LoggerException(String(U("Format specifier is invalid ")) += fmtSpecifier);
            }
            

            std::wcout << U("0x") << std::hex << arg1;
        }
        else if (Equals(fmtSpecifier, U("d"))
                 || Equals(fmtSpecifier, U("double"))
                 || Equals(fmtSpecifier, U("f"))
                 || Equals(fmtSpecifier, U("float")))
        {
            if (!std::is_floating_point<T>::value)
            {
                throw LoggerException(String(U("Format specifier is invalid ")) += fmtSpecifier);
            }

            std::wcout << arg1;
        }
        else
        {
            throw LoggerException(String(U("Format specifier is invalid ")) += fmtSpecifier);
        }

        size_t index = fmtSpecifier.Size() + 2;
        StringView tail;
        if (index >= format.Size())
        {
            tail = U("");
        }
        else
        {
            tail = &format[index];
        }

        WriteLineHelper(tail, args...);
    }
public:
    template<class ...Args>
    void Write(StringView fmtString, Args... args)
    {
        // So we can use cout without interleaving output
        std::lock_guard<std::mutex> guard(_lock);

        WriteLineHelper(fmtString, args...);
    }

    template<class ...Args>
    void WriteLine(StringView fmtString, Args... args)
    {
        // So we can use cout without interleaving output
        std::lock_guard<std::mutex> guard(_lock);

        WriteLineHelper(fmtString, args...);

        std::wcout << std::endl;
    }
};
