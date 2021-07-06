#ifndef __INSPECT_ARG_AND_RET_VAL__
#define __INSPECT_ARG_AND_RET_VAL__

#ifdef __clang__
#pragma clang diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif // __clang__ 

#include "LTSCommon.h"
#include "../../sigparse.h"

#include <map>

#define PARSE_CALLBACK_VARIABLES IPrfCom * pPrfCom = m_pPrfCom; ILTSCom * pILTSCom = dynamic_cast<ILTSCom *>(m_pPrfCom);

typedef map<mdModule, ModuleID> ModuleDefToIDMap;
typedef map<wstring, INT> FieldValMap;

typedef enum InspectionTestFlags
{
    // Test Flags
    TEST_NONE                           = 0x00000000,
    UNIT_TEST_RETVAL_INT                = 0x00000001,
    UNIT_TEST_RETVAL_CLASS              = 0x00000002,
    UNIT_TEST_RETVAL_CLASS_STATIC       = 0x00000004,
    UNIT_TEST_RETVAL_STRUCT             = 0x00000008,
    UNIT_TEST_RETVAL_STRUCT_STATIC      = 0x00000010,
    UNIT_TEST_RETVAL_COMPLEX_STATIC     = 0x00000020,
    UNIT_TEST_RETVAL_GENERIC_CLASS_INT  = 0x00000040,
    UNIT_TEST_RETVAL_GENERIC_METHOD_INT = 0x00000080,
    UNIT_TEST_ARG_INT                   = 0x00000100,
    UNIT_TEST_ARG_CLASS                 = 0x00000200,
    UNIT_TEST_ARG_CLASS_STATIC          = 0x00000400,
    UNIT_TEST_ARG_STRUCT                = 0x00000800,
    UNIT_TEST_ARG_STRUCT_STATIC         = 0x00001000,
    UNIT_TEST_ARG_COMPLEX_STATIC        = 0x00002000,
    UNIT_TEST_ARG_GENERIC_CLASS_INT     = 0x00004000,
    UNIT_TEST_ARG_GENERIC_METHOD_INT    = 0x00008000,
    UNIT_TEST_ATTACH                    = 0x00010000,

    // Composite Flags
    TEST_WITH_CLASS                     = UNIT_TEST_RETVAL_CLASS                 | 
                                            UNIT_TEST_RETVAL_CLASS_STATIC        |
                                            UNIT_TEST_ARG_CLASS                  |
                                            UNIT_TEST_ARG_CLASS_STATIC           |
                                            UNIT_TEST_ARG_COMPLEX_STATIC         |
                                            UNIT_TEST_RETVAL_COMPLEX_STATIC,
    TEST_WITH_STRUCT                    = UNIT_TEST_RETVAL_STRUCT                | 
                                            UNIT_TEST_RETVAL_STRUCT_STATIC       | 
                                            UNIT_TEST_ARG_STRUCT                 | 
                                            UNIT_TEST_ARG_STRUCT_STATIC,
    TEST_ARGS                           = UNIT_TEST_ARG_INT                      |  
                                             UNIT_TEST_ARG_CLASS                 |
                                             UNIT_TEST_ARG_CLASS_STATIC          |
                                             UNIT_TEST_ARG_STRUCT                |
                                             UNIT_TEST_ARG_STRUCT_STATIC         |
                                             UNIT_TEST_ARG_COMPLEX_STATIC        |
                                             UNIT_TEST_ARG_GENERIC_CLASS_INT     |
                                             UNIT_TEST_ARG_GENERIC_METHOD_INT,
    TEST_RETVAL                         = UNIT_TEST_RETVAL_INT                   |
                                             UNIT_TEST_RETVAL_CLASS              |
                                             UNIT_TEST_RETVAL_CLASS_STATIC       |
                                             UNIT_TEST_RETVAL_STRUCT             |
                                             UNIT_TEST_RETVAL_STRUCT_STATIC      |
                                             UNIT_TEST_RETVAL_COMPLEX_STATIC     |
                                             UNIT_TEST_RETVAL_GENERIC_CLASS_INT  |
                                             UNIT_TEST_RETVAL_GENERIC_METHOD_INT,
} InspectionTestFlags;

// helper class for by-ref arguments.  We deref the pointer in the
// range address and we restore it when our dtor runs.
class DerefedRangeHolder
{
public:
    DerefedRangeHolder(COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo, ULONG currentRange)
    {
        m_pArgInfo = argumentInfo;
        m_ulCurrentRange = currentRange;
        m_oldValue = argumentInfo->ranges[currentRange].startAddress;
        m_fBadPointer = false;

        // now deref the arg for the current arg info.
#ifdef WIN32
        if (IsBadReadPtr((PVOID)(m_oldValue), sizeof(ObjectID)) != 0)
        {
            m_fBadPointer = true;
        }
#endif

        ObjectID pointsTo = *(ObjectID*)m_oldValue;
#ifdef WIN32
        if (IsBadReadPtr((PVOID)(pointsTo), sizeof(ObjectID)) != 0)
        {
            m_fBadPointer = true;
        }
#endif

        argumentInfo->ranges[currentRange].startAddress = pointsTo;
    }

    bool IsBadDerefPointer()
    {
        return(m_fBadPointer);
    }

    ~DerefedRangeHolder()
    {
        m_pArgInfo->ranges[m_ulCurrentRange].startAddress = m_oldValue;
    }

private:
    COR_PRF_FUNCTION_ARGUMENT_INFO *m_pArgInfo;
    ULONG m_ulCurrentRange;
    ObjectID m_oldValue;
    bool m_fBadPointer;
};


class InspectArgAndRetVal : protected SigParser
{
public:

    InspectArgAndRetVal(IPrfCom * pPrfCom, InspectionTestFlags testFlags);
    ~InspectArgAndRetVal();
    HRESULT Verify(IPrfCom * pPrfCom);

    HRESULT AttachThreadFunc();
    DWORD m_dwCorPrfFlags;

    //--------------------------------------------------------------------------------

#pragma region Static Wrappers

    static HRESULT FuncEnter2Wrapper(IPrfCom * pPrfCom, FunctionID funcId, UINT_PTR clientData, COR_PRF_FRAME_INFO func, COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
    {
        return STATIC_CLASS_CALL(InspectArgAndRetVal)->FuncEnter2(pPrfCom, funcId, clientData, func, argumentInfo);
    }

    static HRESULT FunctionEnter3WithInfoWrapper(IPrfCom * pPrfCom, FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)
    {
        return STATIC_CLASS_CALL(InspectArgAndRetVal)->FunctionEnter3WithInfo(pPrfCom, functionIdOrClientID, eltInfo);
    }

    static HRESULT FuncLeave2Wrapper(IPrfCom * pPrfCom, FunctionID mappedFuncId, UINT_PTR clientData, COR_PRF_FRAME_INFO func, COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange)
    {
        return STATIC_CLASS_CALL(InspectArgAndRetVal)->FuncLeave2(pPrfCom, mappedFuncId, clientData, func, retvalRange);
    }

    static HRESULT FunctionLeave3WithInfoWrapper(IPrfCom * pPrfCom, FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)
    {
        return STATIC_CLASS_CALL(InspectArgAndRetVal)->FunctionLeave3WithInfo(pPrfCom, functionIdOrClientID, eltInfo);
    }

    static HRESULT ModuleLoadFinishedWrapper(IPrfCom * pPrfCom, ModuleID moduleID, HRESULT hrStatus)
    {
        return STATIC_CLASS_CALL(InspectArgAndRetVal)->ModuleLoadFinished(pPrfCom, moduleID, hrStatus);
    }

#pragma endregion     // Static Wrappers

private:

    HRESULT VerifyClass(IPrfCom * pPrfCom,
                    mdToken token,
                    FieldValMap& varList);

    HRESULT VerifyStruct(IPrfCom * pPrfCom,
                    mdToken token,
                    FieldValMap& varList);

    ObjectID GetObjectID();

    HRESULT InspectObjectFields(IPrfCom * pPrfCom,
                    mdToken token,
                    ModuleID moduleId,
                    ClassID classId,
                    ObjectID objectId,
                    FieldValMap& varList);

    HRESULT VerifyFieldValue(IPrfCom * pPrfCom,
                    const PLATFORM_WCHAR * fieldName,
                    INT fieldVal,
                    INT expectedVal);

    //--------------------------------------------------------------------------------

#pragma region Callback Methods

    HRESULT FuncEnter2(IPrfCom * pPrfCom,
                    FunctionID funcId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO func,
                    COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo);

    HRESULT FuncLeave2(IPrfCom * pPrfCom,
                    FunctionID mappedFuncId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO func,
                    COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange);

    HRESULT FunctionEnter3WithInfo(IPrfCom * pPrfCom,
                    FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo);


    HRESULT FunctionLeave3WithInfo(IPrfCom * pPrfCom,
                    FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo);

    HRESULT ModuleLoadFinished(IPrfCom * pPrfCom, 
                    ModuleID moduleID, 
                    HRESULT hrStatus);

#pragma endregion    // Callback Methods

    //--------------------------------------------------------------------------------

#pragma region Class Data

    IPrfCom *m_pPrfCom;

    FunctionID m_functionID;
    COR_PRF_FRAME_INFO m_func;
    COR_PRF_FUNCTION_ARGUMENT_INFO  * m_pArgumentInfo;
    COR_PRF_FUNCTION_ARGUMENT_RANGE * m_pRetvalRange;

    std::mutex m_csParse;

    BOOL m_bMSCorLibFound;
    mdTypeDef corTypeDefTokens[ELEMENT_TYPE_STRING + 1];

    InspectionTestFlags m_testFlags;

    UINT m_retValIntSuccess;
    UINT m_retValClassSuccess;
    UINT m_retValClassStaticSuccess;
    UINT m_retValStructSuccess;
    UINT m_retValStructStaticSuccess;
    UINT m_retValComplextStaticSuccess;
    UINT m_retValGenericClassIntSuccess;
    UINT m_retValGenericMethodIntSuccess;
    UINT m_argIntSuccess;
    UINT m_argClassSuccess;
    UINT m_argClassStaticSuccess;
    UINT m_argStructSuccess;
    UINT m_argStructStaticSuccess;
    UINT m_argComplexStaticSuccess;
    UINT m_argGenericClassIntSuccess;
    UINT m_argGenericMethodIntSuccess;
    UINT m_allSuccess;

    ULONG m_ulInspectedThisPointers;
    ULONG m_ulCurrentParam;

    ModuleDefToIDMap m_defToIdMap;

    mdTypeDef systemObjectTypeDef;
    mdTypeDef systemValueTypeTypeDef;

	ModuleID mscorelibMduelId;

    BOOL m_bInFunctionEnter;
    BOOL m_bInFunctionLeave;
    BOOL m_bIsValueType;
    BOOL m_bIsClassType;
    BOOL m_bBeginMethod;
    BOOL m_bBeginParams;
    BOOL m_bBeginRetType;

#pragma endregion          // Class Data

    //--------------------------------------------------------------------------------

#pragma region SigParse

    // a method with given elem_type
    virtual void NotifyBeginMethod(sig_elem_type elem_type) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyBeginMethod " << HEX(elem_type)); m_bBeginMethod = TRUE; m_ulCurrentParam = 0;}
    virtual void NotifyEndMethod() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"NotifyEndMethod"); m_bBeginMethod = FALSE; }

    // the method has a this pointer
    virtual void NotifyHasThis();

    // total parameters for the method
    virtual void NotifyParamCount(sig_count count);

    // starting a return type
    virtual void NotifyBeginRetType() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"NotifyBeginRetType"); m_bBeginRetType = TRUE; }
    virtual void NotifyEndRetType()   { PARSE_CALLBACK_VARIABLES; DISPLAY(L"NotifyEndRetType"); m_bBeginRetType = FALSE; }

    // starting a parameter
    virtual void NotifyBeginParam() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"NotifyBeginParam"); m_bBeginParams = TRUE; }
    virtual void NotifyEndParam()   { PARSE_CALLBACK_VARIABLES; DISPLAY(L"NotifyEndParam"); m_bBeginParams = FALSE; }

    // sentinel indication the location of the "..." in the method signature
    virtual void NotifySentinal() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifySentinal"); }

    // number of generic parameters in this method signature (if any)
    virtual void NotifyGenericParamCount(sig_count count);

    //----------------------------------------------------

    // a field with given elem_type
    virtual void NotifyBeginField(sig_elem_type elem_type) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyBeginField " << HEX(elem_type)); }
    virtual void NotifyEndField() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyEndField"); }

    //----------------------------------------------------

    // a block of locals with given elem_type (always just LOCAL_SIG for now)
    virtual void NotifyBeginLocals(sig_elem_type elem_type) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyBeginLocals " << HEX(elem_type)); }
    virtual void NotifyEndLocals() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyEndLocals"); }

    // count of locals with a block
    virtual void NotifyLocalsCount(sig_count count) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyLocalsCount " << HEX(count)); }

    // starting a new local within a local block
    virtual void NotifyBeginLocal() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyBeginLocal"); }
    virtual void NotifyEndLocal() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyEndLocal"); }

    // the only constraint available to locals at the moment is ELEMENT_TYPE_PINNED
    virtual void NotifyConstraint(sig_elem_type elem_type) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyConstraint " << HEX(elem_type)); }


    //----------------------------------------------------

    // a property with given element type
    virtual void NotifyBeginProperty(sig_elem_type elem_type) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyBeginProperty " << HEX(elem_type)); }
    virtual void NotifyEndProperty() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyEndProperty"); }

    //----------------------------------------------------

    // starting array shape information for array types
    virtual void NotifyBeginArrayShape() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyBeginArrayShape"); }
    virtual void NotifyEndArrayShape() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyEndArrayShape"); }

    // array rank (total number of dimensions)
    virtual void NotifyRank(sig_count count) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyRank " << HEX(count)); }

    // number of dimensions with specified sizes followed by the size of each
    virtual void NotifyNumSizes(sig_count count) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyNumSizes " << HEX(count)); }
    virtual void NotifySize(sig_count count) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifySize " << HEX(count)); }

    // BUG BUG lower bounds can be negative, how can this be encoded?
    // number of dimensions with specified lower bounds followed by lower bound of each 
    virtual void NotifyNumLoBounds(sig_count count) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyNumLoBounds " << HEX(count)); }  
    virtual void NotifyLoBound(sig_count count) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyLoBound " << HEX(count)); }

    //----------------------------------------------------


    // starting a normal type (occurs in many contexts such as param, field, local, etc)
    virtual void NotifyBeginType() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"NotifyBeginType"); }
    virtual void NotifyEndType() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"NotifyEndType"); }
    
    virtual void NotifyTypedByref() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyTypedByref"); }

    // the type has the 'byref' modifier on it -- this normally proceeds the type definition in the context
    // the type is used, so for instance a parameter might have the byref modifier on it
    // so this happens before the BeginType in that context
    virtual void NotifyByref() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyByref"); }

    // the type is "VOID" (this has limited uses, function returns and void pointer)
    virtual void NotifyVoid() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyVoid"); }

    // the type has the indicated custom modifiers (which can be optional or required)
    virtual void NotifyCustomMod(sig_elem_type cmod, sig_index_type indexType, sig_index index) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyCustomMod " << HEX(cmod) << L" " << HEX(indexType) << L" " << HEX(index)); }

    // the type is a simple type, the elem_type defines it fully
    virtual void NotifyTypeSimple(sig_elem_type  elem_type);

    // the type is specified by the given index of the given index type (normally a type index in the type metadata)
    // this callback is normally qualified by other ones such as NotifyTypeClass or NotifyTypeValueType
    virtual void NotifyTypeDefOrRef(sig_index_type  indexType, int index);

    // the type is an instance of a generic
    // elem_type indicates value_type or class
    // indexType and index indicate the metadata for the type in question
    // number indicates the number of type specifications for the generic types that will follow
    virtual void NotifyTypeGenericInst(sig_elem_type elem_type, sig_index_type indexType, sig_index index, sig_mem_number number) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyTypeGenericInst " << HEX(elem_type) << L" " << HEX(indexType) << L" " << HEX(index) << L" " << HEX(number)); }

    // the type is the type of the nth generic type parameter for the class
    virtual void NotifyTypeGenericTypeVariable(sig_mem_number number);

    // the type is the type of the nth generic type parameter for the member
    virtual void NotifyTypeGenericMemberVariable(sig_mem_number number) { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyTypeGenericMemberVariable " << HEX(number)); } 

    // the type will be a value type
    virtual void NotifyTypeValueType() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"NotifyTypeValueType"); m_bIsValueType = TRUE; m_bIsClassType = FALSE; }

    // the type will be a class
    virtual void NotifyTypeClass() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"NotifyTypeClass"); m_bIsValueType = FALSE; m_bIsClassType = TRUE; }

    // the type is a pointer to a type (nested type notifications follow)
    virtual void NotifyTypePointer() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyTypePointer"); }

    // the type is a function pointer, followed by the type of the function
    virtual void NotifyTypeFunctionPointer() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyTypeFunctionPointer"); }

    // the type is an array, this is followed by the array shape, see above, as well as modifiers and element type
    virtual void NotifyTypeArray() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyTypeArray"); }
    
    // the type is a simple zero-based array, this has no shape but does have custom modifiers and element type
    virtual void NotifyTypeSzArray() { PARSE_CALLBACK_VARIABLES; DISPLAY(L"\tNotifyTypeSzArray"); }

    //--------------------------------------------------------------------------------

#pragma endregion            // SigParse
};

#endif // __INSPECT_ARG_AND_RET_VAL__
