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
void YArgs_set(YArray* arr, size_t index, YValue* v, YThread* th) {
	return;
}
void YArgs_insert(YArray* arr, size_t index, YValue* v, YThread* th) {
	return;
}
void YArgs_add(YArray* arr, YValue* v, YThread* th) {
	return;
}
void YArgs_remove(YArray* arr, size_t index, YThread* th) {
	return;
}

YOYO_FUNCTION(YSTD_SYSTEM_ARGS) {
	YArgs* yargs = malloc(sizeof(YArgs));
	initAtomicYoyoObject((YoyoObject*) yargs, (void (*)(YoyoObject*)) free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) yargs);
	yargs->array.parent.type = &th->runtime->ArrayType;

	yargs->array.size = YArgs_size;
	yargs->array.get = YArgs_get;
	yargs->array.set = YArgs_set;
	yargs->array.add = YArgs_add;
	yargs->array.insert = YArgs_insert;
	yargs->array.remove = YArgs_remove;
	yargs->array.toString = NULL;
	yargs->argc = th->runtime->env->argc;
	yargs->argv = th->runtime->env->argv;

	return (YValue*) yargs;
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

YOYO_FUNCTION(YSTD_SYSTEM_LOAD_LIBRARY) {
	YRuntime* runtime = th->runtime;
	wchar_t* wstr = toString(args[0], th);
	CompilationResult res = runtime->env->parse(runtime->env, runtime, wstr);
	YValue* out = getNull(th);
	if (res.pid != -1) {
		YObject* libScope = th->runtime->newObject(th->runtime->global_scope,
				th);
		invoke(res.pid, libScope, NULL, th);
		ILProcedure* proc = th->runtime->bytecode->procedures[res.pid];
		proc->free(proc, th->runtime->bytecode);
		out = (YValue*) libScope;
	} else {
		throwException(L"LoadModule", &wstr, 1, th);
		if (th->exception->type->type == ObjectT && res.log != NULL) {
			YObject* obj = (YObject*) th->exception;
			obj->put(obj,
					th->runtime->bytecode->getSymbolId(th->runtime->bytecode,
							L"log"), newString(res.log, th), true, th);
		}
	}
	free(res.log);
	free(wstr);
	return out;
}
YOYO_FUNCTION(YSTD_SYSTEM_LOAD) {
	YRuntime* runtime = th->runtime;
	wchar_t* wstr = toString(args[0], th);
	CompilationResult res = runtime->env->parse(runtime->env, runtime, wstr);
	YValue* out = getNull(th);
	if (res.pid != -1) {
		out = invoke(res.pid, (YObject*) th->frame->regs[0], NULL, th);
		ILProcedure* proc = th->runtime->bytecode->procedures[res.pid];
		proc->free(proc, th->runtime->bytecode);
	} else {
		throwException(L"Load", &wstr, 1, th);
		if (th->exception->type->type == ObjectT && res.log != NULL) {
			YObject* obj = (YObject*) th->exception;
			obj->put(obj,
					th->runtime->bytecode->getSymbolId(th->runtime->bytecode,
							L"log"), newString(res.log, th), true, th);
		}
	}
	free(res.log);
	free(wstr);
	return out;
}
YOYO_FUNCTION(YSTD_SYSTEM_IMPORT) {
	YRuntime* runtime = th->runtime;
	wchar_t* wstr = toString(args[0], th);
	YObject* sys = (YObject*) ((NativeLambda*) lambda)->object;
	int32_t iid = th->runtime->bytecode->getSymbolId(th->runtime->bytecode, L"imported");
	YObject* loaded = (YObject*) sys->get(sys, iid, th);
	int32_t id = runtime->bytecode->getSymbolId(runtime->bytecode, wstr);
	if (loaded->contains(loaded, id, th))
	{
		free(wstr);
		return loaded->get(loaded, id, th);
	}
	CompilationResult res = runtime->env->parse(runtime->env, runtime, wstr);
	YValue* out = getNull(th);
	if (res.pid != -1) {
		YObject* libScope = th->runtime->newObject(th->runtime->global_scope,
				th);
		invoke(res.pid, libScope, NULL, th);
		ILProcedure* proc = th->runtime->bytecode->procedures[res.pid];
		proc->free(proc, th->runtime->bytecode);
		out = (YValue*) libScope;
	} else {
		throwException(L"LoadModule", &wstr, 1, th);
		if (th->exception->type->type == ObjectT && res.log != NULL) {
			YObject* obj = (YObject*) th->exception;
			obj->put(obj,
					th->runtime->bytecode->getSymbolId(th->runtime->bytecode,
							L"log"), newString(res.log, th), true, th);
		}
	}
	free(res.log);
	free(wstr);
	loaded->put(loaded, id, out, true, th);
	return out;
}
YOYO_FUNCTION(YSTD_SYSTEM_EVAL) {
	YRuntime* runtime = th->runtime;
	wchar_t* wstr = toString(args[0], th);
	CompilationResult res = runtime->env->eval(runtime->env, runtime, wstr);
	YValue* out = getNull(th);
	if (res.pid != -1) {
		out = invoke(res.pid, (YObject*) th->frame->regs[0], NULL, th);
		ILProcedure* proc = th->runtime->bytecode->procedures[res.pid];
		proc->free(proc, th->runtime->bytecode);
	} else {
		throwException(L"Eval", &wstr, 1, th);
		if (th->exception->type->type == ObjectT && res.log != NULL) {
			YObject* obj = (YObject*) th->exception;
			obj->put(obj,
					th->runtime->bytecode->getSymbolId(th->runtime->bytecode,
							L"log"), newString(res.log, th), true, th);
		}
	}
	free(res.log);
	free(wstr);
	return out;
}

YOYO_FUNCTION(YSTD_SYSTEM_NATIVE) {
	YoyoLambdaSignature* sig = newLambdaSignature(-1, false, NULL, NULL, th);
	if (args[1]->type->type==DeclarationT) {
		YoyoType* type = (YoyoType*) args[1];
		if (type->type==LambdaSignatureDT) {
			sig = (YoyoLambdaSignature*) type;
		}
	}
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

YObject* Yoyo_SystemObject(YThread* th) {
	YObject* sys = th->runtime->newObject(NULL, th);
	sys->put(sys, th->runtime->bytecode->getSymbolId(th->runtime->bytecode, L"imported")
			,(YValue*)th->runtime->newObject(NULL, th), true, th);

	ADD_METHOD(sys, L"eval", YSTD_SYSTEM_EVAL, 1, th);
	ADD_METHOD(sys, L"load", YSTD_SYSTEM_LOAD, 1, th);
	ADD_METHOD(sys, L"loadLibrary", YSTD_SYSTEM_LOAD_LIBRARY, 1, th);
	ADD_METHOD(sys, L"import", YSTD_SYSTEM_IMPORT, 1, th);
	ADD_METHOD(sys, L"yji", YSTD_SYSTEM_YJI, 0, th);
	ADD_METHOD(sys, L"exit", YSTD_SYSTEM_EXIT, 0, th);
	ADD_METHOD(sys, L"native", YSTD_SYSTEM_NATIVE, 3, th);
	ADD_METHOD(sys, L"sharedLibrary", YSTD_SYSTEM_SHARED_LIBRARY, 1, th);
	ADD_METHOD(sys, L"args", YSTD_SYSTEM_ARGS, 0, th);
	ADD_FIELD(sys, L"platform", newString(PLATFORM, th), th);

	return sys;
}
