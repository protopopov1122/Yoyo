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

#include "yoyo-runtime.h"
#include "types/types.h"

/* File contains procedures to work
 * with runtime and perform garbage collection*/

void freeRuntime(YRuntime* runtime) {
	DEBUG(runtime->debugger, onunload, NULL, yoyo_thread(runtime));
	runtime->state = RuntimeTerminated;
	for (size_t i = 0; i < runtime->symbols.size; i++)
		free(runtime->symbols.map[i].symbol);
	free(runtime->symbols.map);
	DESTROY_MUTEX(&runtime->symbols.mutex);
	runtime->env->free(runtime->env, runtime);
	for (uint32_t i = 0; i < runtime->threads_capacity; i++)
		if (runtime->threads[i] != NULL)
			runtime->threads[i]->free(runtime->threads[i]);
	free(runtime->Constants.IntCache);
	free(runtime->threads);
	free(runtime);
}
void freeThread(YThread* th) {
	YRuntime* runtime = th->runtime;
	MUTEX_LOCK(&runtime->runtime_mutex);

	runtime->threads[th->id] = NULL;
	runtime->threads_size--;

	MUTEX_UNLOCK(&runtime->runtime_mutex);
	DESTROY_MUTEX(&th->mutex);
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
	sleep(2);
	clock_t last_gc = clock();
	while (runtime->state != RuntimeTerminated) {
		YIELD();
		if (!gc->panic&&(runtime->gc->block||
			runtime->state==RuntimePaused||
			clock()-last_gc<CLOCKS_PER_SEC/4))
			continue;
		bool panic = gc->panic;
		if (panic)
			runtime->state = RuntimePaused;
		// Mark all root objects
		MARK(runtime->global_scope);
		for (size_t i = 0; i < runtime->threads_capacity; i++) {
			if (runtime->threads[i] != NULL) {
				YThread* th = runtime->threads[i];
				MUTEX_LOCK(&th->mutex);
				MARK(th->exception);
				LocalFrame* frame = th->frame;
				while (frame != NULL) {
					frame->mark(frame);
					frame = frame->prev;
				}
				MUTEX_UNLOCK(&th->mutex);
			}
		}
//		MUTEX_UNLOCK(&runtime->runtime_mutex);
		for (size_t i = 0; i < runtime->Constants.IntCacheSize; i++)
			MARK(runtime->Constants.IntCache[i]);
		for (size_t i = 0; i < INT_POOL_SIZE; i++)
			MARK(runtime->Constants.IntPool[i]);
		MARK(runtime->Constants.TrueValue);
		MARK(runtime->Constants.FalseValue);
		MARK(runtime->Constants.NullPtr);
		MARK(runtime->Constants.pool);
		MARK(runtime->global_scope);
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
		if (panic)
			runtime->state = RuntimeRunning;
		last_gc = clock();
	}
	gc->free(gc);
	THREAD_EXIT(NULL);
	return NULL;
}

// Wait while there are working threads
void Runtime_wait(YRuntime* runtime) {
	while (runtime->threads_size > 0)
		YIELD();
}
/*Procedure used to invoke lambdas.
 * Check if arguments fits lambda signature.
 * If lambda is variable argument then create array of last arguments.
 * Invoke lambda.*/
YValue* invokeLambda(YLambda* l, YObject* scope, YValue** targs, size_t argc,
		YThread* th) {
	((YoyoObject*) l)->linkc++; // To prevent lambda garbage collection
	YValue** args = NULL;

	if (scope == NULL && l->sig->method) {
		if (argc == 0 || targs[0]->type->type != ObjectT) {
			throwException(L"LambdaArgumentMismatch", NULL, 0, th);
			((YoyoObject*) l)->linkc--;
			return getNull(th);
		}
		scope = (YObject*) targs[0];
		targs++;
		argc--;
	}

	/*Check if argument count equals lambda argument count and
	 * throws exception if not and lambda isn't vararg*/
	if (l->sig->argc != argc && l->sig->argc != -1) {
		if (!(l->sig->vararg && l->sig->argc - 1 <= argc)) {
			throwException(L"LambdaArgumentMismatch", NULL, 0, th);
			((YoyoObject*) l)->linkc--;
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
		for (size_t i = 0; i < argc && i < l->sig->argc; i++) {
			if (l->sig->args[i] != NULL
					&& !l->sig->args[i]->verify(l->sig->args[i], args[i], th)) {
				wchar_t* wstr = toString(args[i], th);
				throwException(L"Wrong argument type", &wstr, 1, th);
				free(wstr);
				free(args);
				((YoyoObject*) l)->linkc--;
				return getNull(th);
			}
		}
	}

	// Invoke lambda
	YValue* out = l->execute(l, scope, args,
			l->sig->argc != -1 ? l->sig->argc : argc, th);
	((YoyoObject*) l)->linkc--;
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

	runtime->threads_capacity = 1;
	runtime->threads = calloc(runtime->threads_capacity, sizeof(YThread*));
	runtime->threads_size = 0;

	runtime->gc = newPlainGC(1000);
	runtime->free = freeRuntime;
	if (env->getDefined(env, L"object")!=NULL&&
		wcscmp(env->getDefined(env, L"object"), L"tree")==0)
		runtime->newObject = newTreeObject;
	else
		runtime->newObject = newHashObject;
	runtime->wait = Runtime_wait;

	Types_init(runtime);
	runtime->Constants.IntCacheSize = INT_CACHE_SIZE;
	runtime->Constants.IntPoolSize = INT_POOL_SIZE;
	if (env->getDefined(env, L"IntCache")!=NULL) {
		size_t cache_size = wcstoul(env->getDefined(env, L"IntCache"), NULL, 0);
		if (cache_size>0)
			runtime->Constants.IntCacheSize = cache_size;
	}
	runtime->Constants.IntCache = calloc(runtime->Constants.IntCacheSize, sizeof(YInteger*));
	memset(runtime->Constants.IntPool, 0, sizeof(YInteger*) * INT_POOL_SIZE);

	YThread* th = yoyo_thread(runtime);
	runtime->Constants.TrueValue = (YBoolean*) newBooleanValue(true, th);
	runtime->Constants.FalseValue = (YBoolean*) newBooleanValue(false, th);
	runtime->Constants.NullPtr = NULL;

	runtime->symbols.map = NULL;
	runtime->symbols.size = 0;
	NEW_MUTEX(&runtime->symbols.mutex);

	runtime->global_scope = th->runtime->newObject(NULL, th);
	runtime->Constants.pool = th->runtime->newObject(NULL, th);

	NEW_THREAD(&runtime->gc_thread, GCThread, runtime);

	DEBUG(runtime->debugger, onload, NULL, yoyo_thread(runtime));

	return runtime;
}

YThread* yoyo_thread(YRuntime* runtime) {
	THREAD current_th = THREAD_SELF();
	for (size_t i = 0; i < runtime->threads_size; i++)
		if (runtime->threads[i]!=NULL&&
			THREAD_EQUAL(current_th, runtime->threads[i]->self))
			return runtime->threads[i];
	YThread* th = malloc(sizeof(YThread));
	NEW_MUTEX(&th->mutex);
	th->runtime = runtime;
	th->state = Working;
	th->free = freeThread;
	th->frame = NULL;
	th->exception = NULL;
	th->self = THREAD_SELF();
	MUTEX_LOCK(&runtime->runtime_mutex);

	if (runtime->threads_size + 1 >= runtime->threads_capacity) {
		size_t newSz = runtime->threads_capacity / 10 + runtime->threads_capacity;
		runtime->threads = realloc(runtime->threads, sizeof(YThread*) * newSz);
		runtime->threads_capacity = newSz;
	}
	th->id = runtime->threads_size++;
	runtime->threads[th->id] = th;

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
				YValue* ret = invokeLambda(exec, NULL, NULL, 0, th);
				wchar_t* out = toString(ret, th);
				return out;
			}
		}
	}
	return v->type->oper.toString(v, th);
}

// Work with the symbol map
int32_t getSymbolId(SymbolMap* map, wchar_t* wsym) {
	if (wsym == NULL)
		return -1;
	MUTEX_LOCK(&map->mutex);
	for (size_t i = 0; i < map->size; i++)
		if (wcscmp(map->map[i].symbol, wsym) == 0) {
			int32_t value = map->map[i].id;
			MUTEX_UNLOCK(&map->mutex);
			return value;
		}
	int32_t id = (int32_t) map->size++;
	map->map = realloc(map->map, sizeof(SymbolMapEntry) * map->size);
	map->map[id].id = id;
	map->map[id].symbol = calloc(1, sizeof(wchar_t) * (wcslen(wsym) + 1));
	wcscpy(map->map[id].symbol, wsym);
	MUTEX_UNLOCK(&map->mutex);
	return id;
}
wchar_t* getSymbolById(SymbolMap* map, int32_t id) {
	if (id < 0 || id >= map->size)
		return NULL;
	MUTEX_LOCK(&map->mutex);
	wchar_t* wcs = map->map[id].symbol;
	MUTEX_UNLOCK(&map->mutex);
	return wcs;
}
