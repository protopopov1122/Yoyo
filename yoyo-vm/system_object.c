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

#include "yoyo.h"

/*Minimal object to access Yoyo system API
 * from code. Minimal API to call compiler, eval code,
 * load shared libraries and call C functions and
 * other system stuff. All other Yoyo API
 * located in YStd/native and called by
 * Yoyo code using functions above.*/

typedef struct YArgs {
	YArray array;

	wchar_t** argv;
	size_t argc;
} YArgs;

size_t YArgs_size(YArray* arr, YThread* th) {
	YArgs* args = (YArgs*) arr;
	return args->argc;
}
YValue* YArgs_get(YArray* arr, size_t index, YThread* th) {
	YArgs* args = (YArgs*) arr;
	if (index < args->argc)
		return newString(args->argv[index], th);
	else
		return getNull(th);
}

YOYO_FUNCTION(YSTD_SYSTEM_ARGS) {
	YArgs* yargs = calloc(1, sizeof(YArgs));
	initAtomicYoyoObject((YoyoObject*) yargs, (void (*)(YoyoObject*)) free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) yargs);
	yargs->array.parent.type = &th->runtime->ArrayType;

	yargs->array.size = YArgs_size;
	yargs->array.get = YArgs_get;
	yargs->array.toString = NULL;
	yargs->argc = th->runtime->env->argc;
	yargs->argv = th->runtime->env->argv;

	return (YValue*) newTuple((YArray*) yargs, th);
}
typedef YValue* (*Yoyo_initYJI)(YRuntime*);
YOYO_FUNCTION(YSTD_SYSTEM_YJI) {
	wchar_t* path = th->runtime->env->getDefined(th->runtime->env, L"libyji");
	if (path == NULL)
		return getNull(th);
	char* cpath = calloc(1, sizeof(wchar_t) * wcslen(path) + 1);
	wcstombs(cpath, path, wcslen(path));
	void* h = dlopen(cpath, RTLD_LAZY | RTLD_GLOBAL);
	free(cpath);
	void* ptr = dlsym(h, "Yoyo_initYJI");
	Yoyo_initYJI *fun_ptr = (Yoyo_initYJI*) &ptr;
	Yoyo_initYJI init = *fun_ptr;
	if (init != NULL)
		return init(th->runtime);
	else
		return getNull(th);
}
YOYO_FUNCTION(YSTD_SYSTEM_EXIT) {
	exit(0);
	return getNull(th);
}
YOYO_FUNCTION(YSTD_SYSTEM_GC_BLOCK) {
	th->runtime->gc->block = true;
	return getNull(th);
}
YOYO_FUNCTION(YSTD_SYSTEM_GC_UNBLOCK) {
	th->runtime->gc->block = false;
	return getNull(th);
}


YOYO_FUNCTION(YSTD_SYSTEM_LOAD_LIBRARY) {
	YRuntime* runtime = th->runtime;
	wchar_t* wstr = toString(args[0], th);
	YObject* lib = th->runtime->newObject(runtime->global_scope, th);
	runtime->env->eval(runtime->env, th->runtime,
			file_input_stream(runtime->env->getFile(runtime->env, wstr)), wstr,
			lib);
	free(wstr);
	return (YValue*) lib;
}
YOYO_FUNCTION(YSTD_SYSTEM_LOAD) {
	YRuntime* runtime = th->runtime;
	wchar_t* wstr = toString(args[0], th);
	YValue* out = runtime->env->eval(runtime->env, th->runtime,
			file_input_stream(runtime->env->getFile(runtime->env, wstr)), wstr,
			(YObject*) ((ExecutionFrame*) th->frame)->regs[0]);
	free(wstr);
	return out;
}
YOYO_FUNCTION(YSTD_SYSTEM_IMPORT) {
	YObject* sys = (YObject*) ((NativeLambda*) lambda)->object;
	YRuntime* runtime = th->runtime;
	wchar_t* wstr = toString(args[0], th);
	int32_t iid = getSymbolId(&runtime->symbols, L"imported");
	YObject* loaded = (YObject*) sys->get(sys, iid, th);
	int32_t id = getSymbolId(&runtime->symbols, wstr);
	if (loaded->contains(loaded, id, th)) {
		free(wstr);
		return loaded->get(loaded, id, th);
	}
	YObject* lib = th->runtime->newObject(runtime->global_scope, th);
	runtime->env->eval(runtime->env, th->runtime,
			file_input_stream(runtime->env->getFile(runtime->env, wstr)), wstr,
			lib);
	free(wstr);
	return (YValue*) lib;
	free(wstr);
	loaded->put(loaded, id, (YValue*) lib, true, th);
	return (YValue*) lib;
}
YOYO_FUNCTION(YSTD_SYSTEM_EVAL) {
	YRuntime* runtime = th->runtime;
	wchar_t* wstr = toString(args[0], th);
	YValue* out = runtime->env->eval(runtime->env, runtime,
			string_input_stream(wstr), wstr,
			(YObject*) ((ExecutionFrame*) th->frame)->regs[0]);
	free(wstr);
	return out;
	return getNull(th);
}

YOYO_FUNCTION(YSTD_SYSTEM_NATIVE) {
	wchar_t* wstr = toString(args[0], th);
	char* cstr = malloc(sizeof(wchar_t) * wcslen(wstr) + 1);
	wcstombs(cstr, wstr, wcslen(wstr));
	cstr[wcslen(wstr)] = '\0';
	void* handle = dlopen(NULL, RTLD_LAZY);
	YCallable clb;
	*(void**) (&clb) = dlsym(NULL, cstr);
	dlclose(handle);
	free(cstr);
	if (clb == NULL) {
		throwException(L"UnknownFunction", &wstr, 1, th);
		free(wstr);
		return getNull(th);
	}
	free(wstr);
	YoyoLambdaSignature* sig = newLambdaSignature(false, -1, false, NULL, NULL,
			th);
	if (args[1]->type == &th->runtime->DeclarationType) {
		YoyoType* type = (YoyoType*) args[1];
		if (type->type == LambdaSignatureDT) {
			sig = (YoyoLambdaSignature*) type;
		}
	}
	NativeLambda* lmbd = (NativeLambda*) newNativeLambda(-1, clb,
			(YoyoObject*) args[2], th);
	lmbd->lambda.sig = sig;
	return (YValue*) lmbd;
}

YOYO_FUNCTION(YSTD_SYSTEM_SHARED_LIBRARY) {
	wchar_t* wstr = toString(args[0], th);
	char* cstr = malloc(sizeof(wchar_t) * wcslen(wstr) + 1);
	wcstombs(cstr, wstr, wcslen(wstr));
	cstr[wcslen(wstr)] = '\0';
	void* handle = dlopen(cstr, RTLD_LAZY | RTLD_GLOBAL);
	free(cstr);
	if (handle == NULL)
		throwException(L"UnknownLibrary", &wstr, 1, th);
	free(wstr);
	return getNull(th);
}


YObject* Yoyo_SystemObject(ILBytecode* bc, YThread* th) {
	YObject* sys = OBJECT(NULL, th);
	OBJECT_NEW(sys, L"bytecode", newRawPointer(bc, free, th), th);
	OBJECT_NEW(sys, L"imported", OBJECT(NULL, th), th);

	METHOD(sys, L"eval", YSTD_SYSTEM_EVAL, 1, th);
	METHOD(sys, L"load", YSTD_SYSTEM_LOAD, 1, th);
	METHOD(sys, L"loadLibrary", YSTD_SYSTEM_LOAD_LIBRARY, 1, th);
	METHOD(sys, L"import", YSTD_SYSTEM_IMPORT, 1, th);
	METHOD(sys, L"yji", YSTD_SYSTEM_YJI, 0, th);
	METHOD(sys, L"exit", YSTD_SYSTEM_EXIT, 0, th);
	METHOD(sys, L"native", YSTD_SYSTEM_NATIVE, 3, th);
	METHOD(sys, L"sharedLibrary", YSTD_SYSTEM_SHARED_LIBRARY, 1, th);
	METHOD(sys, L"args", YSTD_SYSTEM_ARGS, 0, th);
	OBJECT_NEW(sys, L"platform", newString(PLATFORM, th), th);

	return sys;
}
