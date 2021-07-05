




























STRESS_HEAP TEXTEQU <>

STRESS_THREAD TEXTEQU <>




VERIFY_HEAP TEXTEQU <>




TEST_DATA_CONSISTENCY TEXTEQU <>



STRESS_LOG TEXTEQU <>



LOP_FRIENDLY_FRAMES TEXTEQU <>
























ML_MARSHALING TEXTEQU <>









INTEROP_FRAMES TEXTEQU <>



























FEATURE_PREJIT_INDIRECTIONS EQU 1t



FEATURE_SHARE_GENERIC_CODE TEXTEQU <>













WIN64EXCEPTIONS TEXTEQU <>





































EMIT_FIXUPS TEXTEQU <>





USE_INDIRECT_GET_THREAD_APPDOMAIN EQU 0t

















































PAGE_SIZE EQU 01000h
    
USE_UPPER_ADDRESS EQU 1t
    
UPPER_ADDRESS_MAPPING_FACTOR EQU 2t
    
CLR_UPPER_ADDRESS_MIN EQU 064200000000h
    
CODEHEAP_START_ADDRESS EQU 064280000000h
    
CLR_UPPER_ADDRESS_MAX EQU 0642FC000000h
    

CODEHEAP_START_ADDRESS_ASLR EQU 07FF00000000h
    






OS_PAGE_SIZE TEXTEQU <0x1000>




JIT_IS_ALIGNED TEXTEQU <>





































PLATFORM_SUPPORTS_SAFE_THREADSUSPEND TEXTEQU <>








FEATURE_HIJACK TEXTEQU <>




















ALLOW_LOCAL_WORKER TEXTEQU <>














FEATURE_PROFAPI_EVENT_LOGGING TEXTEQU <>




FEATURE_PROFAPI_ATTACH_DETACH TEXTEQU <>



























ASM_ELEMENT_TYPE_R4 EQU 0Ch

;

ASM_ELEMENT_TYPE_R8 EQU 0Dh

;

ASM_CLRTASKHOSTED EQU 02h

;

METHODDESC_REGNUM EQU 10t

METHODDESC_REGISTER TEXTEQU <r10>


PINVOKE_CALLI_TARGET_REGNUM EQU 10t

PINVOKE_CALLI_TARGET_REGISTER TEXTEQU <r10>


PINVOKE_CALLI_SIGTOKEN_REGNUM EQU 11t

PINVOKE_CALLI_SIGTOKEN_REGISTER TEXTEQU <r11>



SIZEOF_MAX_OUTGOING_ARGUMENT_HOMES EQU 020h



SIZEOF_MAX_FP_ARG_SPILL EQU 040h


SIZEOF_GSCookie EQU 08h

;

OFFSETOF__Frame____VFN_table EQU 0t


OFFSETOF__Frame__m_Next EQU 08h

;

SIZEOF__Frame EQU 010h


OFFSETOF__TransitionFrame__m_rsp EQU 010h

;

OFFSETOF__TransitionFrame__m_rdi EQU 018h

;

OFFSETOF__TransitionFrame__m_rsi EQU 020h

;

OFFSETOF__TransitionFrame__m_rbx EQU 028h

;

OFFSETOF__TransitionFrame__m_rbp EQU 030h

;

OFFSETOF__TransitionFrame__m_r12 EQU 038h

;

OFFSETOF__TransitionFrame__m_r13 EQU 040h

;

OFFSETOF__TransitionFrame__m_r14 EQU 048h

;

OFFSETOF__TransitionFrame__m_r15 EQU 050h

;

OFFSETOF__TransitionFrame__m_Datum EQU 058h

;

OFFSETOF__TransitionFrame__m_ReturnAddress EQU 060h

;

SIZEOF__TransitionFrame EQU 068h

;


SIZEOF__TPMethodFrame EQU 068h

;


SIZEOF__PrestubMethodFrame EQU 068h

;

SIZEOF__StubDispatchFrame EQU 068h

;


ALIGNED_SIZEOF__StubDispatchFrame EQU 068h

;
;



SIZEOF__ComPrestubMethodFrame EQU 020h

;



SIZEOF__CleanupWorkList EQU 08h

;

OFFSETOF__NDirectMethodFrame__cbStackArguments EQU -010h

;

OFFSETOF__NDirectMethodFrame__fCalleeSavedRegsUpdated EQU -005h

;

LeaveRuntimeFlags__FLAG_ENTER_RUNTIME_REQUIRED EQU 1t

;

SIZEOF__NDirectMethodFrame__NegInfo EQU 010h

;

OFFSETOF__NDirectMethodFrame__NegInfo__m_flags EQU 0ch

;

OFFSETOF__UnmanagedToManagedFrame__m_pvDatum EQU 010h

;

SIZEOF__UnmanagedToManagedCallFrame__RawNegInfo EQU 028h

;



SIZEOF__ComMethodFrame EQU 020h

;


OFFSETOF__UMThkCallFrame__NegInfo__m_List EQU 0t

;

OFFSETOF__UMThkCallFrame__NegInfo__m_pReturnContext EQU 008h

;

OFFSETOF__UMThkCallFrame__NegInfo__m_pArgs EQU 010h

;

OFFSETOF__UMThkCallFrame__NegInfo__m_GCInfo EQU 018h

;

SIZEOF__UMThkCallFrame EQU 020h

;

OFFSETOF__UMEntryThunk__m_pUMThunkMarshInfo EQU 018h

;

OFFSETOF__UMEntryThunk__m_dwDomainId EQU 020h

;

OFFSETOF__UMThunkMarshInfo__m_pMLStub EQU 028h

;

OFFSETOF__UMThunkMarshInfo__m_pExecStub EQU 030h

;

OFFSETOF__UMThunkMarshInfo__m_cbActualArgSize EQU 03Ch

;

OFFSETOF__Stub__m_pCode EQU 010h

;



SIZEOF__ComPlusMethodFrameGeneric EQU 068h

;

ComPlusMethodFrameGeneric__GetRawNegSpaceSize EQU 020h

;

SIZEOF__ComPlusMethodFrame__NegInfo EQU 010h

;

OFFSETOF__ComPlusMethodFrame__NegInfo__cbArgs_FPRet EQU 00h

;

OFFSETOF__ComPlusMethodFrame__NegInfo__m_flags EQU 08h

;

OFFSETOF__ComPlusCallMethodDesc__compluscall_m_pMLStub EQU 008h

;

OFFSETOF__ComPlusCallMethodDesc__compluscall_m_pInterfaceMT EQU 010h

;

OFFSETOF__ComPlusCallMethodDesc__compluscall_m_cachedComSlot EQU 028h

;


OFFSETOF__Thread__m_State EQU 08h

;

OFFSETOF__Thread__m_fPreemptiveGCDisabled EQU 00Ch

;

OFFSETOF__Thread__m_pFrame EQU 010h

;

OFFSETOF__Thread__m_dwLockCount EQU 028h

;

OFFSETOF__Thread__m_ThreadId EQU 02Ch

;


OFFSETOF__Thread__m_pHostTask EQU 030h

;











OFFSETOF__Thread__m_OSContext EQU 0138h



;














OFFSETOF__Thread__m_ExceptionState EQU 0258h




;















OFFSETOF__Thread__m_pDomain EQU 0378h




;


OFFSET__Thread__m_alloc_context__alloc_ptr EQU 070h





;


OFFSET__Thread__m_alloc_context__alloc_limit EQU 078h





;

OFFSETOF__ThreadExceptionState__m_pCurrentTracker EQU 0000h

;


TP_OFFSET_STUBDATA EQU 010h

;

TP_OFFSET_MT EQU 018h

;

TP_OFFSET_STUB EQU 028h

;


OFFSETOF__NDirectMethodDesc__m_pWriteableData EQU 020h

;

OFFSETOF__ObjHeader__SyncBlkIndex EQU 04h

;

SIZEOF__SyncTableEntry EQU 010h

;;

OFFSETOF__SyncTableEntry__m_SyncBlock EQU 00h

;;

OFFSETOF__SyncBlock__m_Monitor EQU 00h

;;



OFFSETOF__SyncBlock__m_pInteropInfo EQU 028h

;;

OFFSETOF__InteropSyncBlockInfo__m_pRCW EQU 018h

;;

OFFSETOF__TEB__ReservedForOle EQU 01758h

;;

OFFSETOF__RCW__m_UnkEntry EQU 08h

;;

OFFSETOF__IUnkEntry__m_pCtxCookie EQU 00h

;;

OFFSETOF__RCW__m_aInterfaceEntries EQU 038h

;;

SIZEOF__InterfaceEntry EQU 010h

;;

OFFSETOF__InterfaceEntry__m_pMT EQU 00h

;;

OFFSETOF__InterfaceEntry__m_pUnknown EQU 08h

;;





OFFSETOF__SOleTlsData__pCurrentCtx EQU 060h

;;



OFFSETOF__DelegateObject___methodPtr EQU 018h

;;

OFFSETOF__DelegateObject___target EQU 008h

;;

OFFSETOF__AwareLock__m_MonitorHeld EQU 00h

;

OFFSETOF__AwareLock__m_Recursion EQU 04h

;

OFFSETOF__AwareLock__m_HoldingThread EQU 08h

;

OFFSETOF__g_SystemInfo__dwNumberOfProcessors EQU 020h

;

OFFSETOF__MethodTable__m_wFlags EQU 000h

;

OFFSET__MethodTable__m_BaseSize EQU 004h

;

OFFSETOF__MethodTable__m_wNumVtableSlots EQU 00Ah

;
    
OFFSETOF__MethodTable__m_wNumInterfaces EQU 00Eh

;

OFFSETOF__MethodTable__m_pParentMethodTable EQU 010h

;

OFFSETOF__MethodTable__m_pWriteableData EQU 020h

;

OFFSETOF__MethodTable__m_pEEClass EQU 028h

;

SIZEOF__MethodTable EQU 040h

;

METHODTABLE_OFFSET_VTABLE EQU 040h

;


METHODTABLE_PROXY_FLAGS EQU 001000000h

;



METHODTABLE_NONTRIVIALINTERFACECAST_FLAGS EQU 041010000h












;

MethodTable__enum_flag_ContainsPointers EQU 000080000h

;

OFFSETOF__MethodTableWriteableData__m_dwFlags EQU 0t

;

MethodTableWriteableData__enum_flag_Unrestored EQU 004h

;

OFFSETOF__InterfaceInfo_t__m_pMethodTable EQU 0t

;

SIZEOF__InterfaceInfo_t EQU 010h

;







OFFSETOF__AppDomain__m_dwId EQU 078ch

OFFSETOF__AppDomain__m_sDomainLocalBlock EQU 0798h



;


;

OFFSETOF__DomainLocalBlock__m_pModuleSlots EQU 08h

;

OFFSETOF__DomainLocalModule__m_pDataBlob EQU 0028h

;


;





OFFSETOF__DomainLocalModule__m_pGCStatics EQU 0020h

;



CORINFO_NullReferenceException_ASM EQU 0t

;

CORINFO_InvalidCastException_ASM EQU 2t

;

CORINFO_IndexOutOfRangeException_ASM EQU 3t

;

CORINFO_SynchronizationLockException_ASM EQU 5t

;

CORINFO_ArrayTypeMismatchException_ASM EQU 6t

;

CORINFO_ArgumentNullException_ASM EQU 8t

;
                      
CORINFO_ArgumentException_ASM EQU 9t

;




OFFSETOF__MachState__m_Rip EQU 000h

;

OFFSETOF__MachState__m_Rsp EQU 008h

;

OFFSETOF__MachState__m_CaptureRdi EQU 010h

;

OFFSETOF__MachState__m_CaptureRsi EQU 018h

;

OFFSETOF__MachState__m_CaptureRbx EQU 020h

;

OFFSETOF__MachState__m_CaptureRbp EQU 028h

;

OFFSETOF__MachState__m_CaptureR12 EQU 030h

;

OFFSETOF__MachState__m_CaptureR13 EQU 038h

;

OFFSETOF__MachState__m_CaptureR14 EQU 040h

;

OFFSETOF__MachState__m_CaptureR15 EQU 048h

;

OFFSETOF__MachState__m_pRdi EQU 050h

;

OFFSETOF__MachState__m_pRsi EQU 058h

;

OFFSETOF__MachState__m_pRbx EQU 060h

;

OFFSETOF__MachState__m_pRbp EQU 068h

;

OFFSETOF__MachState__m_pR12 EQU 070h

;

OFFSETOF__MachState__m_pR13 EQU 078h

;

OFFSETOF__MachState__m_pR14 EQU 080h

;

OFFSETOF__MachState__m_pR15 EQU 088h

;

OFFSETOF__MachState___pRetAddr EQU 090h

;

OFFSETOF__LazyMachState__m_CaptureRip EQU 098h

;

OFFSETOF__LazyMachState__m_CaptureRsp EQU 0A0h

;

OFFSETOF__MethodDesc__m_wFlags EQU 006h

;

OFFSETOF__VASigCookie__pNDirectMLStub EQU 08h

;

SIZEOF__CONTEXT EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + (2t*2t + 1t*2t + 2t + 4t + 2t*2t + 4t + 2t*2t + 4t*2t + 16t*8t + 16t*16t + 1t*96t) + 26t*16t + 8t + 8t*5t)

;

OFFSETOF__CONTEXT__Rax EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t)

;

OFFSETOF__CONTEXT__Rcx EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t)

;

OFFSETOF__CONTEXT__Rdx EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*2t)

;

OFFSETOF__CONTEXT__Rbx EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*3t)

;

OFFSETOF__CONTEXT__Rsp EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*4t)

;

OFFSETOF__CONTEXT__Rbp EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*5t)

;

OFFSETOF__CONTEXT__Rsi EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*6t)

;

OFFSETOF__CONTEXT__Rdi EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*7t)

;

OFFSETOF__CONTEXT__R8 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*8t)

;

OFFSETOF__CONTEXT__R9 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*9t)

;

OFFSETOF__CONTEXT__R10 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*10t)

;

OFFSETOF__CONTEXT__R11 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*11t)

;

OFFSETOF__CONTEXT__R12 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*12t)

;

OFFSETOF__CONTEXT__R13 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*13t)

;

OFFSETOF__CONTEXT__R14 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*14t)

;

OFFSETOF__CONTEXT__R15 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*15t)

;

OFFSETOF__CONTEXT__Rip EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t)

;

OFFSETOF__CONTEXT__Xmm0 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t)

;

OFFSETOF__CONTEXT__Xmm1 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t)

;

OFFSETOF__CONTEXT__Xmm2 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*2t)

;

OFFSETOF__CONTEXT__Xmm3 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*3t)

;

OFFSETOF__CONTEXT__Xmm4 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*4t)

;

OFFSETOF__CONTEXT__Xmm5 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*5t)

;

OFFSETOF__CONTEXT__Xmm6 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*6t)

;

OFFSETOF__CONTEXT__Xmm7 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*7t)

;

OFFSETOF__CONTEXT__Xmm8 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*8t)

;

OFFSETOF__CONTEXT__Xmm9 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*9t)

;

OFFSETOF__CONTEXT__Xmm10 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*10t)

;

OFFSETOF__CONTEXT__Xmm11 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*11t)

;

OFFSETOF__CONTEXT__Xmm12 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*12t)

;

OFFSETOF__CONTEXT__Xmm13 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*13t)

;

OFFSETOF__CONTEXT__Xmm14 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*14t)

;

OFFSETOF__CONTEXT__Xmm15 EQU (8t*6t + 4t*2t + 2t*6t + 4t + 8t*6t + 8t*16t + 8t + 2t*16t + 8t*16t + 16t*15t)

;

SIZEOF__FaultingExceptionFrame EQU (0x68 + 8t + (8*6 + 4*2 + 2*6 + 4 + 8*6 + 8*16 + 8 + (2*2 + 1*2 + 2 + 4 + 2*2 + 4 + 2*2 + 4*2 + 16*8 + 16*16 + 1*96) + 26*16 + 8 + 8*5))

;

OFFSETOF__VARARGS__ArgPtr EQU 018h

;

OFFSETOF__VARARGS__ArgCookie EQU 00h

;

OFFSETOF__VASigCookie__sizeOfArgs EQU 00h

;

OFFSETOF__PtrArray__m_NumComponents EQU 08h

;

OFFSETOF__PtrArray__m_ElementType EQU 010h

;

OFFSETOF__PtrArray__m_Array EQU 018h

;


MethodDescClassification__mdcStatic EQU 020h

;

OFFSET__TEB__TlsSlots EQU 01480h

;

OFFSETOF__TEB__LastErrorValue EQU 068h

;





REDIRECTSTUB_ESTABLISHER_OFFSET_RBP EQU 0t

REDIRECTSTUB_RBP_OFFSET_CONTEXT EQU 020h


THROWSTUB_ESTABLISHER_OFFSET_FaultingExceptionFrame EQU 030h

THROWSTUB_ESTABLISHER_OFFSET_HANDLER_BOOL EQU 020h






UMENTRYPRESTUB_ESTABLISHER_OFFSET_UMThkCallFrame EQU (020h + 8t + 3t*8t + 040h + 8t + 028h)


UMTHUNKSTUB_HOST_NOTIFY_FLAG_RBPOFFSET EQU (040h)




ResolveWorkerStaticDecoratedName TEXTEQU <?ResolveWorkerStatic@VirtualCallStubManager@@CAPEBEPEAVStubDispatchFrame@@PEAPEBE_K@Z>

GetTargetForTPWorkerQuickDecoratedName TEXTEQU <?GetTargetForTPWorkerQuick@VirtualCallStubManager@@SAPEBE_KPEAVMethodTable@@@Z>


Thread__ObjectRefFlush TEXTEQU <?ObjectRefFlush@Thread@@SAXPEAV1@@Z>



DELEGATE_FIELD_OFFSET__METHOD_AUX EQU 020h

;


ASM_LARGE_OBJECT_SIZE EQU 85000t

;

ASM_LARGE_ELEMENT_SIZE EQU 24t

;

OFFSETOF__ArrayBase__m_NumComponents EQU 8t

;

OFFSETOF__StringObject__m_ArrayLength EQU 08h

;

OFFSETOF__StringObject__m_StringLength EQU 0ch

;

OFFSETOF__ArrayTypeDesc__m_TemplateMT EQU 8t

;

OFFSETOF__ArrayTypeDesc__m_Arg EQU 010h

;

THREAD_CATCHATSAFEPOINT_BITS EQU 05Fh

;

SYNCBLOCKINDEX_OFFSET EQU 04h

;







