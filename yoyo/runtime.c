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

#include "../headers/yoyo/runtime.h"

#include "../yoyo/types/types.h"

#include "../headers/yoyo/core.h"
#include "../headers/yoyo/stringbuilder.h"
#include "yoyo/interpreter.h"

/* File contains procedures to work
 * with runtime and perform garbage collection*/

void freeRuntime(YRuntime* runtime) {
	DEBUG(runtime->debugger, onunload, NULL, runtime->CoreThread);
	runtime->state = RuntimeTerminated;
	for (size_t i = 0; i < runtime->symbols.size; i++)
		free(runtime->symbols.map[i].symbol);
	free(runtime->symbols.map);
	runtime->env->free(runtime->env, runtime);
	for (uint32_t i = 0; i < runtime->thread_size; i++)
		if (runtime->threads[i] != NULL)
			runtime->threads[i]->free(runtime->threads[i]);
	free(runtime->threads);
	freeBytecode(runtime->bytecode);
	free(runtime);
}
void freeThread(YThread* th) {
	YRuntime* runtime = th->runtime;
	MUTEX_LOCK(&runtime->runtime_mutex);

	runtime->threads[th->id] = NULL;
	runtime->thread_count--;

	MUTEX_UNLOCK(&runtime->runtime_mutex);
	free(th);
}

void Types_init(YRuntime* runtime) {
	Int_type_init(runtime);
	Float_type_init(runtime);
	Boolean_type_init(runtime);
	String_type_init(runtime);
	Array_type_init(runtime);
	Object_type_init(runtime);
	Lambda_type_init(runtime);
	Declaration_type_init(runtime);
	Null_type_init(runtime);
}

/* This procedure works while runtime
 * is not terminated. It performs garbage collection
 * every second, after terminating
 * it frees garbage collector*/
void* GCThread(void* ptr) {
	YRuntime* runtime = (YRuntime*) ptr;
	GarbageCollector* gc = runtime->gc;
	while (runtime->state == RuntimeRunning) {
		sleep(1);

		// Lock runtime
		runtime->state = RuntimePaused;
		MUTEX_LOCK(&runtime->runtime_mutex);

		// Mark all root objects
		MARK(runtime->global_scope);
		for (size_t i = 0; i < runtime->thread_size; i++) {
			if (runtime->threads[i] != NULL) {
				YThread* th = runtime->threads[i];
				MARK(th->exception);
				ExecutionFrame* frame = th->frame;
				while (frame != NULL) {
					for (size_t j = 0; j < frame->regc; j++)
						MARK(frame->regs[j]);
					for (size_t j = 0; j < frame->stack_offset; j++) {
						MARK(frame->stack[j]);
						MARK(frame->retType);
					}
					frame = frame->prev;
				}
			}
		}
		MUTEX_UNLOCK(&runtime->runtime_mutex);
		for (size_t i = 0; i < INT_POOL_SIZE; i++)
			if (runtime->Constants.IntPool[i] != NULL)
				MARK(runtime->Constants.IntPool[i]);
		MARK(runtime->Constants.TrueValue);
		MARK(runtime->Constants.FalseValue);
		MARK(runtime->Constants.NullPtr);
		MARK(runtime->Constants.pool);
		MARK(runtime->global_scope);
		MARK(runtime->AbstractObject);
#define MARKTYPE(type) MARK(runtime->type.TypeConstant);
		MARKTYPE(ArrayType);
		MARKTYPE(BooleanType);
		MARKTYPE(DeclarationType);
		MARKTYPE(FloatType);
		MARKTYPE(IntType);
		MARKTYPE(LambdaType);
		MARKTYPE(NullType);
		MARKTYPE(ObjectType);
		MARKTYPE(StringType);
#undef MARKTYPE

		// Collect garbage
		gc->collect(gc);
		// Unlock runtime
		runtime->state = RuntimeRunning;
	}
	gc->free(gc);
	THREAD_EXIT(NULL);
	return NULL;
}

// Invoke procedure in new thread
YValue* Runtime_interpret(int32_t pid, YRuntime* runtime) {
	YThread* th = newThread(runtime);
	YValue* out = invoke(pid, runtime->global_scope, NULL, th);
	if (th->exception != NULL) {
		wchar_t* wstr = toString(th->exception, th);
		fprintf(runtime->env->out_stream, "%ls\n", wstr);
		free(wstr);
		th->exception = NULL;
	}
	th->free(th);
	return out;
}

// Wait while there are working threads
void Runtime_wait(YRuntime* runtime) {
	while (runtime->thread_count > 1)
		YIELD();
}
/*Procedure used to invoke lambdas.
 * Check if arguments fits lambda signature.
 * If lambda is variable argument then create array of last arguments.
 * Invoke lambda.*/
YValue* invokeLambda(YLambda* l, YValue** targs, size_t argc, YThread* th) {
	((HeapObject*) l)->linkc++; // To prevent lambda garbage collection
	YValue** args = NULL;

	/*Check if argument count equals lambda argument count and
	 * throws exception if not and lambda isn't vararg*/
	if (l->sig->argc != argc && l->sig->argc != -1) {
		if (!(l->sig->vararg && l->sig->argc - 1 <= argc)) {
			throwException(L"LambdaArgumentMismatch", NULL, 0, th);
			((HeapObject*) l)->linkc--;
			return getNull(th);
		}
	}

	/*If lambda is vararg, create array of last arguments*/
	if (l->sig->vararg) {
		if (!(l->sig->argc == argc && targs[argc - 1]->type->type == ArrayT)) {
			args = malloc(sizeof(YValue*) * l->sig->argc);
			for (size_t i = 0; i < l->sig->argc - 1; i++)
				args[i] = targs[i];
			YArray* vararg = newArray(th);
			args[l->sig->argc - 1] = (YValue*) vararg;
			for (size_t i = l->sig->argc - 1; i < argc; i++)
				vararg->add(vararg, targs[i], th);
		}
	}
	if (args == NULL) {
		args = malloc(sizeof(YValue*) * argc);
		memcpy(args, targs, sizeof(YValue*) * argc);
	}

	/*Check each argument type*/
	if (l->sig->args != NULL) {
		for (size_t i = 0; i < argc && i<l->sig->argc; i++) {
			if (l->sig->args[i] != NULL
					&& !l->sig->args[i]->verify(l->sig->args[i], args[i], th)) {
				wchar_t* wstr = toString(args[i], th);
				throwException(L"Wrong argument type", &wstr, 1, th);
				free(wstr);
				free(args);
				((HeapObject*) l)->linkc--;
				return getNull(th);
			}
		}
	}

	// Invoke lambda
	YValue* out = l->execute(l, args, l->sig->argc != -1 ? l->sig->argc : argc,
			th);
	((HeapObject*) l)->linkc--;
	free(args);
	return out;
}

// Create new runtime
YRuntime* newRuntime(Environment* env, YDebug* debug) {
	YRuntime* runtime = malloc(sizeof(YRuntime));
	runtime->debugger = debug;

	runtime->env = env;

	runtime->state = RuntimeRunning;
	NEW_MUTEX(&runtime->runtime_mutex);

	runtime->thread_size = 10;
	runtime->threads = malloc(sizeof(YThread*) * runtime->thread_size);
	for (size_t i = 0; i < runtime->thread_size; i++)
		runtime->threads[i] = NULL;
	runtime->thread_count = 0;
	runtime->CoreThread = newThread(runtime);
	runtime->CoreThread->type = Core;

	runtime->gc = newGenerationalGC(1000, 3);
	runtime->free = freeRuntime;
	runtime->interpret = Runtime_interpret;
	runtime->newObject = newHashObject;
	runtime->wait = Runtime_wait;

	Types_init(runtime);
	for (size_t i = 0; i < INT_POOL_SIZE; i++)
		runtime->Constants.IntPool[i] = NULL;

	YThread* th = runtime->CoreThread;
	runtime->Constants.TrueValue = (YBoolean*) newBooleanValue(true, th);
	runtime->Constants.FalseValue = (YBoolean*) newBooleanValue(false, th);
	runtime->Constants.NullPtr = NULL;
	runtime->AbstractObject = NULL;

	runtime->symbols.map = NULL;
	runtime->symbols.size = 0;
	runtime->bytecode = newBytecode(&runtime->symbols);

	runtime->global_scope = th->runtime->newObject(NULL, th);
	runtime->Constants.pool = th->runtime->newObject(NULL, th);
	runtime->AbstractObject = th->runtime->newObject(NULL, th);
	ADD_FIELD(runtime->global_scope, L"sys",
			yili_getSystem(runtime->CoreThread), th);

	NEW_THREAD(&runtime->gc_thread, GCThread, runtime);

	DEBUG(runtime->debugger, onload, NULL, runtime->CoreThread);

	return runtime;
}

YThread* newThread(YRuntime* runtime) {
	YThread* th = malloc(sizeof(YThread));
	th->runtime = runtime;
	th->state = Working;
	th->free = freeThread;
	th->frame = NULL;
	th->exception = NULL;
	th->type = Normal;
	MUTEX_LOCK(&runtime->runtime_mutex);

	th->id = 0;
	if (runtime->thread_count + 1 >= runtime->thread_size) {
		size_t newSz = runtime->thread_size / 10 + runtime->thread_size;
		runtime->threads = realloc(runtime->threads, sizeof(YThread*) * newSz);
		for (size_t i = runtime->thread_size; i < newSz; i++)
			runtime->threads[i] = NULL;
		runtime->thread_size = newSz;
	}
	for (size_t i = 0; i < runtime->thread_size; i++) {
		if (runtime->threads[i] == NULL) {
			th->id = i;
			runtime->threads[i] = th;
			break;
		}
	}
	runtime->thread_count++;

	MUTEX_UNLOCK(&runtime->runtime_mutex);
	return th;
}

wchar_t* toString(YValue* v, YThread* th) {
	int32_t id = getSymbolId(&th->runtime->symbols,
	TO_STRING);
	if (v->type->type == ObjectT) {
		YObject* obj = (YObject*) v;
		if (obj->contains(obj, id, th)) {
			YValue* val = obj->get(obj, id, th);
			if (val->type->type == LambdaT) {
				YLambda* exec = (YLambda*) val;
				YValue* ret = invokeLambda(exec, NULL, 0, th);
				wchar_t* out = toString(ret, th);
				return out;
			}
		}
	}
	return v->type->oper.toString(v, th);
}

// Work with the symbol map
int32_t getSymbolId(SymbolMap* map, wchar_t* wsym) {
	for (size_t i = 0; i < map->size; i++)
		if (wcscmp(map->map[i].symbol, wsym) == 0)
			return map->map[i].id;
	int32_t id = (int32_t) map->size++;
	map->map = realloc(map->map, sizeof(SymbolMapEntry) * map->size);
	map->map[id].id = id;
	map->map[id].symbol = calloc(1, sizeof(wchar_t) * (wcslen(wsym) + 1));
	wcscpy(map->map[id].symbol, wsym);
	return id;
}
wchar_t* getSymbolById(SymbolMap* map, int32_t id) {
	if (id < 0 || id >= map->size)
		return NULL;
	return map->map[id].symbol;
}
