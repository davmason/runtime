// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.


#define InlineNone 0x00
#define ShortInlineVar 0x01
#define ShortInlineI 0x02
#define InlineI 0x03
#define InlineI8 0x04
#define ShortInlineR 0x05
#define InlineR 0x06
#define InlineMethod 0x07
#define InlineSig 0x08
#define ShortInlineBrTarget 0x09
#define InlineBrTarget 0x10
#define InlineSwitch 0x11
#define InlineType 0x12
#define InlineString 0x13
#define InlineField 0x14
#define InlineTok 0x15
#define InlineVar 0x16

struct OpcodeInfo
{
	OpcodeInfo(const char *name, DWORD argType, BYTE b1, BYTE b2)
	{
		Name=name;
		ArgumentType=argType;
		Byte1=b1;
		Byte2=b2;
	}
	const char *Name;

	DWORD ArgumentType;
	DWORD Instrumentation;

	BYTE Byte1, Byte2;
};

#define OPDEF(macroName, stringName, stackBehavior1, stackBehavior2, operandParams, operandKind, length, byte1, byte2, controlFlow) \
	macroName = 	byte2,

enum Instruction
{
	#include <opcode.def>
};

#undef OPDEF
#define OPDEF(macroName, stringName, stackBehavior1, stackBehavior2, operandParams, operandKind, length, byte1, byte2, controlFlow) \
	OpcodeInfo(stringName, operandParams, byte1, byte2),



struct OpcodeInfo g_opcodes [] = {
#include <opcode.def>    
};
/*
#undef OPDEF
#define OPDEF(macroName, stringName, stackBehavior1, stackBehavior2, operandParams, operandKind, length, byte1, byte2, controlFlow) \
    const int stringName ## _byte1 = byte1;\
    const int stringName ## _byte2 = byte2;
*/
