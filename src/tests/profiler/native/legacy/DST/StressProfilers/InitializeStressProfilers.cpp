#include "ProfilerCommon.h"
extern PMODULEMETHODTABLE pCallTable; 
extern IPrfCom * g_satellite_pPrfCom;

void AllCallbacks_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void CodeProfiler_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void SamplingProfiler_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);

void Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
    if (testName == L"AllCallbacks") AllCallbacks_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"CodeProfiler") CodeProfiler_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"SamplingProfiler") SamplingProfiler_Initialize(pPrfCom, pModuleMethodTable);
    else FAILURE(L"Test name not recognized!");

	pCallTable = pModuleMethodTable; 
	g_satellite_pPrfCom = pPrfCom;

	return;
}
extern "C" BOOL WINAPI DllMain( HINSTANCE /*hInstance*/, DWORD dwReason, LPVOID lpReserved )
{
	
	if ( dwReason == DLL_PROCESS_DETACH && g_satellite_pPrfCom->m_Shutdown ==0 )
	{
		HRESULT hr = S_OK;
		IPrfCom * pPrfCom = g_satellite_pPrfCom;
		InterlockedIncrement(&PPRFCOM->m_Shutdown);
        if (pCallTable->VERIFY(PPRFCOM) != S_OK)
        {
            DISPLAY(L"");
            DISPLAY(L"TEST FAILED");
            hr = E_FAIL;
        }
        else if (PPRFCOM->m_ulError > 0) 
        {
            DISPLAY(L"");
            DISPLAY(L"TEST FAILED");
            DISPLAY(L"Search \"ERROR: Test Failure\" in the log file for details.");
            hr = E_FAIL;
        }
        else
        {
            DISPLAY(L"");
            DISPLAY(L"TEST PASSED");
        }    

	}
	return TRUE;
} // DllMain
