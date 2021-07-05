// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ShadowStackTest.cpp
//
// 
// ======================================================================================

#include "JITCommon.h"

// Legacy support
#include "../LegacyCompat.h"
// Legacy support

/*

Test Overview:

This test monitors JIT Compilation, JIT Cache Search, Function Enter/Leave and Exception events.

It validates that functions
	- start JIT compilation before finishing
	- start a cache search before finishing
	- are not both cached and compiled
	- have been seen in a jit event before entering

It maintains a shadow stack using Enter, Leave, TailCall and ExceptionUnwind events validating
    that the function being left is on the top.

It also validates
    - jit compilation started count is equal to finished count
    - jit function search started count is equal to finished count
	- the function enter count is equal to the sum of function leave, tailcall and unwind counts

*/

class ShadowStackTest
{
    public:

        ShadowStackTest(IPrfCom * pPrfCom);
        ~ShadowStackTest();

		BOOL m_bEnableSlowEnterLeave;
		BOOL m_bEnableDoStackSnapshot;

		static HRESULT ThreadCreatedWrapper(IPrfCom * pPrfCom,
                                        ThreadID managedThreadId)
		{
            return STATIC_CLASS_CALL(ShadowStackTest)->ThreadCreated(pPrfCom,
                                        managedThreadId);
		}
		static HRESULT ThreadDestroyedWrapper(IPrfCom * pPrfCom,
                                        ThreadID threadID)
		{
            return STATIC_CLASS_CALL(ShadowStackTest)->ThreadDestroyed(pPrfCom,
                                        threadID);
		}
		static HRESULT JITCompilationStartedWrapper(IPrfCom * pPrfCom,
                                        FunctionID functionId,
                                        BOOL fIsSafeToBlock)
		{
            return STATIC_CLASS_CALL(ShadowStackTest)->JITCompilationStarted(pPrfCom,
                                        functionId,
                                        fIsSafeToBlock);
		}
		static HRESULT JITCompilationFinishedWrapper(IPrfCom * pPrfCom,
                                        FunctionID functionId,
                                        HRESULT hrStatus,
                                        BOOL fIsSafeToBlock)
		{
            return STATIC_CLASS_CALL(ShadowStackTest)->JITCompilationFinished(pPrfCom,
                                        functionId,
                                        hrStatus,
                                        fIsSafeToBlock);
		}
		static HRESULT JITCachedFunctionSearchStartedWrapper(IPrfCom * pPrfCom,
                                        FunctionID functionId,
                                        BOOL *pbUseCachedFunction)
		{
            return STATIC_CLASS_CALL(ShadowStackTest)->JITCachedFunctionSearchStarted(pPrfCom,
                                        functionId,
                                        pbUseCachedFunction);
		}
		static HRESULT JITCachedFunctionSearchFinishedWrapper(IPrfCom * pPrfCom,
                                        FunctionID functionId,
                                        COR_PRF_JIT_CACHE result)
		{
            return STATIC_CLASS_CALL(ShadowStackTest)->JITCachedFunctionSearchFinished(pPrfCom,
                                        functionId,
                                        result);
		}
		static HRESULT JITFunctionPitchedWrapper(IPrfCom * pPrfCom,
                                        FunctionID functionId)
		{
            return STATIC_CLASS_CALL(ShadowStackTest)->JITFunctionPitched(pPrfCom,
                                        functionId);
		}
		static VOID FunctionEnter2Wrapper(IPrfCom * pPrfCom,
                                        FunctionID mappedFuncId,
                                        UINT_PTR clientData,
                                        COR_PRF_FRAME_INFO func,
                                        COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
        {
            return STATIC_CLASS_CALL(ShadowStackTest)->FunctionEnter2(pPrfCom,
                                        mappedFuncId,
                                        clientData,
                                        func,
                                        argumentInfo); 
        }

		
        static VOID FunctionLeave2Wrapper(IPrfCom * pPrfCom,
                                        FunctionID mappedFuncId,
                                        UINT_PTR clientData,
                                        COR_PRF_FRAME_INFO func,
                                        COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange) 
        {
            return STATIC_CLASS_CALL(ShadowStackTest)->FunctionLeave2(pPrfCom,
                                        mappedFuncId,
                                        clientData,
                                        func,
                                        retvalRange); 
        }
        static VOID FunctionTailCall2Wrapper(IPrfCom * pPrfCom,
                                        FunctionID mappedFuncId,
                                        UINT_PTR clientData,
                                        COR_PRF_FRAME_INFO func) 
        {
            return STATIC_CLASS_CALL(ShadowStackTest)->FunctionTailCall2(pPrfCom,
                                        mappedFuncId,
                                        clientData,
                                        func); 
        }

		static VOID FunctionEnter3Wrapper(IPrfCom * pPrfCom, FunctionIDOrClientID functionIdOrClientID)
        {
			ShadowStackTest * sst = reinterpret_cast<ShadowStackTest * >(pPrfCom->m_pTestClassInstance);
            return sst->FunctionEnter3(pPrfCom,functionIdOrClientID);
        }

		static VOID FunctionLeave3Wrapper(IPrfCom * pPrfCom, FunctionIDOrClientID functionIdOrClientID)
        {
			ShadowStackTest * sst = reinterpret_cast<ShadowStackTest * >(pPrfCom->m_pTestClassInstance);
            return sst->FunctionLeave3(pPrfCom,functionIdOrClientID);
        }

		static VOID FunctionTailCall3Wrapper(IPrfCom * pPrfCom, FunctionIDOrClientID functionIdOrClientID)
        {
			ShadowStackTest * sst = reinterpret_cast<ShadowStackTest * >(pPrfCom->m_pTestClassInstance);
            return sst->FunctionTailCall3(pPrfCom,functionIdOrClientID);
        }

		static VOID FunctionEnter3WithInfoWrapper(IPrfCom * pPrfCom, FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)
        {
			ShadowStackTest * sst = reinterpret_cast<ShadowStackTest * >(pPrfCom->m_pTestClassInstance);
            return sst->FunctionEnter3WithInfo(pPrfCom,functionIdOrClientID, eltInfo);
        }

		static VOID FunctionLeave3WithInfoWrapper(IPrfCom * pPrfCom, FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)
        {
			ShadowStackTest * sst = reinterpret_cast<ShadowStackTest * >(pPrfCom->m_pTestClassInstance);
            return sst->FunctionLeave3WithInfo(pPrfCom,functionIdOrClientID, eltInfo);
        }

		static VOID FunctionTailCall3WithInfoWrapper(IPrfCom * pPrfCom, FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)
        {
			ShadowStackTest * sst = reinterpret_cast<ShadowStackTest * >(pPrfCom->m_pTestClassInstance);
            return sst->FunctionTailCall3WithInfo(pPrfCom,functionIdOrClientID, eltInfo);
        }

		static HRESULT ExceptionUnwindFunctionLeaveWrapper(IPrfCom * pPrfCom)
		{
			return STATIC_CLASS_CALL(ShadowStackTest)->ExceptionUnwindFunctionLeave(pPrfCom);
		}

        HRESULT Verify(IPrfCom * pPrfCom);

    private:

		HRESULT ThreadCreated(IPrfCom * pPrfCom,
                                        ThreadID managedThreadId);
		HRESULT ThreadDestroyed(IPrfCom * pPrfCom,
                                        ThreadID threadID);
		HRESULT JITCompilationStarted(IPrfCom * pPrfCom,
                                        FunctionID functionId,
										BOOL fIsSafeToBlock);
		HRESULT JITCompilationFinished(IPrfCom * pPrfCom,
                                        FunctionID functionId,
                                        HRESULT hrStatus,
										BOOL fIsSafeToBlock);
		HRESULT JITCachedFunctionSearchStarted(IPrfCom * pPrfCom,
                                        FunctionID functionId,
										BOOL *pbUseCachedFunction);
		HRESULT JITCachedFunctionSearchFinished(IPrfCom * pPrfCom,
                                        FunctionID functionId,
                                        COR_PRF_JIT_CACHE result);
		HRESULT JITFunctionPitched(IPrfCom * pPrfCom,
                                        FunctionID functionId);
        VOID FunctionEnter2(IPrfCom * pPrfCom,
                                        FunctionID mappedFuncId,
                                        UINT_PTR clientData,
                                        COR_PRF_FRAME_INFO func,
                                        COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo);
        VOID FunctionLeave2(IPrfCom * pPrfCom,
                                        FunctionID mappedFuncId,
                                        UINT_PTR clientData,
                                        COR_PRF_FRAME_INFO func,
                                        COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange);
        VOID FunctionTailCall2(IPrfCom * pPrfCom,
                                        FunctionID mappedFuncId,
                                        UINT_PTR clientData,
                                        COR_PRF_FRAME_INFO func);
		VOID FunctionEnter3(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID);
		VOID FunctionLeave3(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID);
		VOID FunctionTailCall3(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID);
		VOID FunctionEnter3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo);
		VOID FunctionLeave3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo);
		VOID FunctionTailCall3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo);
		HRESULT ExceptionUnwindFunctionLeave(IPrfCom * pPrfCom);

        /*  Success/Failure counters */
        ULONG m_ulSuccess;
        ULONG m_ulFailure;
};


/*
 *  Initialize the ShadowStackTest class.  
 */
ShadowStackTest::ShadowStackTest(IPrfCom * /* pPrfCom */)
{
    // Initialize result counters
    m_ulSuccess = 0;
    m_ulFailure = 0;
}


/*
 *  Clean up
 */
ShadowStackTest::~ShadowStackTest()
{
}

HRESULT ShadowStackTest::ThreadCreated(IPrfCom * pPrfCom,
                                ThreadID threadId)
{
    DERIVED_COMMON_POINTER(IJITCom);
	if (FAILED(pIJITCom->AddThreadToMap(threadId)))
	{
		this->m_ulFailure++;
	}
    return S_OK;
}

HRESULT ShadowStackTest::ThreadDestroyed(IPrfCom * pPrfCom,
                                ThreadID threadId)
{
    IJITCom * pJITCom = dynamic_cast<IJITCom *>(pPrfCom);
	if (FAILED(pJITCom->RemoveThreadFromMap(threadId)))
	{
		this->m_ulFailure++;
	}
    return S_OK;
}

HRESULT ShadowStackTest::JITCompilationStarted(IPrfCom * pPrfCom,
                                FunctionID functionId,
								BOOL /* fIsSafeToBlock */)
{
    IJITCom * pJITCom = dynamic_cast<IJITCom *>(pPrfCom);
	if (FAILED(pJITCom->AddFunctionToMap(functionId)))
	{
		this->m_ulFailure++;
	}
	FunctionInfo *fi = pJITCom->GetFunctionFromMap(functionId);
	if (NULL != fi)
	{
		fi->IncrementJITCompilationStarted();
	}
    return S_OK;
}

HRESULT ShadowStackTest::JITCompilationFinished(IPrfCom * pPrfCom,
                                FunctionID functionId,
                                HRESULT hrStatus,
								BOOL /* fIsSafeToBlock */)
{
    IJITCom * pJITCom = dynamic_cast<IJITCom *>(pPrfCom);
	FunctionInfo *fi = pJITCom->GetFunctionFromMap(functionId);
	if (NULL == fi)
	{
		FAILURE(L"JITCompilationFinished for function that never started");
	}
	else
	{
		fi->IncrementJITCompilationFinished();
		if (SUCCEEDED(hrStatus))
		{
			if (FAILED(fi->SetFunctionFinishedJIT()))
			{
				this->m_ulFailure++;
			}
		}
		else
		{
			DISPLAY(L"JITCompilation failed for function " << HEX(functionId));
		}
	}
    return S_OK;
}

HRESULT ShadowStackTest::JITCachedFunctionSearchStarted(IPrfCom * pPrfCom,
                                FunctionID functionId,
								BOOL * /* pbUseCachedFunction */)
{
    IJITCom * pJITCom = dynamic_cast<IJITCom *>(pPrfCom);
	if (FAILED(pJITCom->AddFunctionToMap(functionId)))
	{
		this->m_ulFailure++;
	}
    return S_OK;
}

HRESULT ShadowStackTest::JITCachedFunctionSearchFinished(IPrfCom * pPrfCom,
                                FunctionID functionId,
                                COR_PRF_JIT_CACHE result)
{
    IJITCom * pJITCom = dynamic_cast<IJITCom *>(pPrfCom);
	FunctionInfo *fi = pJITCom->GetFunctionFromMap(functionId);
	if (NULL == fi)
	{
		FAILURE(L"JITCachedFunctionSearchFinished for function that never started");
	}
	else
	{
		if (result == COR_PRF_CACHED_FUNCTION_FOUND)
		{
			if (FAILED(fi->SetFunctionCached()))
			{
				this->m_ulFailure++;
			}
		}
		else
		{
			pJITCom->RemoveFunctionFromMap(functionId);
			DISPLAY(L"JITCachedFunctionSearchFinished failed for function " << HEX(functionId));
		}
	}
    return S_OK;
}

HRESULT ShadowStackTest::JITFunctionPitched(IPrfCom * pPrfCom,
                                FunctionID /* functionId */)
{
	FAILURE(L"JITFunctionPitched was called but is supposed to obsolete!");
    return S_OK;
}

VOID ShadowStackTest::FunctionEnter3(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID)
{
	//For Shadowstack testing in ELT, we ignore clientData, func and argumentinfo, passing NULL and calling FunctionEnter2 verbatim
	FunctionEnter2(pPrfCom, functionIDOrClientID.functionID, NULL, NULL, NULL);
}

VOID ShadowStackTest::FunctionLeave3(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID)
{
	//For Shadowstack testing in ELT, we ignore clientData, func and argumentinfo, passing NULL and calling FunctionEnter2 verbatim
	FunctionLeave2(pPrfCom, functionIDOrClientID.functionID, NULL, NULL, NULL);
}

VOID ShadowStackTest::FunctionTailCall3(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID)
{
	//For Shadowstack testing in ELT, we ignore clientData, func and argumentinfo, passing NULL and calling FunctionEnter2 verbatim
	FunctionTailCall2(pPrfCom, functionIDOrClientID.functionID, NULL, NULL);
}

VOID ShadowStackTest::FunctionEnter3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO /*eltInfo*/)
{
	//For Shadowstack testing in ELT, we ignore clientData, func and argumentinfo, passing NULL and calling FunctionEnter2 verbatim
	FunctionEnter2(pPrfCom, functionIDOrClientID.functionID, NULL, NULL, NULL);
}

VOID ShadowStackTest::FunctionLeave3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO /*eltInfo*/)
{
	//For Shadowstack testing in ELT, we ignore clientData, func and argumentinfo, passing NULL and calling FunctionEnter2 verbatim
	FunctionLeave2(pPrfCom,functionIDOrClientID.functionID, NULL, NULL, NULL);
}

VOID ShadowStackTest::FunctionTailCall3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO /*eltInfo*/)
{
	//For Shadowstack testing in ELT, we ignore all arguments except functionID. Calling FunctionTailCall2 verbatim since it contains the implementation of shadow stack
	FunctionTailCall2(pPrfCom, functionIDOrClientID.functionID, NULL, NULL);
}

VOID ShadowStackTest::FunctionEnter2(IPrfCom * pPrfCom,
                                FunctionID funcId,
                                UINT_PTR /* clientData */,
                                COR_PRF_FRAME_INFO /* func */,
                                COR_PRF_FUNCTION_ARGUMENT_INFO * /* argumentInfo */)
{
	ThreadID threadId;
    DERIVED_COMMON_POINTER(IJITCom);
	FunctionInfo *fi = pIJITCom->GetFunctionFromMap(funcId);
	if (NULL == fi)
	{
        DISPLAY(L"FunctionEnter2 for function that isn't known. " << HEX(funcId));

        if (pIJITCom->m_functionMap.begin() == pIJITCom->m_functionMap.end())
        {
            DISPLAY(L"\tNo known functions!");
        }
        else
        {
            for (FunctionMap::iterator iFM = pIJITCom->m_functionMap.begin(); 
                 iFM != pIJITCom->m_functionMap.end(); 
                 iFM++)
            {
                
                DISPLAY(L"\tKnown funcId: " << HEX(iFM->second->GetFunctionId()) << L" " << iFM->second->GetFunctionName());
            }
        }

        FAILURE(L"FunctionEnter2 for function that isn't known: " << HEX(funcId));
	}
	MUST_PASS(PINFO->GetCurrentThreadID(&threadId));
	ThreadInfo *ti = pIJITCom->GetThreadFromMap(threadId);
	if (NULL == ti)
	{
		FAILURE(L"FunctionEnter2: Unable to find ThreadInfo for thread id " << HEX(threadId));
	}
	ti->AddFunctionToStack(fi);
    return;
}

VOID ShadowStackTest::FunctionLeave2(IPrfCom * pPrfCom,
                                FunctionID mappedFuncId,
                                UINT_PTR /* clientData */,
                                COR_PRF_FRAME_INFO /* func */,
                                COR_PRF_FUNCTION_ARGUMENT_RANGE * /* retvalRange */)
{
    IJITCom * pJITCom = dynamic_cast<IJITCom *>(pPrfCom);
	FunctionInfo *fi = pJITCom->GetFunctionFromMap(mappedFuncId);
	if (NULL == fi)
	{
		FAILURE(L"FunctionLeave2 for function that isn't known");
	}
	ThreadID threadId;
	MUST_PASS(pPrfCom->m_pInfo->GetCurrentThreadID(&threadId));
	ThreadInfo *ti = pJITCom->GetThreadFromMap(threadId);
	if (NULL == ti)
	{
		FAILURE(L"FunctionEnter2: Unable to find ThreadInfo for thread id " << HEX(threadId));
	}
	if (FAILED(ti->PopFunctionFromStack(mappedFuncId)))
	{
		this->m_ulFailure++;
	}
    return;
}

VOID ShadowStackTest::FunctionTailCall2(IPrfCom * pPrfCom,
                                FunctionID mappedFuncId,
                                UINT_PTR /* clientData */,
                                COR_PRF_FRAME_INFO /* func */)
{
    IJITCom * pJITCom = dynamic_cast<IJITCom *>(pPrfCom);
	FunctionInfo *fi = pJITCom->GetFunctionFromMap(mappedFuncId);
	if (NULL == fi)
	{
		FAILURE(L"FunctionTailCall2 for function that isn't known");
	}
	ThreadID threadId;
	MUST_PASS(pPrfCom->m_pInfo->GetCurrentThreadID(&threadId));
	ThreadInfo *ti = pJITCom->GetThreadFromMap(threadId);
	if (NULL == ti)
	{
		FAILURE(L"FunctionEnter2: Unable to find ThreadInfo for thread id " << HEX(threadId));
	}
	if (FAILED(ti->PopFunctionFromStack(mappedFuncId)))
	{
		this->m_ulFailure++;
	}
    return;
}

HRESULT ShadowStackTest::ExceptionUnwindFunctionLeave(IPrfCom * pPrfCom)
{
    IJITCom * pJITCom = dynamic_cast<IJITCom *>(pPrfCom);
	ThreadID threadId;
	MUST_PASS(pPrfCom->m_pInfo->GetCurrentThreadID(&threadId));
	ThreadInfo *ti = pJITCom->GetThreadFromMap(threadId);
	if (NULL == ti)
	{
		FAILURE(L"FunctionEnter2: Unable to find ThreadInfo for thread id " << HEX(threadId));
	}
	if (FAILED(ti->PopFunctionFromStack(NULL)))
	{
		this->m_ulFailure++;
	}
	return S_OK;
}

//
//  Verify the results of this run.
//
HRESULT ShadowStackTest::Verify(IPrfCom *pPrfCom)
{
	IJITCom * pJITCom = dynamic_cast<IJITCom *>(pPrfCom);

	DISPLAY(L"ShadowStackTest Callback Verification\n");
	DISPLAY(L"m_JITCompilationStarted:           " << pPrfCom->m_JITCompilationStarted );
	DISPLAY(L"m_JITCompilationFinished:          " << pPrfCom->m_JITCompilationFinished );
	DISPLAY(L"m_JITCachedFunctionSearchStarted:  " << pPrfCom->m_JITCachedFunctionSearchStarted );
	DISPLAY(L"m_JITCachedFunctionSearchFinished: " << pPrfCom->m_JITCachedFunctionSearchFinished );
	DISPLAY(L"m_JITFunctionPitched:              " << pPrfCom->m_JITFunctionPitched );
	DISPLAY(L"m_JITInlining:                     " << pPrfCom->m_JITInlining );
	DISPLAY(L"m_FunctionEnter2:                  " << pPrfCom->m_FunctionEnter2 );
	DISPLAY(L"m_FunctionLeave2:                  " << pPrfCom->m_FunctionLeave2 );
	DISPLAY(L"m_FunctionTailcall2:               " << pPrfCom->m_FunctionTailcall2 );
	DISPLAY(L"m_FunctionEnter3:                  " << pPrfCom->m_FunctionEnter3 );
	DISPLAY(L"m_FunctionLeave3:                  " << pPrfCom->m_FunctionLeave3 );
	DISPLAY(L"m_FunctionTailCall3:               " << pPrfCom->m_FunctionTailCall3 );
	DISPLAY(L"m_FunctionEnter3WithInfo:          " << pPrfCom->m_FunctionEnter3WithInfo);
	DISPLAY(L"m_FunctionLeave3WithInfo:          " << pPrfCom->m_FunctionLeave3WithInfo);
	DISPLAY(L"m_FunctionTailCall3WithInfo:       " << pPrfCom->m_FunctionTailCall3WithInfo);
	DISPLAY(L"m_ExceptionUnwindFunctionEnter:    " << pPrfCom->m_ExceptionUnwindFunctionEnter );
	DISPLAY(L"m_ExceptionUnwindFunctionLeave:    " << pPrfCom->m_ExceptionUnwindFunctionLeave );
	DISPLAY(L"");

	if (IsWithinFudgeFactor<LONG>(pPrfCom->m_JITCompilationFinished, pPrfCom->m_JITCompilationStarted, 0.95))
	{
		m_ulSuccess++;
		DISPLAY(L"m_JITCompilationStarted == m_JITCompilationFinished");
	}
	else
	{
		m_ulFailure++;
		FAILURE(L"m_JITCompilationStarted != m_JITCompilationFinished");
		pJITCom->FunctionMapVerification();
	}

	if (IsWithinFudgeFactor<LONG>(pPrfCom->m_JITCachedFunctionSearchFinished, pPrfCom->m_JITCachedFunctionSearchStarted, 0.95))
	{
		m_ulSuccess++;
		DISPLAY(L"m_JITCachedFunctionSearchStarted == m_JITCachedFunctionSearchFinished");
	}
	else
	{
		m_ulFailure++;
		FAILURE(L"m_JITCachedFunctionSearchStarted != m_JITCachedFunctionSearchFinished");
	}

	if (pPrfCom->m_FunctionEnter2 > 0 || pPrfCom->m_FunctionEnter3 > 0 ||  pPrfCom->m_FunctionEnter3WithInfo > 0)
	{
		if(pPrfCom->m_FunctionEnter2 > 0)
		{
			if (IsWithinFudgeFactor<LONG>(pPrfCom->m_FunctionLeave2 + pPrfCom->m_FunctionTailcall2 + pPrfCom->m_ExceptionUnwindFunctionLeave, pPrfCom->m_FunctionEnter2, 0.9))
			{
				m_ulSuccess++;
				DISPLAY(L"m_FunctionEnter2 == m_FunctionLeave2 + m_FunctionTailcall2 + m_ExceptionUnwindFunctionLeave");
			}
			else
			{
				m_ulFailure++;
				FAILURE(L"m_FunctionEnter2 != m_FunctionLeave2 + m_FunctionTailcall2 + m_ExceptionUnwindFunctionLeave");
			}
		}
		if(pPrfCom->m_FunctionEnter3 > 0)
		{
			if (IsWithinFudgeFactor<LONG>(pPrfCom->m_FunctionLeave3 + pPrfCom->m_FunctionTailCall3 + pPrfCom->m_ExceptionUnwindFunctionLeave, pPrfCom->m_FunctionEnter3, 0.9))
			{
				m_ulSuccess++;
				DISPLAY(L"m_FunctionEnter3 == m_FunctionLeave3 + m_FunctionTailCall3 + m_ExceptionUnwindFunctionLeave");
			}
			else
			{
				m_ulFailure++;
				FAILURE(L"m_FunctionEnter3 != m_FunctionLeave3 + m_FunctionTailCall3 + m_ExceptionUnwindFunctionLeave");
			}
		}
		if(pPrfCom->m_FunctionEnter3WithInfo > 0)
		{
			if (IsWithinFudgeFactor<LONG>(pPrfCom->m_FunctionLeave3WithInfo + pPrfCom->m_FunctionTailCall3WithInfo + pPrfCom->m_ExceptionUnwindFunctionLeave, pPrfCom->m_FunctionEnter3WithInfo, 0.9))
			{
				m_ulSuccess++;
				DISPLAY(L"m_FunctionEnter3WithInfo == m_FunctionLeave3WithInfo + m_FunctionTailCall3WithInfo + m_ExceptionUnwindFunctionLeave");
			}
			else
			{
				m_ulFailure++;
				FAILURE(L"m_FunctionEnter3WithInfo != m_FunctionLeave3WithInfo + m_FunctionTailCall3WithInfo + m_ExceptionUnwindFunctionLeave");
			}
		}
	}
	else
	{
		FAILURE(L"m_FunctionEnter2 is not > 0 and m_FunctionEnter3 is not > 0 and m_FunctionEnter3WithInfo is not > 0");
	}
	DISPLAY(L"");
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


//
//  Verification routine called by TestProfiler
//
HRESULT ShadowStackTest_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(ShadowStackTest);
    HRESULT hr = pShadowStackTest->Verify(pPrfCom);

    // Clean up instance of test class
    delete pShadowStackTest;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}


//
//  Initialization routine called by TestProfiler
//
void ShadowStackTest_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, BOOL bEnableDoStackSnapshot, BOOL bEnableSlowEnterLeave, bool useELT3Callbacks)
{
    DISPLAY(L"Initialize ShadowStackTest.\n")

	// Create and save an instance of test class
    // pModuleMethodTable->TEST_POINTER is passed into every callback as pPrfCom->m_pTestClassInstance
	ShadowStackTest *sst = new ShadowStackTest(pPrfCom);
    pModuleMethodTable->TEST_POINTER = reinterpret_cast<void *>(sst);

	// Enable the Function ID Mapper
	//IJITCom * pJITCom = dynamic_cast<IJITCom *>(pPrfCom);
	//pJITCom->EnableFunctionMapper(pModuleMethodTable);

    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_THREADS
							  | COR_PRF_MONITOR_ENTERLEAVE
							  | COR_PRF_MONITOR_EXCEPTIONS
							  | COR_PRF_MONITOR_CACHE_SEARCHES
							  | COR_PRF_MONITOR_JIT_COMPILATION;

	sst->m_bEnableDoStackSnapshot = bEnableDoStackSnapshot;
	if (bEnableDoStackSnapshot)
	{
		pModuleMethodTable->FLAGS |= COR_PRF_ENABLE_STACK_SNAPSHOT;
	}

	sst->m_bEnableSlowEnterLeave = bEnableSlowEnterLeave;
	if (bEnableSlowEnterLeave)
	{
		pModuleMethodTable->FLAGS |= COR_PRF_ENABLE_FUNCTION_ARGS | COR_PRF_ENABLE_FUNCTION_RETVAL | COR_PRF_ENABLE_FRAME_INFO;
	}

	REGISTER_CALLBACK(THREADCREATED, ShadowStackTest::ThreadCreatedWrapper);
	REGISTER_CALLBACK(THREADDESTROYED, ShadowStackTest::ThreadDestroyedWrapper);
	REGISTER_CALLBACK(JITCOMPILATIONSTARTED, ShadowStackTest::JITCompilationStartedWrapper);
	REGISTER_CALLBACK(JITCOMPILATIONFINISHED, ShadowStackTest::JITCompilationFinishedWrapper);
	REGISTER_CALLBACK(JITCACHEDFUNCTIONSEARCHSTARTED, ShadowStackTest::JITCachedFunctionSearchStartedWrapper);
	REGISTER_CALLBACK(JITCACHEDFUNCTIONSEARCHFINISHED, ShadowStackTest::JITCachedFunctionSearchFinishedWrapper);
	REGISTER_CALLBACK(JITFUNCTIONPITCHED, ShadowStackTest::JITFunctionPitchedWrapper);
	if(!useELT3Callbacks)
	{
		REGISTER_CALLBACK(FUNCTIONENTER2, ShadowStackTest::FunctionEnter2Wrapper);
		REGISTER_CALLBACK(FUNCTIONLEAVE2, ShadowStackTest::FunctionLeave2Wrapper);
		REGISTER_CALLBACK(FUNCTIONTAILCALL2, ShadowStackTest::FunctionTailCall2Wrapper);
	}
	else
	{
		if(!bEnableSlowEnterLeave)
		{
			REGISTER_CALLBACK(FUNCTIONENTER3, ShadowStackTest::FunctionEnter3Wrapper);
			REGISTER_CALLBACK(FUNCTIONLEAVE3, ShadowStackTest::FunctionLeave3Wrapper);
			REGISTER_CALLBACK(FUNCTIONTAILCALL3, ShadowStackTest::FunctionTailCall3Wrapper);
		}
		else
		{
			REGISTER_CALLBACK(FUNCTIONENTER3WITHINFO, ShadowStackTest::FunctionEnter3WithInfoWrapper);
			REGISTER_CALLBACK(FUNCTIONLEAVE3WITHINFO, ShadowStackTest::FunctionLeave3WithInfoWrapper);
			REGISTER_CALLBACK(FUNCTIONTAILCALL3WITHINFO, ShadowStackTest::FunctionTailCall3WithInfoWrapper);
		}
	}
	REGISTER_CALLBACK(EXCEPTIONUNWINDFUNCTIONLEAVE, ShadowStackTest::ExceptionUnwindFunctionLeaveWrapper);
    REGISTER_CALLBACK(VERIFY, ShadowStackTest_Verify);

    return;
}
