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

#ifndef YILI_RUNTIME_H
#define YILI_RUNTIME_H

#include "../yoyo/core.h"
#include "../yoyo/debug.h"
#include "../yoyo/exceptions.h"
#include "../yoyo/gc.h"
#include "yoyo/bytecode.h"

#define TO_STRING L"toString"
#define HASHCODE L"hashCode"
#define READ_INDEX L"#readIndex"
#define WRITE_INDEX L"#writeIndex"
#define EQUALS L"equals"
#ifndef __cplusplus
#define YOYO_FUNCTION(name) YValue* name(YLambda* lambda,\
	YValue** args, size_t argc, YThread* th)
#else
#define YOYO_FUNCTION(name) extern "C" YValue* name(YLambda* lambda,\
	YValue** args, size_t argc, YThread* th)
#endif

#define COMPARE_EQUALS 1
#define COMPARE_NOT_EQUALS (1 << 1)
#define COMPARE_LESSER (1 << 2)
#define COMPARE_GREATER (1 << 3)
#define COMPARE_LESSER_OR_EQUALS (1 << 4)
#define COMPARE_GREATER_OR_EQUALS (1 << 5)
#define COMPARE_PTR_EQUALS (1 << 6)

#define CHECK_EQUALS(v1, v2, th) ((v1->type->oper.compare(v1, v2, th)&COMPARE_EQUALS)!=0)

typedef YValue* (*BinaryOperation)(YValue*, YValue*, YThread*);
typedef YValue* (*UnaryOperation)(YValue*, YThread*);
typedef struct Operations {
	wchar_t* (*toString)(YValue*, YThread*);
	int (*compare)(YValue*, YValue*, YThread*);

	BinaryOperation add_operation;
	BinaryOperation subtract_operation;
	BinaryOperation multiply_operation;
	BinaryOperation divide_operation;
	BinaryOperation modulo_operation;
	BinaryOperation power_operation;
	BinaryOperation and_operation;
	BinaryOperation or_operation;
	BinaryOperation xor_operation;
	BinaryOperation shl_operation;
	BinaryOperation shr_operation;

	UnaryOperation not_operation;
	UnaryOperation negate_operation;

	YValue* (*readProperty)(int32_t, YValue*, YThread*);
	uint64_t (*hashCode)(YValue*, YThread*);
	YValue* (*readIndex)(YValue*, YValue*, YThread*);
	YValue* (*writeIndex)(YValue*, YValue*, YValue*, YThread*);
	YValue* (*subseq)(YValue*, size_t, size_t, YThread*);
	YoyoIterator* (*iterator)(YValue*, YThread*);
} Operations;

typedef struct YType {
	ValueType type;
	Operations oper;
	YoyoType* TypeConstant;
} YType;

typedef struct CatchBlock {
	uint32_t pc;
	struct CatchBlock* prev;
} CatchBlock;

typedef struct ExecutionFrame {
	size_t regc;

	YValue** regs;

	YValue** stack;
	size_t stack_offset;
	size_t stack_size;

	size_t pc;

	CatchBlock* catchBlock;

	YoyoType* retType;
	ILProcedure* proc;

	struct ExecutionFrame* prev;

	void* debug_ptr;
	int64_t debug_field;
	uint64_t debug_flags;
	void* breakpoint;
} ExecutionFrame;

typedef struct YThread {
	uint32_t id;
	enum {
		Core, Normal
	} type;
	enum {
		Working, Paused
	} state;

	YValue* exception;

	ExecutionFrame* frame;

	YRuntime* runtime;
	void (*free)(YThread*);
} YThread;

typedef struct CompilationResult {
	int32_t pid;
	wchar_t* log;
} CompilationResult;

typedef struct Environment {
	FILE* out_stream;
	FILE* in_stream;
	FILE* err_stream;

	wchar_t** argv;
	size_t argc;

	void (*free)(struct Environment*, struct YRuntime*);
	YObject* (*system)(struct Environment*, YRuntime*);
	struct CompilationResult (*eval)(struct Environment*, YRuntime*, wchar_t*);
	struct CompilationResult (*parse)(struct Environment*, struct YRuntime*,
			wchar_t*);
	wchar_t* (*getDefined)(struct Environment*, wchar_t*);
	void (*define)(struct Environment*, wchar_t*, wchar_t*);
	FILE* (*getFile)(struct Environment*, wchar_t*);
	void (*addPath)(struct Environment*, wchar_t*);
	wchar_t** (*getLoadedFiles)(struct Environment*);
} Environment;

#define INT_POOL_SIZE 10000
typedef struct YRuntime {
	enum {
		RuntimeRunning, RuntimePaused, RuntimeTerminated
	} state;
	Environment* env;
	ILBytecode* bytecode;
	SymbolMap symbols;
	GarbageCollector* gc;
	YDebug* debugger;
	YObject* global_scope;
	struct {
		YInteger* IntPool[INT_POOL_SIZE];
		YBoolean* TrueValue;
		YBoolean* FalseValue;
		YValue* NullPtr;
		YObject* pool;
	} Constants;
	YType IntType;
	YType FloatType;
	YType BooleanType;
	YType StringType;
	YType ObjectType;
	YType ArrayType;
	YType LambdaType;
	YType DeclarationType;
	YType NullType;
	YObject* AbstractObject;

	YThread** threads;
	size_t thread_size;
	size_t thread_count;
	YThread* CoreThread;

	MUTEX runtime_mutex;
	COND suspend_cond;
	THREAD gc_thread;

	void (*free)(YRuntime*);
	YValue* (*interpret)(int32_t, YRuntime*);
	YObject* (*newObject)(YObject*, YThread*);
	void (*wait)(YRuntime*);
} YRuntime;

YObject* yili_getSystem(YThread*);
YValue* invokeLambda(YLambda*, YValue**, size_t, YThread*);
YThread* newThread(YRuntime*);
YRuntime* newRuntime(Environment*, YDebug*);
wchar_t* toString(YValue*, YThread*);

#endif
