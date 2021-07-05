#include "stdafx.cpp"

#define PrologBufferSize 256

class CModuleInfo
{
public:
    CModuleInfo()
    {
        m_moduleID = NULL;
        m_assemblyID = NULL;
        m_pBaseLoadAddress = NULL;
        m_cchModuleName = MAX_PATH;
        m_tkPInvokeMethodDef = 0;
    }
private:

public:

    ModuleID m_moduleID;
    AssemblyID m_assemblyID;

    LPCBYTE m_pBaseLoadAddress;
    PROFILER_WCHAR m_wszModuleName[MAX_PATH];
    ULONG m_cchModuleName;
    mdMethodDef m_tkPInvokeMethodDef;
};