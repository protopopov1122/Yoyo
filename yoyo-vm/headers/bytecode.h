/*
 * Copyright (C) 2016  Jevgenijs Protopopovs <protopopov1122@yandex.ru>
 */
/*This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 3 as published by
 the Free Software Foundation.


 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#ifndef YOYO_VM_BYTECODE_H
#define YOYO_VM_BYTECODE_H

#include "core.h"
#include "jit.h"

typedef struct Constant {
	int32_t id;
	enum {
		IntegerC, FloatC, BooleanC, StringC, NullC
	} type;
	union {
		int64_t i64;
		double fp64;
		bool boolean;
		int32_t string_id;
	} value;
} Constant;
typedef Constant YConstant;

typedef struct ConstantPool {
	Constant** pool;
	uint32_t size;
} ConstantPool;

typedef struct LabelEntry {
	int32_t id;
	uint32_t value;
} LabelEntry;

typedef struct LabelTable {
	LabelEntry* table;
	size_t length;
} LabelTable;

typedef struct CodeTableEntry {
	uint32_t line;
	uint32_t charPos;
	uint32_t offset;
	uint32_t end;
	int32_t file;
} CodeTableEntry;

typedef struct CodeTable {
	CodeTableEntry* table;
	size_t length;
} CodeTable;

typedef struct ILProcedure {
	YoyoObject o;
	int32_t id;
	uint32_t regc;
	size_t code_length;
	uint8_t* code;

	LabelTable labels;
	CodeTable codeTable;

	void (*appendCode)(struct ILProcedure*, uint8_t*, size_t);
	void (*addLabel)(struct ILProcedure*, int32_t, uint32_t);
	LabelEntry* (*getLabel)(struct ILProcedure*, int32_t);
	void (*addCodeTableEntry)(struct ILProcedure*, uint32_t, uint32_t, uint32_t,
			uint32_t, int32_t);
	CodeTableEntry* (*getCodeTableEntry)(struct ILProcedure*, uint32_t);
	void (*free)(struct ILProcedure*, struct ILBytecode*);

	struct ProcedureStats* stats;
	struct ILBytecode* bytecode;

	CompiledProcedure* compiled;
} ILProcedure;

typedef struct ILBytecode {
	YoyoObject o;

	SymbolMap* symbols;
	ConstantPool constants;

	ILProcedure** procedures;
	size_t procedure_count;

	MUTEX access_mutex;

	ILProcedure* (*newProcedure)(struct ILBytecode*);
	int32_t (*getSymbolId)(struct ILBytecode*, wchar_t*);
	wchar_t* (*getSymbolById)(struct ILBytecode*, int32_t);

	int32_t (*addIntegerConstant)(struct ILBytecode*, int64_t);
	int32_t (*addFloatConstant)(struct ILBytecode*, double);
	int32_t (*addBooleanConstant)(struct ILBytecode*, bool);
	int32_t (*addStringConstant)(struct ILBytecode*, wchar_t*);
	int32_t (*getNullConstant)(struct ILBytecode*);
	Constant* (*getConstant)(struct ILBytecode*, int32_t);

} ILBytecode;

ILBytecode* newBytecode(SymbolMap*);
void freeBytecode(ILBytecode*);

#endif
