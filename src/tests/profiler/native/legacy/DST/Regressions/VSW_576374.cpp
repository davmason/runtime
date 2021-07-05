#include "ProfilerCommon.h"
#include "LegacyCompat.h" // Whidbey Framework Support

/*******************************************************************************
 *
 *  This test serves as a regression case for
 *
 ******************************************************************************/


class VSW576374 // & VSW522030
{
public:

    VSW576374(IPrfCom * pPrfCom);

    HRESULT Verify(IPrfCom * pPrfCom);

    HRESULT FuncEnter2(
                    IPrfCom *pPrfCom,
                    FunctionID funcId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO frameInfo,
                    COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo);

    static HRESULT FuncEnter2Wrapper(
                    IPrfCom *pPrfCom,
                    FunctionID funcId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO frameInfo,
                    COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
    {
        return Instance()->FuncEnter2(pPrfCom, funcId, clientData, frameInfo, argumentInfo);
    }

    static VSW576374* Instance()
    {
        return This;
    }

private:

    static VSW576374* This;
};


/*
 *  ThreadFunc is used  to create the asynchronous ForceGC thread.
 */


VSW576374::VSW576374(IPrfCom * pPrfCom)
{
    VSW576374::This = this;
}


HRESULT VSW576374::FuncEnter2(	IPrfCom *pPrfCom,
                                FunctionID funcId,
                                UINT_PTR clientData,
                                COR_PRF_FRAME_INFO frameInfo,
                                COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
{
    return S_OK;
}



HRESULT VSW576374::Verify(IPrfCom * pPrfCom)
{
	DISPLAY( L"VSW_576374 Passed");
	return S_OK;
}


VSW576374* VSW576374::This = NULL;

VSW576374* global_VSW576374 = NULL;


HRESULT VSW576374_Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = VSW576374::Instance()->Verify(pPrfCom);

    delete global_VSW576374;

    return hr;
}


void VSW_576374_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize VSW576374 Extension ...\n" );

    global_VSW576374 = new VSW576374(pPrfCom);

    pModuleMethodTable->FLAGS =  COR_PRF_MONITOR_ENTERLEAVE  |
                                 COR_PRF_ENABLE_FUNCTION_ARGS  |
                                 COR_PRF_ENABLE_FUNCTION_RETVAL |
								 COR_PRF_USE_PROFILE_IMAGES;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &VSW576374_Verify;
	pModuleMethodTable->FUNCTIONENTER2 = (FC_FUNCTIONENTER2) &VSW576374::FuncEnter2Wrapper;;

    return;
}
