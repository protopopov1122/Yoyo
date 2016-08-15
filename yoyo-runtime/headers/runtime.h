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

#include "core.h"
#include "debug.h"
#include "exceptions.h"
#include "gc.h"

#define TO_STRING L"toString"
#define HASHCODE L"hashCode"
#define READ_INDEX L"#readIndex"
#define WRITE_INDEX L"#writeIndex"
#define REMOVE_INDEX L"#removeIndex"
#define EQUALS L"equals"

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
	YValue* (*removeIndex)(YValue*, YValue*, YThread*);
	YValue* (*subseq)(YValue*, size_t, size_t, YThread*);
	YoyoIterator* (*iterator)(YValue*, YThread*);
} Operations;

typedef struct YType {
	Operations oper;
	YoyoType* TypeConstant;
	wchar_t* wstring;
} YType;

typedef struct SourceIdentifier {
	int32_t file;
	uint32_t line;
	uint32_t charPosition;
} SourceIdentifier;
typedef struct LocalFrame {
	void (*mark)(struct LocalFrame*);
	SourceIdentifier (*get_source_id)(struct LocalFrame*);
	struct LocalFrame* prev;
} LocalFrame;

typedef struct YThread {
	uint32_t id;
	enum {
		Working, Paused
	} state;

	MUTEX mutex;
	YValue* exception;
	LocalFrame* frame;

	THREAD self;

	YRuntime* runtime;
	void (*free)(YThread*);

	struct YThread* prev;
	struct YThread* next;
} YThread;

typedef struct Environment {
	FILE* out_stream;
	FILE* in_stream;
	FILE* err_stream;

	wchar_t** argv;
	size_t argc;

	void (*free)(struct Environment*, struct YRuntime*);
	YObject* (*system)(struct Environment*, YRuntime*);
	YValue* (*eval)(struct Environment*, YRuntime*, InputStream*, wchar_t*,
			YObject*);
	wchar_t* (*getDefined)(struct Environment*, wchar_t*);
	void (*define)(struct Environment*, wchar_t*, wchar_t*);
	FILE* (*getFile)(struct Environment*, wchar_t*);
	void (*addPath)(struct Environment*, wchar_t*);
	wchar_t** (*getLoadedFiles)(struct Environment*);
} Environment;

#ifndef INT_CACHE_SIZE
#define INT_CACHE_SIZE 8192 /* 2**12 */
#endif
#ifndef INT_POOL_SIZE
#define INT_POOL_SIZE 2048 /* 2**11 */
#endif

typedef struct RuntimeConstants {
	size_t IntCacheSize;
	size_t IntPoolSize;
	YInteger** IntCache;
	YInteger** IntPool;
	YBoolean* TrueValue;
	YBoolean* FalseValue;
	YValue* NullPtr;
	YObject* pool;
} RuntimeConstants;

typedef struct YRuntime {
	enum {
		RuntimeRunning, RuntimePaused, RuntimeTerminated
	} state;
	Environment* env;
	SymbolMap symbols;
	GarbageCollector* gc;
	YDebug* debugger;
	YObject* global_scope;
	RuntimeConstants Constants;
	YType IntType;
	YType FloatType;
	YType BooleanType;
	YType StringType;
	YType ObjectType;
	YType ArrayType;
	YType LambdaType;
	YType DeclarationType;
	YType NullType;

	YThread* threads;
	size_t thread_count;

	MUTEX runtime_mutex;
	COND suspend_cond;
	THREAD gc_thread;

	void (*free)(YRuntime*);
	YObject* (*newObject)(YObject*, YThread*);
	void (*wait)(YRuntime*);
} YRuntime;

YObject* new_yoyo_thread(YRuntime*, YLambda*);
YValue* invokeLambda(YLambda*, YObject*, YValue**, size_t, YThread*);
YThread* yoyo_thread(YRuntime*);
YRuntime* newRuntime(Environment*, YDebug*);
wchar_t* toString(YValue*, YThread*);

#endif
