#ifndef __PROFILER_STRUCTS_H__
#define __PROFILER_STRUCTS_H__

#include "cor.h"
#include "corhdr.h"      // Needs to be included before corprof
#include "corprof.h"     // Definations for ICorProfilerCallback and friends
#include "corsym.h"     // Definations for ISymUnmanagedReader

#include <windows.h>
#include <map>        // STL Map and MultiMap

#include <string>
#include <sstream>
#include <iostream>

using namespace std;

class PrfClassInfo
{
    public:

        PrfClassInfo( ClassID classId);

        ClassID     m_classID;
        ClassID     m_parentClassID;
        ModuleID    m_moduleID;
        mdTypeDef   m_classToken;
        BOOL        m_isPremature;
        ULONG32     m_nTypeArgs;
        ULONG       m_objectsAllocated;
        ClassID     m_typeArgs[5]; //?? Magic Number
        wstring      m_className;
};


class PrfFunctionInfo
{
    public:

        LONG    m_enter;
        LONG    m_left;
        LONG    m_tail;
        LONG    m_unwind;
        BOOL    m_finishedJIT;
        BOOL    m_pitched;
        BOOL    m_cached;

        FunctionID  m_functionID;
        ClassID     m_classID;
        ModuleID    m_moduleID;
        ULONG       m_codeSize;
        mdToken     m_functionToken;
        LPCBYTE     m_pStartAddress;
        wstring     m_functionName;
        TCHAR       m_functionSig[4*1024];    //?? Magic Number    

        ULONG       m_oldILCodeSize;
        ULONG       m_newILCodeSize;
        LPCBYTE     m_pOldHeader;
        LPCBYTE     m_pNewHeader;
};


class PrfGCVariables
{
    public:
        
        PrfGCVariables();
        virtual VOID reset();

        ThreadID m_threadID;
        BOOL m_resumeStarted;
        BOOL m_suspendCompleted;
        BOOL m_referencesUpdated;
        BOOL m_shutdownMode;
        BOOL m_GCInShutdownOccurred;

        DWORD m_movedReferences;
        DWORD m_objectReferences;
        DWORD m_objectsAllocatedByClass;
        DWORD m_rootReferences;
        DWORD m_objectsAllocated;
};


class PrfModuleInfo
{
    public:
        ~PrfModuleInfo();

        ModuleID        m_moduleID;
        mdModule    m_moduleToken;
        AssemblyID    m_assemblyID;
        IMetaDataImport *m_pMDImport;
        LPCBYTE        m_pBaseLoadAddress;
        TCHAR        m_moduleName[1024];    //?? Magic Number
        ISymUnmanagedReader *m_pReader;
        BOOL        m_bIsPrejitted;
        DWORD        m_dwFunctionsUsed;

        map<FunctionID, PrfFunctionInfo *> m_functionMap;
        
};


class PrfObjectInfo
{
    public:

        PrfObjectInfo( ObjectID objectID, ClassID classID );
        ~PrfObjectInfo();

        ObjectID    m_newID;
        ObjectID    m_objectID;
        ClassID    m_classID;
        SIZE_T    m_size;
        BOOL    m_isRoot;
        BOOL    m_isVisited;
        BOOL    m_isSurvived;
        BOOL    m_isQueuedForFinalize;
        PrfObjectInfo *m_pParent;

        typedef map<UINT_PTR, UINT_PTR> ReferencesMap;
        ReferencesMap m_referencesMap;
        ReferencesMap::iterator m_irm;
};


class PrfGCHandleInfo
{
    public:

        PrfGCHandleInfo( GCHandleID handleID, ObjectID objectId );

        GCHandleID    m_handleID;
        ObjectID    m_objectID;
};


class PrfSuspensionInfo
{
    public:

        PrfSuspensionInfo( ThreadID threadID, COR_PRF_SUSPEND_REASON suspendReason);

        BOOL m_aborted;
        COR_PRF_SUSPEND_REASON m_reason;
        ThreadID m_threadID;
};


#endif //__PROFILER_STRUCTS_H__
    
