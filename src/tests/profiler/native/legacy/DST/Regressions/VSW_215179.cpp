#include "../../ProfilerCommon.h"
#include "../../LegacyCompat.h" // Whidbey Framework Support

class VSWhidbey_215179
{
    public:

    VSWhidbey_215179();
    ~VSWhidbey_215179();

    HRESULT ObjectAllocated(IPrfCom * pPrfCom,
                                ObjectID objectId,
                                ClassID classId);

    static HRESULT ObjectAllocatedWrapper(IPrfCom * pPrfCom,
                                ObjectID objectId,
                                ClassID classId)
    {
        return Instance()->ObjectAllocated(pPrfCom, objectId, classId);
    }

    HRESULT Verify(IPrfCom * pPrfCom);

    static VSWhidbey_215179* Instance()
    {
        return This;
    }

    static VSWhidbey_215179* This;

    ULONG success;
    ULONG failure;

    std::mutex m_criticalSection;
};

VSWhidbey_215179* VSWhidbey_215179::This = NULL;

VSWhidbey_215179::VSWhidbey_215179()
{
    // Static this pointer
    VSWhidbey_215179::This = this;

    success = 0;
    failure = 0;
}

VSWhidbey_215179::~VSWhidbey_215179()
{
    // Static this pointer
    VSWhidbey_215179::This = NULL;
}

HRESULT VSWhidbey_215179::ObjectAllocated(IPrfCom * pPrfCom,
                                                ObjectID objectId,
                                                ClassID classId)
{
    HRESULT hr = S_OK;

    lock_guard<recursive_mutex> guard(pPrfCom->m_criticalSection);

    CorElementType cet = (CorElementType)NULL;
    ClassID baseClassId = NULL;
    ULONG rank = NULL;

    hr = PINFO->GetClassFromObject(objectId,
                                &baseClassId);
    if ((FAILED(hr)) || (baseClassId != classId))
    {
        FAILURE(L"GetClassFromObject returned HR=0x" << HEX(hr) << L", classID=0x" << HEX(baseClassId) << L", ObjectAllocated ClassID=0x," << HEX(classId) << L"\n");
    }


    hr = PINFO->IsArrayClass(classId,
                                &cet,
                                &baseClassId,
                                &rank);
    if (FAILED(hr))
    {
        FAILURE(L"ICorProfilerInfo::IsArrayClass return HR=0x" << HEX(hr) << " for ClassID=0x" << HEX(classId) << L"\n");
        failure++;
    }
    else if (hr == S_OK)
    {
        ULONG32 *dimensionSizes = (ULONG32 *)_alloca(sizeof(ULONG32) * rank);
        INT *dimensionLowerBounds = (int *)_alloca(sizeof(int) * rank);
        BYTE *pData = NULL;

        hr = PINFO->GetArrayObjectInfo(objectId,
                                rank,
                                dimensionSizes,
                                dimensionLowerBounds,
                                &pData);
        if(FAILED(hr))
        {
            FAILURE(L"ICorProfilerInfo2::GetArrayObjectInfo return HR=0x" << HEX(hr) << " for ObjectID=0x" << HEX(objectId) << L"\n");
            failure++;
        }
        else
        {
             for(ULONG i = 0; i < rank; i++)
            {
                if (dimensionSizes[i] == 0)
                {
                    DISPLAY(L"pDimensionSizes returned from GetArrayObjectInfo is 0\n");
                    failure++;
                }
                else
                {
                    // Success.  We have into for the dimension.
                    success++;
                }
            }
        }

    }

    return hr;
}

HRESULT VSWhidbey_215179::Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;

    DISPLAY( L"Verify VSWhidbey 215179\n" );

    if (success < 1)
    {
        FAILURE(L"_FAILURE_OLD_.  Success " << success << L", Failure %d" << failure << L"\n");
        hr = E_FAIL;
    }
    else
    {
        DISPLAY(L"SUCCESS.  Successful array inspections " << success << L"\n");
    }

    return hr;
}


/*
 *  Straight function callback used by TestProfiler
 */

VSWhidbey_215179* global_VSWhidbey_215179;

HRESULT VSWhidbey_215179_Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = VSWhidbey_215179::Instance()->Verify(pPrfCom);

    delete global_VSWhidbey_215179;

    return hr;
}

void VSW_215179_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY( L"Initialize 215179 extension\n" );

    global_VSWhidbey_215179 = new VSWhidbey_215179();

    pModuleMethodTable->FLAGS = COR_PRF_ENABLE_OBJECT_ALLOCATED |
                                                    COR_PRF_MONITOR_OBJECT_ALLOCATED;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &VSWhidbey_215179_Verify;
    pModuleMethodTable->OBJECTALLOCATED= (FC_OBJECTALLOCATED) &VSWhidbey_215179::ObjectAllocatedWrapper;

    return;
}

