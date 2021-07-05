// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// AllCallbacksWithDrilldown.cpp
//
// ======================================================================================

#include "ProfilerCommon.h"

typedef enum _MapFuncReturns {
    KeyWasNotInMap = 0,
    KeyWasInMap = 1
} MapFuncReturns;

class AllCallbacksWithDrilldown
{
    public:

        AllCallbacksWithDrilldown(IPrfCom * pPrfCom);
        ~AllCallbacksWithDrilldown();
		HRESULT Verify(IPrfCom * pPrfCom);

		/* Port of prf_g090.dll which implemented the following to varying degrees:
		HRESULT ProfilerCallback::AppDomainCreationStarted( AppDomainID appDomainID )
		HRESULT ProfilerCallback::AppDomainCreationFinished( AppDomainID appDomainID,
		HRESULT ProfilerCallback::AppDomainShutdownFinished( AppDomainID appDomainID,
		HRESULT ProfilerCallback::AssemblyLoadStarted( AssemblyID assemblyID )
		HRESULT ProfilerCallback::AssemblyLoadFinished( AssemblyID assemblyID,
		HRESULT ProfilerCallback::AssemblyUnloadStarted( AssemblyID assemblyID )
		HRESULT ProfilerCallback::AssemblyUnloadFinished( AssemblyID assemblyID,
		HRESULT ProfilerCallback::ModuleLoadStarted( ModuleID moduleID )
		HRESULT ProfilerCallback::ModuleLoadFinished( ModuleID moduleID,
		HRESULT ProfilerCallback::ModuleUnloadFinished( ModuleID moduleID,
		HRESULT ProfilerCallback::ModuleUnloadStarted( ModuleID moduleID )
		HRESULT ProfilerCallback::ModuleAttachedToAssembly( ModuleID moduleID,
		HRESULT ProfilerCallback::ClassLoadFinished( ClassID classID,
		HRESULT ProfilerCallback::ClassUnloadFinished( ClassID classID,
		HRESULT ProfilerCallback::JITCompilationStarted( FunctionID functionID,
		HRESULT ProfilerCallback::JITCompilationFinished( FunctionID functionID,
		HRESULT ProfilerCallback::JITFunctionPitched( FunctionID functionID )
		HRESULT ProfilerCallback::ThreadCreated( ThreadID threadID )
		HRESULT ProfilerCallback::ThreadDestroyed( ThreadID threadID )
		HRESULT ProfilerCallback::ThreadAssignedToOSThread( ThreadID managedThreadID,
		HRESULT ProfilerCallback::CreateObject( REFIID riid, void **ppInterface )
		*/

		/*
		Basic goal of the test:
			Register for a bunch of the callbacks and verify that the arguments passed to
            the callbacks can be used to make info calls to drill into the objects and/or
            verify that the current state is as expected (e.g. in many cases the test
            will verify that the object cannot be drilled into because it isn't valid yet
            such as in AppDomainCreationStarted).

        Basic outline of what the test does for each set of callbacks:
            ObjectLoad/CreateStarted events:
                Verify that requesting more information about the ObjectID either returns
                that additional info is not available or returns valid info.
            ObjectLoad/CreateFinished events:
                Verify that requesting more information about the ObjectID returns valid info.
            ObjectUnload/DesstroyStarted events:
                Verify that requesting more information about the ObjectID returns valid info.
            ObjectUnload/DesstroyFinished events:
                Don't do anything... We could verify that the requesting info about the
                ObjectID returns an HR indicating the object is gone but there is no assurance
                that the CLR will do that so drilling into an object once the CLR has told
                us it's gone will likely blow up.
		*/

#pragma region static_wrapper_methods
        static HRESULT AppDomainCreationFinishedWrapper(IPrfCom * pPrfCom,
                                                AppDomainID appDomainId,
                                                HRESULT hrStatus)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->AppDomainCreationFinished(pPrfCom, appDomainId, hrStatus);
        }

        static HRESULT AppDomainCreationStartedWrapper(IPrfCom * pPrfCom,
                                                AppDomainID appDomainId)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->AppDomainCreationStarted(pPrfCom, appDomainId);
        }

        static HRESULT AppDomainShutdownFinishedWrapper(IPrfCom * pPrfCom,
                                                AppDomainID appDomainId,
                                                HRESULT hrStatus)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->AppDomainShutdownFinished(pPrfCom, appDomainId, hrStatus);
        }

        static HRESULT AppDomainShutdownStartedWrapper(IPrfCom * pPrfCom,
                                                AppDomainID appDomainId)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->AppDomainShutdownStarted(pPrfCom, appDomainId);
        }

        static HRESULT AssemblyLoadFinishedWrapper(IPrfCom * pPrfCom,
                                                AssemblyID assemblyId,
                                                HRESULT hrStatus)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->AssemblyLoadFinished(pPrfCom, assemblyId, hrStatus);
        }

        static HRESULT AssemblyLoadStartedWrapper(IPrfCom * pPrfCom,
                                                AssemblyID assemblyId)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->AssemblyLoadStarted(pPrfCom, assemblyId);
        }

        static HRESULT AssemblyUnloadFinishedWrapper(IPrfCom * pPrfCom,
                                                AssemblyID assemblyId,
                                                HRESULT hrStatus)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->AssemblyUnloadFinished(pPrfCom, assemblyId, hrStatus);
        }

        static HRESULT AssemblyUnloadStartedWrapper(IPrfCom * pPrfCom,
                                                AssemblyID assemblyId)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->AssemblyUnloadStarted(pPrfCom, assemblyId);
        }

        static HRESULT ClassLoadFinishedWrapper(IPrfCom * pPrfCom,
                                                ClassID classId,
                                                HRESULT hrStatus)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->ClassLoadFinished(pPrfCom, classId, hrStatus);
        }

        static HRESULT ClassLoadStartedWrapper(IPrfCom * pPrfCom,
                                                ClassID classId)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->ClassLoadStarted(pPrfCom, classId);
        }

        static HRESULT ClassUnloadFinishedWrapper(IPrfCom * pPrfCom,
                                                ClassID classId,
                                                HRESULT hrStatus)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->ClassUnloadFinished(pPrfCom, classId, hrStatus);
        }

        static HRESULT ClassUnloadStartedWrapper(IPrfCom * pPrfCom,
                                                ClassID classId)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->ClassUnloadStarted(pPrfCom, classId);
        }

        static HRESULT JITCompilationFinishedWrapper(IPrfCom * pPrfCom,
                                                FunctionID functionId,
                                                HRESULT hrStatus,
                                                BOOL fIsSafeToBlock)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->JITCompilationFinished(pPrfCom, functionId, hrStatus, fIsSafeToBlock);
        }

        static HRESULT JITCompilationStartedWrapper(IPrfCom * pPrfCom,
                                                FunctionID functionId,
                                                BOOL fIsSafeToBlock)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->JITCompilationStarted(pPrfCom, functionId, fIsSafeToBlock);
        }

        static HRESULT ModuleAttachedToAssemblyWrapper(IPrfCom * pPrfCom,
                                                ModuleID moduleId,
                                                AssemblyID AssemblyId)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->ModuleAttachedToAssembly(pPrfCom, moduleId, AssemblyId);
        }

        static HRESULT ModuleLoadFinishedWrapper(IPrfCom * pPrfCom,
                                                ModuleID moduleId,
                                                HRESULT hrStatus)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->ModuleLoadFinished(pPrfCom, moduleId, hrStatus);
        }

        static HRESULT ModuleLoadStartedWrapper(IPrfCom * pPrfCom,
                                                ModuleID moduleId)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->ModuleLoadStarted(pPrfCom, moduleId);
        }

        static HRESULT ModuleUnloadFinishedWrapper(IPrfCom * pPrfCom,
                                                ModuleID moduleId,
                                                HRESULT hrStatus)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->ModuleUnloadFinished(pPrfCom, moduleId, hrStatus);
        }

        static HRESULT ModuleUnloadStartedWrapper(IPrfCom * pPrfCom,
                                                ModuleID moduleId)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->ModuleUnloadStarted(pPrfCom, moduleId);
        }

        static HRESULT ThreadAssignedToOSThreadWrapper(IPrfCom * pPrfCom,
                                                ThreadID managedThreadId,
                                                DWORD osThreadId)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->ThreadAssignedToOSThread(pPrfCom, managedThreadId, osThreadId);
        }

        static HRESULT ThreadCreatedWrapper(IPrfCom * pPrfCom,
                                                ThreadID managedThreadId)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->ThreadCreated(pPrfCom, managedThreadId);
        }

        static HRESULT ThreadDestroyedWrapper(IPrfCom * pPrfCom,
                                                ThreadID threadID)
        {
            return STATIC_CLASS_CALL(AllCallbacksWithDrilldown)->ThreadDestroyed(pPrfCom, threadID);
        }

#pragma endregion

    private:

#pragma region Callback_Handler_Prototypes

		HRESULT AllCallbacksWithDrilldown::AppDomainCreationFinished(
                                                IPrfCom * /* pPrfCom */,
												AppDomainID /* appDomainId */,
												HRESULT /* hrStatus */);

		HRESULT AllCallbacksWithDrilldown::AppDomainCreationStarted(
                                                IPrfCom * /* pPrfCom */,
												AppDomainID /* appDomainId */);

		HRESULT AllCallbacksWithDrilldown::AppDomainShutdownFinished(
                                                IPrfCom * /* pPrfCom */,
												AppDomainID /* appDomainId */,
												HRESULT /* hrStatus */);

		HRESULT AllCallbacksWithDrilldown::AppDomainShutdownStarted(
                                                IPrfCom * /* pPrfCom */,
												AppDomainID /* appDomainId */);

		HRESULT AllCallbacksWithDrilldown::AssemblyLoadFinished(
                                                IPrfCom * /* pPrfCom */,
												AssemblyID /* assemblyId */,
												HRESULT /* hrStatus */);

		HRESULT AllCallbacksWithDrilldown::AssemblyLoadStarted(
                                                IPrfCom * /* pPrfCom */,
												AssemblyID /* assemblyId */);

		HRESULT AllCallbacksWithDrilldown::AssemblyUnloadFinished(
                                                IPrfCom * /* pPrfCom */,
												AssemblyID /* assemblyId */,
												HRESULT /* hrStatus */);

		HRESULT AllCallbacksWithDrilldown::AssemblyUnloadStarted(
                                                IPrfCom * /* pPrfCom */,
												AssemblyID /* assemblyId */);

		HRESULT AllCallbacksWithDrilldown::ClassLoadFinished(
                                                IPrfCom * /* pPrfCom */,
												ClassID /* classId */,
												HRESULT /* hrStatus */);

		HRESULT AllCallbacksWithDrilldown::ClassLoadStarted(
                                                IPrfCom * /* pPrfCom */,
												ClassID /* classId */);

		HRESULT AllCallbacksWithDrilldown::ClassUnloadFinished(
                                                IPrfCom * /* pPrfCom */,
												ClassID /* classId */,
												HRESULT /* hrStatus */);

		HRESULT AllCallbacksWithDrilldown::ClassUnloadStarted(
                                                IPrfCom * /* pPrfCom */,
												ClassID /* classId */);

		HRESULT AllCallbacksWithDrilldown::JITCompilationFinished(
                                                IPrfCom * /* pPrfCom */,
												FunctionID /* functionId */,
												HRESULT /* hrStatus */,
												BOOL /* fIsSafeToBlock */);

		HRESULT AllCallbacksWithDrilldown::JITCompilationStarted(
                                                IPrfCom * /* pPrfCom */,
												FunctionID /* functionId */,
												BOOL /* fIsSafeToBlock */);

		HRESULT AllCallbacksWithDrilldown::ModuleAttachedToAssembly(
                                                IPrfCom * /* pPrfCom */,
												ModuleID /* moduleId */,
												AssemblyID /* AssemblyId */);

		HRESULT AllCallbacksWithDrilldown::ModuleLoadFinished(
                                                IPrfCom * /* pPrfCom */,
												ModuleID /* moduleId */,
												HRESULT /* hrStatus */);

		HRESULT AllCallbacksWithDrilldown::ModuleLoadStarted(
                                                IPrfCom * /* pPrfCom */,
												ModuleID /* moduleId */);

		HRESULT AllCallbacksWithDrilldown::ModuleUnloadFinished(
                                                IPrfCom * /* pPrfCom */,
												ModuleID /* moduleId */,
												HRESULT /* hrStatus */);

		HRESULT AllCallbacksWithDrilldown::ModuleUnloadStarted(
                                                IPrfCom * /* pPrfCom */,
												ModuleID /* moduleId */);

		HRESULT AllCallbacksWithDrilldown::ThreadAssignedToOSThread(
                                                IPrfCom * /* pPrfCom */,
												ThreadID /* managedThreadId */,
												DWORD /* osThreadId */);

		HRESULT AllCallbacksWithDrilldown::ThreadCreated(
                                                IPrfCom * /* pPrfCom */,
												ThreadID /* managedThreadId */);

		HRESULT AllCallbacksWithDrilldown::ThreadDestroyed(
                                                IPrfCom * /* pPrfCom */,
												ThreadID /* threadID */);

#pragma endregion

#pragma region Drilldown_Function_Prototypes

        HRESULT AllCallbacksWithDrilldown::AppDomainDrilldown(IPrfCom * pPrfCom,
                                        AppDomainID appDomainId,
                                        wstring callbackName,
                                        BOOL bAllowDataIncomplete);

        HRESULT AllCallbacksWithDrilldown::AssemblyDrilldown(IPrfCom * pPrfCom,
                                        AssemblyID assemblyId,
                                        wstring callbackName,
                                        BOOL bAllowDataIncomplete);

        HRESULT AllCallbacksWithDrilldown::ClassDrilldown(IPrfCom * pPrfCom,
                                        ClassID classId,
                                        wstring callbackName,
                                        BOOL bAllowDataIncomplete);

        HRESULT AllCallbacksWithDrilldown::FunctionDrilldown(IPrfCom * pPrfCom,
                                        FunctionID functionId,
                                        wstring callbackName,
                                        BOOL bAllowDataIncomplete);

        HRESULT AllCallbacksWithDrilldown::ModuleDrilldown(IPrfCom * pPrfCom,
                                        ModuleID moduleId,
                                        wstring callbackName,
                                        BOOL bAllowDataIncomplete);

#pragma endregion

        /* Maps to track which IDs have been seen before */
        typedef map<ObjectID, BOOL> InspectedMap;
        InspectedMap m_AppDomainMap;
        InspectedMap m_AssemblyMap;
        InspectedMap m_ClassMap;
        InspectedMap m_FunctionMap;
        InspectedMap m_ModuleMap;
        InspectedMap m_ThreadMap;

        MapFuncReturns AddIDToMap(InspectedMap *pmap, ObjectID objId);
        MapFuncReturns RemoveIDFromMap(InspectedMap *pmap, ObjectID objId);

        std::mutex m_InspectedMapsMutex;
        VOID GetInspectedMapsLock()
        {
            m_InspectedMapsMutex.lock();
        }
        VOID ReleaseInspectedMapsLock()
        {
            m_InspectedMapsMutex.unlock();
        }

        /*  Success/Failure counters */
        ULONG m_ulSuccess;
        ULONG m_ulFailure;
};


/*
 *  Initialize the AllCallbacksWithDrilldown class.  
 */
AllCallbacksWithDrilldown::AllCallbacksWithDrilldown(IPrfCom * /* pPrfCom */)
{
    // Initialize result counters
    m_ulSuccess = 0;
    m_ulFailure = 0;
}

/*
 *  Clean up
 */
AllCallbacksWithDrilldown::~AllCallbacksWithDrilldown()
{
}

MapFuncReturns AllCallbacksWithDrilldown::AddIDToMap(InspectedMap *pmap, ObjectID objId)
{
    MapFuncReturns retval = KeyWasInMap;

    this->GetInspectedMapsLock();

    InspectedMap::iterator iter = pmap->find(objId);
    if (iter == pmap->end())
    {
        pmap->insert(make_pair(objId, TRUE));
        retval = KeyWasNotInMap;
    }

	this->ReleaseInspectedMapsLock();

    return retval;
}

MapFuncReturns AllCallbacksWithDrilldown::RemoveIDFromMap(InspectedMap *pmap, ObjectID objId)
{
	MapFuncReturns retval = KeyWasNotInMap;

	this->GetInspectedMapsLock();

    InspectedMap::iterator iter = pmap->find(objId);
    if (iter != pmap->end())
    {
        pmap->erase(iter);
        retval = KeyWasInMap;
    }

    this->ReleaseInspectedMapsLock();

	return retval;
}

HRESULT AllCallbacksWithDrilldown::AppDomainDrilldown(IPrfCom * pPrfCom,
                                        AppDomainID appDomainId,
                                        wstring callbackName,
                                        BOOL bAllowDataIncomplete)
{
    wostringstream stdprefix;
    stdprefix << callbackName << L"(" << HEX(appDomainId) << L")";
    if (0 == appDomainId)
    {
        FAILURE(stdprefix.str() << L": AppDomainID cannot be NULL" );
        return E_FAIL;
    }
    PLATFORM_WCHAR appDomainName[STRING_LENGTH];
    PROFILER_WCHAR appDomainNameTemp[STRING_LENGTH];
    ULONG nameLength = 0;
    ProcessID processId = 0;
    HRESULT hr = PINFO->GetAppDomainInfo(appDomainId, STRING_LENGTH,
        &nameLength, appDomainNameTemp, &processId);
    if (hr == S_OK)
    {
        ConvertProfilerWCharToPlatformWChar(appDomainName, STRING_LENGTH, appDomainNameTemp, STRING_LENGTH);
        wstring name = appDomainName;
        DISPLAY(stdprefix.str() << L": AppDomain name is " << name << L".");
    }
    else if (hr == CORPROF_E_DATAINCOMPLETE)
    {
        if (bAllowDataIncomplete == TRUE)
        {
            DISPLAY(stdprefix.str() << L": GetAppDomainInfo() returned CORPROF_E_DATAINCOMPLETE.");
        }
        else
        {
		    FAILURE(stdprefix.str() << L": GetAppDomainInfo() returned CORPROF_E_DATAINCOMPLETE.");
        }
    }
    else
    {
	    FAILURE(stdprefix.str() << L": GetAppDomainInfo() returned " << HEX(hr));
    }
    return hr;
}

HRESULT AllCallbacksWithDrilldown::AppDomainCreationFinished(IPrfCom * pPrfCom,
                                        AppDomainID appDomainId,
                                        HRESULT hrStatus)
{
    if (FAILED(hrStatus))
    {
        DISPLAY(L"AppDomainCreationFinished: AppDomain Creation FAILED for AppDomainID "
            << HEX(appDomainId));
    }
    else
    {
        // TODO/Idea: Verify that mscorlib.dll loads at least once
        //            Verify name is non-empty
        this->AddIDToMap(&this->m_AppDomainMap, appDomainId);
        this->AppDomainDrilldown(pPrfCom, appDomainId, L"AppDomainCreationFinished", FALSE);
    }
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::AppDomainCreationStarted(IPrfCom * pPrfCom,
                                        AppDomainID appDomainId)
{
    this->AppDomainDrilldown(pPrfCom, appDomainId, L"AppDomainCreationStarted", TRUE);
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::AppDomainShutdownFinished(IPrfCom * pPrfCom,
                                        AppDomainID appDomainId,
                                        HRESULT hrStatus)
{
    if (FAILED(hrStatus))
    {
        DISPLAY(L"AppDomainShutdownFinished: AppDomain Shutdown FAILED for AppDomainID "
            << HEX(appDomainId));
    }
    else
    {
        if (KeyWasNotInMap == (this->RemoveIDFromMap(&this->m_AppDomainMap, appDomainId)))
        {
            FAILURE(L"AppDomainShutdownFinished: No matching AppDomainCreationFinished for AppDomainID "
                << HEX(appDomainId));
        }
    }
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::AppDomainShutdownStarted(IPrfCom *  pPrfCom ,
                                        AppDomainID  appDomainId )
{
    this->AppDomainDrilldown(pPrfCom, appDomainId, L"AppDomainShutdownStarted", FALSE);
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::AssemblyDrilldown(IPrfCom * pPrfCom,
                                        AssemblyID assemblyId,
                                        wstring callbackName,
                                        BOOL bAllowDataIncomplete)
{
    wostringstream stdprefix;
    stdprefix << callbackName << L"(" << HEX(assemblyId) << L")";
    if (0 == assemblyId)
    {
        FAILURE(stdprefix.str() << L": AssemblyID cannot be NULL");
        return E_FAIL;
    }
    ULONG dummy;
    AppDomainID appDomainId;
    ModuleID moduleId;
    WCHAR assemblyName[LONG_LENGTH];
    HRESULT hr = PINFO->GetAssemblyInfo( assemblyId,
	 								       LONG_LENGTH,
	                                       &dummy,
	                                       assemblyName,
	                                       &appDomainId,
									       &moduleId );
    if ( hr == S_OK )
    {
        wstring name;
        MUST_PASS(hr = pPrfCom->GetAssemblyIDName(assemblyId, name, TRUE));
        DISPLAY(stdprefix.str() << L": Assembly name is: " << name << L".");
	    if ((assemblyName[0] != 0) && (name.size() > 0))
	    {
		    if ( appDomainId != 0 )
		    {
			    if ( moduleId == 0 )
                {
				    FAILURE(stdprefix.str() << L": NULL Module ID from GetAssemblyInfo()");
                    hr = E_FAIL;
                }
		    }
		    else
            {
			    FAILURE(stdprefix.str() << L": NULL AppDomain ID from GetAssemblyInfo()");
                hr = E_FAIL;
            }
        }
	    else
        {
            FAILURE(stdprefix.str() << L": Zero length Assembly Name from GetAssemblyInfo()");
            hr = E_FAIL;
        }
    }
    else if (hr == CORPROF_E_DATAINCOMPLETE)
    {
        if (bAllowDataIncomplete == TRUE)
        {
            DISPLAY(stdprefix.str() << L": GetAssemblyInfo() returned CORPROF_E_DATAINCOMPLETE");
        }
        else
        {
            FAILURE(stdprefix.str() << L": GetAssemblyInfo() returned CORPROF_E_DATAINCOMPLETE");
        }
    }
    else
    {
        FAILURE(stdprefix.str() << L": GetAssemblyInfo() returned " << HEX(hr));
    }
    return hr;
}

HRESULT AllCallbacksWithDrilldown::AssemblyLoadFinished(IPrfCom * pPrfCom,
                                        AssemblyID assemblyId,
                                        HRESULT hrStatus)
{
    if (FAILED(hrStatus))
    {
        DISPLAY(L"AssemblyLoadFinished: Assembly Load FAILED for AssemblyID "
            << HEX(assemblyId));
    }
    else
    {
        this->AddIDToMap(&this->m_AssemblyMap, assemblyId);
        this->AssemblyDrilldown(pPrfCom, assemblyId, L"AssemblyLoadFinished", FALSE);
    }
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::AssemblyLoadStarted(IPrfCom * pPrfCom,
                                        AssemblyID assemblyId)
{
    this->AssemblyDrilldown(pPrfCom, assemblyId, L"AssemblyLoadStarted", TRUE);
    return S_OK;
}

HRESULT AllCallbacksWithDrilldown::AssemblyUnloadFinished(IPrfCom * pPrfCom,
                                        AssemblyID assemblyId,
                                        HRESULT hrStatus)
{
    if (FAILED(hrStatus))
    {
        DISPLAY(L"AssemblyUnloadFinished: Assembly Unload FAILED for AssemblyID "
            << HEX(assemblyId));
    }
    else
    {
        if (KeyWasNotInMap == (this->RemoveIDFromMap(&this->m_AssemblyMap, assemblyId)))
        {
            FAILURE(L"AssemblyUnloadFinished: No AssemblyLoadFinished for AssemblyID "
                << HEX(assemblyId));
        }
    }
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::AssemblyUnloadStarted(IPrfCom * pPrfCom,
                                        AssemblyID assemblyId )
{
    this->AssemblyDrilldown(pPrfCom, assemblyId, L"AssemblyUnloadStarted", FALSE);
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::ClassDrilldown(IPrfCom * pPrfCom,
                                        ClassID classId,
                                        wstring callbackName,
                                        BOOL bAllowDataIncomplete)
{
    wostringstream stdprefix;
    stdprefix << callbackName << L"(" << HEX(classId) << L")";
    if (0 == classId)
    {
	    FAILURE(stdprefix.str() << L": ClassID cannot be NULL");
        return E_FAIL;
    }
    ModuleID modId;
    mdTypeDef classToken;
    ClassID parentClassID;
    ULONG32 nTypeArgs;
    ClassID typeArgs[SHORT_LENGTH];
    HRESULT hr = PINFO->GetClassIDInfo2(classId,
                                &modId,
                                &classToken,
                                &parentClassID,
                                SHORT_LENGTH,
                                &nTypeArgs,
                                typeArgs);
    if ((S_OK == hr) || (CORPROF_E_CLASSID_IS_ARRAY == hr) || (CORPROF_E_CLASSID_IS_COMPOSITE == hr))
    {
        wstring name;
        MUST_PASS(hr = pPrfCom->GetClassIDName(classId, name, TRUE));
        DISPLAY(stdprefix.str() << L": Class name is: " << name << L".");
    }
    else if (hr == CORPROF_E_DATAINCOMPLETE)
    {
        if (bAllowDataIncomplete == TRUE)
        {
            DISPLAY(stdprefix.str() << L": GetClassIDInfo2() returned CORPROF_E_DATAINCOMPLETE");
        }
        else
        {
            FAILURE(stdprefix.str() << L": GetClassIDInfo2() returned CORPROF_E_DATAINCOMPLETE");
        }
    }
    else
    {
        FAILURE(stdprefix.str() << L": GetClassIDInfo2 returned " << HEX(hr));
    }
    return hr;
}

HRESULT AllCallbacksWithDrilldown::ClassLoadFinished(IPrfCom * pPrfCom,
                                        ClassID classId,
                                        HRESULT hrStatus)
{
    if (FAILED(hrStatus))
    {
        DISPLAY(L"ClassLoadFinished: Class Load FAILED for ClassID " << HEX(classId));
    }
    else
    {
        this->AddIDToMap(&this->m_ClassMap, classId);
        this->ClassDrilldown(pPrfCom, classId, L"ClassLoadFinished", FALSE);
    }
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::ClassLoadStarted(IPrfCom * pPrfCom,
                                        ClassID classId)
{
    this->ClassDrilldown(pPrfCom, classId, L"ClassLoadStarted", TRUE);
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::ClassUnloadFinished(IPrfCom * pPrfCom,
                                        ClassID classId,
                                        HRESULT hrStatus)
{
    if (FAILED(hrStatus))
    {
        DISPLAY(L"ClassUnloadFinished: Class Unload FAILED for ClassID " << HEX(classId));
    }
    else
    {
        if (KeyWasNotInMap == (this->RemoveIDFromMap(&this->m_ClassMap, classId)))
        {
            FAILURE(L"ClassUnloadFinished: No matching ClassLoadFinished for ClassID "
                << HEX(classId));
        }
    }
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::ClassUnloadStarted(IPrfCom *  pPrfCom,
                                        ClassID classId)
{
    this->ClassDrilldown(pPrfCom, classId, L"ClassUnloadStarted", FALSE);
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::FunctionDrilldown(IPrfCom * pPrfCom,
                                        FunctionID functionId,
                                        wstring callbackName,
                                        BOOL bAllowDataIncomplete)
{
    wostringstream stdprefix;
    stdprefix << callbackName << L"(" << HEX(functionId) << L")";
    if (0 == functionId)
    {
	    DISPLAY(stdprefix.str() << L": FunctionID is NULL, Unknown Native Function");
        return S_OK;
    }

    HRESULT hr;
    COR_PRF_FRAME_INFO frameInfo = NULL;
    ClassID classId = NULL;
    ModuleID moduleId = NULL;
    mdToken token = NULL;
    ULONG32 nTypeArgs = NULL;
    ClassID typeArgs[SHORT_LENGTH];

    hr = PINFO->GetFunctionInfo2(functionId, 
                                  frameInfo,
                                  &classId,
                                  &moduleId,
                                  &token,
                                  SHORT_LENGTH,
                                  &nTypeArgs,
                                  typeArgs);
    if (S_OK == hr)
    {
        wstring name;
        MUST_PASS(pPrfCom->GetFunctionIDName(functionId, name, NULL, TRUE));
        DISPLAY(stdprefix.str() << L": Function name is: " << name << L".");
    }
    else if (hr == CORPROF_E_DATAINCOMPLETE)
    {
        if (bAllowDataIncomplete == TRUE)
        {
            DISPLAY(stdprefix.str() << L": GetFunctionInfo2() returned CORPROF_E_DATAINCOMPLETE");
        }
        else
        {
            FAILURE(stdprefix.str() << L": GetFunctionInfo2() returned CORPROF_E_DATAINCOMPLETE");
        }
    }
    else
    {
        FAILURE(stdprefix.str() << L": GetFunctionInfo2 returned " << HEX(hr));
    }
    return hr;
}

HRESULT AllCallbacksWithDrilldown::JITCompilationFinished(IPrfCom *pPrfCom,
                                        FunctionID functionId,
                                        HRESULT hrStatus,
                                        BOOL /* fIsSafeToBlock */)
{
    if (FAILED(hrStatus))
    {
        DISPLAY(L"JITCompilationFinished: JIT Compilation FAILED for FunctionID "
            << HEX(functionId));
    }
    else
    {
        this->AddIDToMap(&this->m_FunctionMap, functionId);
        this->FunctionDrilldown(pPrfCom, functionId, L"JITCompilationFinished", FALSE);
    }
    return S_OK;
}

HRESULT AllCallbacksWithDrilldown::JITCompilationStarted(IPrfCom * pPrfCom,
                                        FunctionID functionId,
                                        BOOL /* fIsSafeToBlock */)
{
    this->FunctionDrilldown(pPrfCom, functionId, L"JITCompilationStarted", TRUE);
    return S_OK;
}

HRESULT AllCallbacksWithDrilldown::ModuleDrilldown(IPrfCom * pPrfCom,
                                        ModuleID moduleId,
                                        wstring callbackName,
                                        BOOL bAllowDataIncomplete)
{
    wostringstream stdprefix;
    stdprefix << callbackName << L"(" << HEX(moduleId) << L")";
    if (0 == moduleId)
    {
	    FAILURE(stdprefix.str() << L": ModuleID cannot be NULL");
        return E_FAIL;
    }

    WCHAR moduleName[STRING_LENGTH];
    ULONG nameLength = 0;
    AssemblyID assemblyId;

    HRESULT hr = PINFO->GetModuleInfo(moduleId, NULL, STRING_LENGTH, &nameLength,
                                   moduleName, &assemblyId);

    if (S_OK == hr)
    {
        wstring name;
        MUST_PASS(pPrfCom->GetModuleIDName(moduleId, name, TRUE));
        DISPLAY(stdprefix.str() << L": Module name is " << name << L".");
    }
    else if (hr == CORPROF_E_DATAINCOMPLETE)
    {
        if (bAllowDataIncomplete == TRUE)
        {
            DISPLAY(stdprefix.str() << L": GetModuleInfo() returned CORPROF_E_DATAINCOMPLETE");
        }
        else
        {
            FAILURE(stdprefix.str() << L": GetModuleInfo() returned CORPROF_E_DATAINCOMPLETE");
        }
    }
    else
    {
        FAILURE(stdprefix.str() << L": GetModuleInfo returned " << HEX(hr));
    }
    return hr;
}

HRESULT AllCallbacksWithDrilldown::ModuleAttachedToAssembly(IPrfCom * pPrfCom,
                                        ModuleID moduleId,
                                        AssemblyID assemblyId)
{
    if (assemblyId == PROFILER_PARENT_UNKNOWN)
    {
        FAILURE(L"ModuleAttachedToAssembly: PROFILER_PARENT_UNKNOWN for ModuleID "
            << HEX(moduleId));
    }
    else
    {
        this->ModuleDrilldown(pPrfCom, moduleId, L"ModuleAttachedToAssembly", FALSE);
        this->AssemblyDrilldown(pPrfCom, assemblyId, L"ModuleAttachedToAssembly", FALSE);
    }
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::ModuleLoadFinished(IPrfCom * pPrfCom,
                                        ModuleID moduleId,
                                        HRESULT hrStatus)
{
    if (FAILED(hrStatus))
    {
        DISPLAY(L"ModuleLoadFinished: Module Load FAILED for ModuleID " << HEX(moduleId));
    }
    else
    {
        this->AddIDToMap(&this->m_ModuleMap, moduleId);
        this->ModuleDrilldown(pPrfCom, moduleId, L"ModuleLoadFinished", FALSE);
    }
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::ModuleLoadStarted(IPrfCom *pPrfCom,
                                        ModuleID moduleId)
{
    this->ModuleDrilldown(pPrfCom, moduleId, L"ModuleLoadStarted", TRUE);
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::ModuleUnloadFinished(IPrfCom * pPrfCom,
                                        ModuleID moduleId,
                                        HRESULT hrStatus)
{
    if (FAILED(hrStatus))
    {
        DISPLAY(L"ModuleUnloadFinished: Module Unload FAILED for ModuleID " << HEX(moduleId));
    }
    else
    {
        if (KeyWasNotInMap == (this->RemoveIDFromMap(&this->m_ModuleMap, moduleId)))
        {
            FAILURE(L"ModuleUnloadFinished: No matching ModuleLoadFinished for ModuleID "
                << HEX(moduleId));
        }
    }
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::ModuleUnloadStarted(IPrfCom *pPrfCom,
                                        ModuleID moduleId)
{
    this->ModuleDrilldown(pPrfCom, moduleId, L"ModuleUnloadStarted", FALSE);
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::ThreadAssignedToOSThread(IPrfCom * pPrfCom,
                                        ThreadID managedThreadId,
                                        DWORD osThreadId)
{
    wostringstream stdprefix;
    stdprefix << L"ThreadAssignedToOSThread(" << HEX(managedThreadId) << L"," << HEX(osThreadId) << L")";
    if (0 == managedThreadId)
    {
	    FAILURE(stdprefix.str() << L": managedThreadId cannot be NULL");
        return E_FAIL;
    }
    if (0 == osThreadId)
    {
	    FAILURE(stdprefix.str() << L": osThreadId cannot be NULL");
        return E_FAIL;
    }
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::ThreadCreated(IPrfCom * pPrfCom,
                                        ThreadID managedThreadId)
{
    this->AddIDToMap(&this->m_ThreadMap, managedThreadId);
    ThreadID myThreadId;
    MUST_PASS(PINFO->GetCurrentThreadID(&myThreadId));
    if (myThreadId == managedThreadId)
    {
        DWORD dwWin32ThreadId;
        HANDLE hThread;
        ContextID contextId;
        MUST_PASS(PINFO->GetThreadInfo(managedThreadId, &dwWin32ThreadId));
        MUST_PASS(PINFO->GetHandleFromThread(managedThreadId, &hThread));
        MUST_PASS(PINFO->GetThreadContext(managedThreadId, &contextId));
        DISPLAY(L"ThreadCreated: Thread " << HEX(managedThreadId) << L" created");
    }
    else
    {
        FAILURE(L"ThreadCreated: GetCurrentThreadID returned a different ThreadID than the callback: "
            << HEX(managedThreadId) << L" vs. " << HEX(myThreadId));
    }
	return S_OK;
}

HRESULT AllCallbacksWithDrilldown::ThreadDestroyed(IPrfCom * pPrfCom ,
                                        ThreadID threadId)
{
    if (0 == threadId)
    {
        FAILURE(L"ThreadDestroyed call with a NULL threadId");
    }
    else 
    {
        if (KeyWasNotInMap == (this->RemoveIDFromMap(&this->m_ThreadMap, threadId)))
        {
            FAILURE(L"ThreadDestroyed: No matching ThreadCreated for ThreadID <"
                << HEX(threadId) << L">");
        }
    }
	return S_OK;
}


/*
 *  Verify the results of this run.
 */
HRESULT AllCallbacksWithDrilldown::Verify(IPrfCom * pPrfCom)
{
	DISPLAY(L"AppDomainCreationStarted:  " << pPrfCom->m_AppDomainCreationStarted );
	DISPLAY(L"AppDomainCreationFinished: " << pPrfCom->m_AppDomainCreationFinished );
	if ((pPrfCom->m_AppDomainCreationStarted == pPrfCom->m_AppDomainCreationFinished) &&
        (pPrfCom->m_AppDomainCreationStarted > 0))
	{
		m_ulSuccess++;
	}
	else
	{
		m_ulFailure++;
		FAILURE(L"Unexpected AppDomainCreationStarted/Finished count");
	}

    DISPLAY(L"AssemblyLoadStarted:       " << pPrfCom->m_AssemblyLoadStarted );
    DISPLAY(L"AssemblyLoadFinished:      " << pPrfCom->m_AssemblyLoadFinished );
    if ((pPrfCom->m_AssemblyLoadStarted == pPrfCom->m_AssemblyLoadFinished) &&
        (pPrfCom->m_AssemblyLoadStarted > 0))
	{
		m_ulSuccess++;
	}
	else
	{
		m_ulFailure++;
		FAILURE(L"Unexpected AssemblyLoadStarted/Finished count");
	}

    DISPLAY(L"AssemblyUnloadStarted:     " << pPrfCom->m_AssemblyUnloadStarted );
    DISPLAY(L"AssemblyUnloadFinished:    " << pPrfCom->m_AssemblyUnloadFinished );
    if (pPrfCom->m_AssemblyUnloadStarted == pPrfCom->m_AssemblyUnloadFinished)
	{
		m_ulSuccess++;
	}
	else
	{
		m_ulFailure++;
		FAILURE(L"Unexpected AssemblyUnloadStarted/Finished count");
	}

    DISPLAY(L"ClassLoadStarted:       " << pPrfCom->m_ClassLoadStarted);
    DISPLAY(L"ClassLoadFinished:      " << pPrfCom->m_ClassLoadFinished);
    if ((pPrfCom->m_ClassLoadStarted == pPrfCom->m_ClassLoadFinished) &&
        (pPrfCom->m_ClassLoadStarted > 0))
	{
		m_ulSuccess++;
	}
	else
	{
		m_ulFailure++;
		FAILURE(L"Unexpected ClassLoadStarted/Finished count");
	}

    DISPLAY(L"ModuleLoadStarted:       " << pPrfCom->m_ModuleLoadStarted);
    DISPLAY(L"ModuleLoadFinished:      " << pPrfCom->m_ModuleLoadFinished );
    if ((pPrfCom->m_ModuleLoadStarted == pPrfCom->m_ModuleLoadFinished) &&
        (pPrfCom->m_ModuleLoadStarted > 0))
	{
		m_ulSuccess++;
	}
	else
	{
		m_ulFailure++;
		FAILURE(L"Unexpected ModuleLoadStarted/Finished count");
	}

    if ((m_ulFailure == 0) && (m_ulSuccess > 0))
    {
        DISPLAY(L"Test passed!")
        return S_OK;
    }
    else
    {
        FAILURE(L"Either some checks failed, or no successful checks were completed.")
        return E_FAIL;
    }
}


/*
 *  Verification routine called by TestProfiler
 */
HRESULT AllCallbacksWithDrilldown_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(AllCallbacksWithDrilldown);
    HRESULT hr = pAllCallbacksWithDrilldown->Verify(pPrfCom);

    // Clean up instance of test class
    delete pAllCallbacksWithDrilldown;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}


/*
 *  Initialization routine called by TestProfiler
 */
void AllCallbacksWithDrilldown_Initialize(IPrfCom * pPrfCom,
                                          PMODULEMETHODTABLE pModuleMethodTable)
{
    // Create and save an instance of test class
    // Retrieve it in a method using LOCAL_CLASS_POINTER(AllCallbacksWithDrilldown);
    SET_CLASS_POINTER( new AllCallbacksWithDrilldown(pPrfCom) );

    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ALL
		                        & ~COR_PRF_MONITOR_ENTERLEAVE
								& ~COR_PRF_MONITOR_EXCEPTIONS
                                & ~COR_PRF_ENABLE_REJIT;

	REGISTER_CALLBACK(APPDOMAINCREATIONFINISHED, AllCallbacksWithDrilldown::AppDomainCreationFinishedWrapper);
	REGISTER_CALLBACK(APPDOMAINCREATIONSTARTED, AllCallbacksWithDrilldown::AppDomainCreationStartedWrapper);
	REGISTER_CALLBACK(APPDOMAINSHUTDOWNFINISHED, AllCallbacksWithDrilldown::AppDomainShutdownFinishedWrapper);
	REGISTER_CALLBACK(APPDOMAINSHUTDOWNSTARTED, AllCallbacksWithDrilldown::AppDomainShutdownStartedWrapper);
	REGISTER_CALLBACK(ASSEMBLYLOADFINISHED, AllCallbacksWithDrilldown::AssemblyLoadFinishedWrapper);
	REGISTER_CALLBACK(ASSEMBLYLOADSTARTED, AllCallbacksWithDrilldown::AssemblyLoadStartedWrapper);
	REGISTER_CALLBACK(ASSEMBLYUNLOADFINISHED, AllCallbacksWithDrilldown::AssemblyUnloadFinishedWrapper);
	REGISTER_CALLBACK(ASSEMBLYUNLOADSTARTED, AllCallbacksWithDrilldown::AssemblyUnloadStartedWrapper);
	REGISTER_CALLBACK(CLASSLOADFINISHED, AllCallbacksWithDrilldown::ClassLoadFinishedWrapper);
	REGISTER_CALLBACK(CLASSLOADSTARTED, AllCallbacksWithDrilldown::ClassLoadStartedWrapper);
	REGISTER_CALLBACK(CLASSUNLOADFINISHED, AllCallbacksWithDrilldown::ClassUnloadFinishedWrapper);
	REGISTER_CALLBACK(CLASSUNLOADSTARTED, AllCallbacksWithDrilldown::ClassUnloadStartedWrapper);
	REGISTER_CALLBACK(JITCOMPILATIONFINISHED, AllCallbacksWithDrilldown::JITCompilationFinishedWrapper);
	REGISTER_CALLBACK(JITCOMPILATIONSTARTED, AllCallbacksWithDrilldown::JITCompilationStartedWrapper);
	REGISTER_CALLBACK(MODULEATTACHEDTOASSEMBLY, AllCallbacksWithDrilldown::ModuleAttachedToAssemblyWrapper);
	REGISTER_CALLBACK(MODULELOADFINISHED, AllCallbacksWithDrilldown::ModuleLoadFinishedWrapper);
	REGISTER_CALLBACK(MODULELOADSTARTED, AllCallbacksWithDrilldown::ModuleLoadStartedWrapper);
	REGISTER_CALLBACK(MODULEUNLOADFINISHED, AllCallbacksWithDrilldown::ModuleUnloadFinishedWrapper);
	REGISTER_CALLBACK(MODULEUNLOADSTARTED, AllCallbacksWithDrilldown::ModuleUnloadStartedWrapper);
	REGISTER_CALLBACK(THREADASSIGNEDTOOSTHREAD, AllCallbacksWithDrilldown::ThreadAssignedToOSThreadWrapper);
	REGISTER_CALLBACK(THREADCREATED, AllCallbacksWithDrilldown::ThreadCreatedWrapper);
	REGISTER_CALLBACK(THREADDESTROYED, AllCallbacksWithDrilldown::ThreadDestroyedWrapper);
    REGISTER_CALLBACK(VERIFY, AllCallbacksWithDrilldown_Verify);

    return;
}
