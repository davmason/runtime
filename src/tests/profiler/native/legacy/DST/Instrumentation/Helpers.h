// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

class IUnknownHolder
{
public:
    IUnknownHolder (IUnknown *pUnknown)
    {
        m_pUnknown = pUnknown;
    }

    ~IUnknownHolder ()
    {
        m_pUnknown->Release();
    }
private:
    IUnknown *m_pUnknown;
};
