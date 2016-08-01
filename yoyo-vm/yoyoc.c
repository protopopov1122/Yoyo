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

#include "yoyoc.h"
#include "yoyo-runtime.h"
#include "parser.h"
#include "codegen.h"

void YoyoC_free(Environment* _env, YRuntime* runtime) {
	YoyoCEnvironment* env = (YoyoCEnvironment*) _env;
	for (size_t i = 0; i < env->envvars_size; i++) {
		free(env->envvars[i].key);
		free(env->envvars[i].value);
	}
	free(env->envvars);
	for (size_t i = 0; i < _env->argc; i++)
		free(_env->argv[i]);
	free(_env->argv);
	for (size_t i = 0; i < env->files_size; i++)
		free(env->files[i]);
	free(env->files);
	free(env);
}
YObject* YoyoC_system(Environment* env, YRuntime* runtime) {
	YThread* th = yoyo_thread(runtime);
	YObject* obj = th->runtime->newObject(NULL, th);
	return obj;
}

YValue* YoyoC_eval(Environment* _env, YRuntime* runtime, InputStream* is,
		wchar_t* wname, YObject* scope) {
	YoyoCEnvironment* env = (YoyoCEnvironment*) _env;
	if (env->bytecode == NULL) {
		env->bytecode = newBytecode(&runtime->symbols);
	}
	CompilationResult res = yoyoc(env, is, wname);
	YThread* th = yoyo_thread(runtime);
	if (res.pid != -1) {
		if (scope == NULL)
			scope = runtime->global_scope;
		YValue* out = invoke(res.pid, env->bytecode, scope, NULL, th);
		ILProcedure* proc = env->bytecode->procedures[res.pid];
		proc->free(proc, env->bytecode);
		return out;
	} else if (scope != NULL) {
		throwException(L"Eval", &wname, 1, th);
		if (th->exception->type->type == ObjectT && res.log != NULL) {
			YObject* obj = (YObject*) th->exception;
			obj->put(obj, getSymbolId(&th->runtime->symbols, L"log"),
					newString(res.log, th), true, th);
		}
		return getNull(th);
	} else {
		fprintf(runtime->env->err_stream, "%ls\n", res.log);
		return getNull(th);
	}
}
wchar_t* YoyoC_getenv(Environment* _env, wchar_t* wkey) {
	YoyoCEnvironment* env = (YoyoCEnvironment*) _env;
	for (size_t i = 0; i < env->envvars_size; i++)
		if (wcscmp(env->envvars[i].key, wkey) == 0)
			return env->envvars[i].value;
	return NULL;
}
void YoyoC_putenv(Environment* _env, wchar_t* key, wchar_t* value) {
	YoyoCEnvironment* env = (YoyoCEnvironment*) _env;
	for (size_t i = 0; i < env->envvars_size; i++)
		if (wcscmp(env->envvars[i].key, key) == 0) {
			free(env->envvars[i].value);
			env->envvars[i].value = malloc(
					sizeof(wchar_t) * (wcslen(value) + 1));
			wcscpy(env->envvars[i].value, value);
			env->envvars[i].value[wcslen(value)] = L'\0';
		}
	env->envvars_size++;
	env->envvars = realloc(env->envvars, sizeof(EnvEntry) * env->envvars_size);
	env->envvars[env->envvars_size - 1].key = malloc(
			sizeof(wchar_t) * (wcslen(key) + 1));
	wcscpy(env->envvars[env->envvars_size - 1].key, key);
	env->envvars[env->envvars_size - 1].key[wcslen(key)] = L'\0';
	env->envvars[env->envvars_size - 1].value = malloc(
			sizeof(wchar_t) * (wcslen(value) + 1));
	wcscpy(env->envvars[env->envvars_size - 1].value, value);
	env->envvars[env->envvars_size - 1].value[wcslen(value)] = L'\0';
}
void YoyoC_addpath(Environment* _env, wchar_t* wpath) {
	YoyoCEnvironment* env = (YoyoCEnvironment*) _env;
	env->PATH_size++;
	env->PATH = realloc(env->PATH, sizeof(wchar_t*) * env->PATH_size);
	env->PATH[env->PATH_size - 1] = malloc(
			sizeof(wchar_t) * (wcslen(wpath) + 1));
	wcscpy(env->PATH[env->PATH_size - 1], wpath);
	env->PATH[env->PATH_size - 1][wcslen(wpath)] = L'\0';
}
FILE* YoyoC_getfile(Environment* _env, wchar_t* wfile) {
	YoyoCEnvironment* env = (YoyoCEnvironment*) _env;
	return search_file(wfile, env->PATH, env->PATH_size);
}
wchar_t** YoyoC_getLoadedFiles(Environment* _env) {
	YoyoCEnvironment* env = (YoyoCEnvironment*) _env;
	return env->files;
}
YoyoCEnvironment* newYoyoCEnvironment(ILBytecode* bc) {
	YoyoCEnvironment* env = malloc(sizeof(YoyoCEnvironment));
	env->bytecode = bc;
	env->env.out_stream = stdout;
	env->env.in_stream = stdin;
	env->env.err_stream = stderr;
	env->env.free = YoyoC_free;
	env->env.system = YoyoC_system;
	env->env.eval = YoyoC_eval;
	env->env.getDefined = YoyoC_getenv;
	env->env.define = YoyoC_putenv;
	env->env.getFile = YoyoC_getfile;
	env->env.addPath = YoyoC_addpath;
	env->env.getLoadedFiles = YoyoC_getLoadedFiles;
	env->envvars = NULL;
	env->envvars_size = 0;
	env->PATH = NULL;
	env->PATH_size = 0;
	env->files = malloc(sizeof(wchar_t*));
	env->files[0] = NULL;
	env->files_size = 1;
	return env;
}

CompilationResult yoyoc(YoyoCEnvironment* env, InputStream* input,
		wchar_t* name) {
	ParseHandle handle;
	FILE* errfile = tmpfile();
	handle.error_stream = errfile;
	handle.input = input;
	handle.charPos = 0;
	handle.line = 1;
	handle.fileName = name;
	handle.constants = NULL;
	handle.constants_size = 0;
	handle.symbols = NULL;
	handle.symbols_size = 0;
	handle.error_flag = false;
	shift(&handle);
	shift(&handle);
	shift(&handle);
	shift(&handle);
	YNode* root = parse(&handle);
	input->close(input);
	if (handle.error_flag || root == NULL) {
		if (root != NULL)
			root->free(root);
		fflush(handle.error_stream);
		rewind(handle.error_stream);
		char* buffer = NULL;
		size_t len = 0;
		int ch;
		while ((ch = fgetc(handle.error_stream)) != EOF) {
			buffer = realloc(buffer, sizeof(char) * (++len));
			buffer[len - 1] = (char) ch;
		}
		buffer = realloc(buffer, sizeof(char) * (++len));
		buffer[len - 1] = '\0';
		while (isspace(buffer[strlen(buffer) - 1]))
			buffer[strlen(buffer) - 1] = '\0';
		CompilationResult res = { .log = calloc(1,
				sizeof(wchar_t) * (strlen(buffer) + 1)), .pid = -1 };
		mbstowcs(res.log, buffer, strlen(buffer));
		free(buffer);
		if (errfile != NULL)
			fclose(errfile);
		for (size_t i = 0; i < handle.constants_size; i++)
			if (handle.constants[i].type == WcsConstant)
				free(handle.constants[i].value.wcs);
		free(handle.constants);
		for (size_t i = 0; i < handle.symbols_size; i++)
			free(handle.symbols[i]);
		free(handle.symbols);
		return res;
	}
	bool cont = false;
	for (size_t i = 0; i < env->files_size; i++)
		if (env->files[i] != NULL && wcscmp(env->files[i], name) == 0) {
			cont = true;
			break;
		}
	if (!cont) {
		env->files = realloc(env->files,
				sizeof(wchar_t*) * (env->files_size + 1));
		env->files[env->files_size - 1] = calloc(1,
				sizeof(wchar_t) * (wcslen(name) + 1));
		wcscpy(env->files[env->files_size - 1], name);
		env->files[env->files_size] = NULL;
		env->files_size++;
	}
	int32_t pid = ycompile(env, root, handle.error_stream);
	fflush(handle.error_stream);
	wchar_t* wlog = NULL;
	if (ftell(handle.error_stream) != 0) {
		rewind(handle.error_stream);
		size_t len = 0;
		char* log = NULL;
		int ch;
		while ((ch = fgetc(handle.error_stream)) != EOF) {
			log = realloc(log, sizeof(char) * (++len));
			log[len - 1] = (char) ch;
		}
		log = realloc(log, sizeof(char) * (++len));
		log[len - 1] = '\0';
		while (isspace(log[strlen(log) - 1]))
			log[strlen(log) - 1] = '\0';
		wlog = calloc(1, sizeof(wchar_t) * (strlen(log) + 1));
		mbstowcs(wlog, log, strlen(log));
	}
	if (errfile != NULL)
		fclose(errfile);
	CompilationResult res = { .log = wlog, .pid = pid };
	free(handle.constants);
	for (size_t i = 0; i < handle.symbols_size; i++)
		free(handle.symbols[i]);
	free(handle.symbols);
	return res;
}
