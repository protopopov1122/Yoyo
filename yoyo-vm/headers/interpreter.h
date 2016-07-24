/*
 * Copyright (C) 2016  Jevgenijs Protopopovs <protopopov1122@yandex.ru>
 */
/*This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#ifndef YILI_INTERPRETER_H
#define YILI_INTERPRETER_H

#include "yoyo-runtime.h"
#include "opcodes.h"
#include "bytecode.h"
#include "yoyo_io.h"

/*Procedures used to interpret bytecode.
 * See virtual machine description and 'interpreter.c'.*/

typedef struct CompilationResult {
	int32_t pid;
	wchar_t* log;
} CompilationResult;

typedef struct CatchBlock {
	uint32_t pc;
	struct CatchBlock* prev;
} CatchBlock;

typedef struct ExecutionFrame {
	LocalFrame frame;

	ILBytecode* bytecode;
	size_t regc;

	YValue** regs;

	YValue** stack;
	size_t stack_offset;
	size_t stack_size;

	size_t pc;

	CatchBlock* catchBlock;

	YoyoType* retType;
	ILProcedure* proc;

	void* debug_ptr;
	int64_t debug_field;
	uint64_t debug_flags;
	void* breakpoint;
} ExecutionFrame;

YObject* Yoyo_SystemObject(ILBytecode*, YThread*);
YLambda* newProcedureLambda(int32_t, ILBytecode*, YObject*, int32_t*,
		YoyoLambdaSignature*, YThread*);
YValue* invoke(int32_t, ILBytecode*, YObject*, YoyoType*, YThread*);
YValue* execute(YThread*);
YDebug* newDefaultDebugger(ILBytecode*);

#endif
