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
	while (runtime->threads!=NULL) {
			runtime->threads->free(runtime->threads);
	}
	for (size_t i=0;i<runtime->Type_pool_size;i++)
		free(runtime->Type_pool[i]);
	free(runtime->Type_pool);
	free(runtime->Constants.IntCache);
	free(runtime->Constants.IntPool);
	free(runtime->threads);
	free(runtime);
}
void freeThread(YThread* th) {
	DESTROY_MUTEX(&th->mutex);
	YRuntime* runtime = th->runtime;
	MUTEX_LOCK(&runtime->runtime_mutex);

	if (th->prev != NULL)
		th->prev->next = th->next;
	if (th->next != NULL)
		th->next->prev = th->prev;
	if (runtime->threads == th)
		runtime->threads = th->prev;
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
	clock_t last_gc = clock();
	while (runtime->state != RuntimeTerminated) {
		YIELD();
		if (!gc->panic&&(runtime->gc->block||
			runtime->state==RuntimePaused||
			clock()-last_gc<2*CLOCKS_PER_SEC))
			continue;
		bool panic = gc->panic;
		runtime->state = RuntimePaused;
		bool wait = true;
		while (wait) {
			wait = false;
			YIELD();
			for (YThread* th = runtime->threads; th!=NULL; th = th->prev) {
				if (th->state==ThreadWorking) {
					wait = true;
					break;
				}
			}
		}
		// Mark all root objects
		MARK(runtime->global_scope);
		for (YThread* th = runtime->threads; th!=NULL; th=th->prev) {
				MUTEX_LOCK(&th->mutex);
				MARK(th->exception);
				LocalFrame* frame = th->frame;
				while (frame != NULL) {
					frame->mark(frame);
					frame = frame->prev;
				}
				MUTEX_UNLOCK(&th->mutex);
		}
//		MUTEX_UNLOCK(&runtime->runtime_mutex);
		for (size_t i = 0; i < runtime->Constants.IntCacheSize; i++)
			MARK(runtime->Constants.IntCache[i]);
		for (size_t i = 0; i < runtime->Constants.IntPoolSize; i++)
			MARK(runtime->Constants.IntPool[i]);
		MARK(runtime->Constants.TrueValue);
		MARK(runtime->Constants.FalseValue);
		MARK(runtime->Constants.NullPtr);
		MARK(runtime->Constants.pool);
		MARK(runtime->global_scope);
#define MARKTYPE(type) MARK(runtime->type.TypeConstant);\
												MARK(runtime->type.prototype);
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
		for (size_t i=0;i<runtime->Type_pool_size;i++)
			MARK(runtime->Type_pool[i]->TypeConstant);
		if (!panic)
			runtime->state = RuntimeRunning;


		// Collect garbage
		gc->collect(gc);
		runtime->state = RuntimeRunning;
		last_gc = clock();
	}
	gc->free(gc);
	THREAD_EXIT(NULL);
	return NULL;
}

// Wait while there are working threads
void Runtime_wait(YRuntime* runtime) {
	while (runtime->threads!=NULL)
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
		if (argc == 0) {
			throwException(L"LambdaArgumentMismatch", NULL, 0, th);
			((YoyoObject*) l)->linkc--;
			return getNull(th);
		}
		if (targs[0]->type != &th->runtime->ObjectType) {
			scope = th->runtime->newObject(NULL, th);
			OBJECT_NEW(scope, L"value", targs[0], th);
		} else
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
		if (!(l->sig->argc == argc && targs[argc - 1]->type == &th->runtime->ArrayType)) {
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
				throwException(L"WrongArgumentType", &wstr, 1, th);
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

typedef struct NewThread {
	YLambda* lambda;
	YRuntime* runtime;
	YObject* task;
	bool mark;
} NewThread;

void* launch_new_thread(void* ptr) {
	NewThread* nth = (NewThread*) ptr;
	YThread* th = yoyo_thread(nth->runtime);
	nth->mark = false;
	YValue* result = invokeLambda(nth->lambda, NULL, (YValue**) &nth->task, 1, th);
	if (th->exception!=NULL) {
		OBJECT_PUT(nth->task, L"exception", th->exception, th);
	} else {
		OBJECT_PUT(nth->task, L"result", result, th);
	}
	OBJECT_PUT(nth->task, L"finished", newBoolean(true, th), th);
	th->free(th);
	free(nth);
	THREAD_EXIT(NULL);
	return NULL;
}
	
YObject* new_yoyo_thread(YRuntime* runtime, YLambda* lambda) {
	NewThread* nth = malloc(sizeof(NewThread));
	YObject* task = runtime->newObject(NULL, yoyo_thread(runtime));
	OBJECT_NEW(task, L"result", getNull(yoyo_thread(runtime)), yoyo_thread(runtime));
	OBJECT_NEW(task, L"exception", getNull(yoyo_thread(runtime)), yoyo_thread(runtime));
	OBJECT_NEW(task, L"finished", newBoolean(false, yoyo_thread(runtime)), yoyo_thread(runtime));
	nth->lambda = lambda;
	nth->runtime = runtime;
	nth->task = task;
	nth->mark = true;
	THREAD pthr;
	NEW_THREAD(&pthr, launch_new_thread, nth);
	while (nth->mark)
		YIELD();	
	return task;
}

// Create new runtime
YRuntime* newRuntime(Environment* env, YDebug* debug) {
	YRuntime* runtime = calloc(1, sizeof(YRuntime));
	runtime->debugger = debug;

	runtime->env = env;

	runtime->state = RuntimeRunning;
	NEW_MUTEX(&runtime->runtime_mutex);

	runtime->threads = NULL;
	runtime->thread_count = 0;

	if (env->getDefined(env, L"GCGenerational")!=NULL) {
		size_t genCount = GENERATIONAL_GC_GEN_COUNT;
		size_t genGap = GENERATIONAL_GC_GEN_GAP;
		if (env->getDefined(env, L"GCGenerationCount")!=NULL) {
			size_t sz = wcstoul(env->getDefined(env, L"GCGenerationCount"), NULL, 0);
			if (sz>0)
				genCount = sz;
		}
		if (env->getDefined(env, L"GCGenerationGap")!=NULL) {
			size_t sz = wcstoul(env->getDefined(env, L"GCGenerationGap"), NULL, 0);
			if (sz>0)
				genGap = sz;
		}
		runtime->gc = newGenerationalGC(genCount, genGap);
	} else
		runtime->gc = newPlainGC();
	runtime->free = freeRuntime;
	if (env->getDefined(env, L"objects")!=NULL&&
		wcscmp(env->getDefined(env, L"objects"), L"tree")==0)
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
	if (env->getDefined(env, L"IntPool")!=NULL) {
		size_t pool_size = wcstoul(env->getDefined(env, L"IntPool"), NULL, 0);
		if (pool_size>0)
			runtime->Constants.IntPoolSize = pool_size;
	}
	runtime->Constants.IntCache = calloc(runtime->Constants.IntCacheSize, sizeof(YInteger*));
	runtime->Constants.IntPool = calloc(runtime->Constants.IntPoolSize, sizeof(YInteger*));

	YThread* th = yoyo_thread(runtime);
	runtime->Constants.TrueValue = (YBoolean*) newBooleanValue(true, th);
	runtime->Constants.FalseValue = (YBoolean*) newBooleanValue(false, th);
	runtime->Constants.NullPtr = NULL;

	runtime->Type_pool_size = 0;
	runtime->Type_pool_capacity = 5;
	runtime->Type_pool = malloc(sizeof(YType*) * runtime->Type_pool_capacity);

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
	YThread* th;
	for (th=runtime->threads; th!=NULL; th=th->prev)
		if (THREAD_EQUAL(current_th, th->self))
			return th;
	th = malloc(sizeof(YThread));
	th->runtime = runtime;
	th->state = ThreadWorking;
	th->free = freeThread;
	th->frame = NULL;
	th->exception = NULL;
	th->self = THREAD_SELF();
	NEW_MUTEX(&th->mutex);
	MUTEX_LOCK(&runtime->runtime_mutex);
	th->prev = runtime->threads;
	if (th->prev!=NULL)
		th->prev->next = th;
	th->next = NULL;
	runtime->threads = th;
	runtime->thread_count++;
	MUTEX_UNLOCK(&runtime->runtime_mutex);
	return th;
}

YType* yoyo_type(YRuntime* runtime) {
	if (runtime->Type_pool_size+1>=runtime->Type_pool_capacity) {
		runtime->Type_pool_capacity += 5;
		runtime->Type_pool = realloc(runtime->Type_pool, sizeof(YType*) * runtime->Type_pool_capacity);
	}
	YType* type = calloc(1, sizeof(YType));
	runtime->Type_pool[runtime->Type_pool_size++] = type;
	Type_init(type, yoyo_thread(runtime));
	return type;
}

wchar_t* toString(YValue* v, YThread* th) {
	int32_t id = getSymbolId(&th->runtime->symbols,
	TO_STRING);
	if (v->type == &th->runtime->ObjectType) {
		YObject* obj = (YObject*) v;
		if (obj->contains(obj, id, th)) {
			YValue* val = obj->get(obj, id, th);
			if (val->type == &th->runtime->LambdaType) {
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
