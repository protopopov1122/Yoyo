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

#include "yoyoc/yoyoc.h"

#include "stringbuilder.h"
#include "value.h"
#include "yoyoc/token.h"
#include "yoyoc/yparse.h"
#include "yoyoc/codegen.h"

typedef struct StringInputStream {
	InputStream is;
	wchar_t* wstr;
	size_t offset;
} StringInputStream;

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
	YThread* th = runtime->CoreThread;
	YObject* obj = th->runtime->newObject(NULL, th);
	return obj;
}
CompilationResult YoyoC_parse(Environment* _env, YRuntime* runtime,
		wchar_t* wpath) {
	YoyoCEnvironment* env = (YoyoCEnvironment*) _env;
	if (env->bytecode == NULL) {
		CompilationResult res = { .pid = -1, .log = NULL };
		return res;
	}
	return yoyoc(env, yfileinput(_env->getFile(_env, wpath)), wpath);
}
wint_t SIS_get(InputStream* is) {
	StringInputStream* sis = (StringInputStream*) is;
	if (sis->offset < wcslen(sis->wstr))
		return (wint_t) sis->wstr[sis->offset++];
	return WEOF;
}
void SIS_unget(InputStream* is, wint_t i) {
	StringInputStream* sis = (StringInputStream*) is;
	if (i != WEOF && sis->offset > 0)
		sis->offset--;
}
void SIS_close(InputStream* is) {
	StringInputStream* sis = (StringInputStream*) is;
	free(sis->wstr);
	free(sis);
}
CompilationResult YoyoC_eval(Environment* _env, YRuntime* runtime,
		wchar_t* wstr) {
	YoyoCEnvironment* env = (YoyoCEnvironment*) _env;
	if (env->bytecode == NULL) {
		CompilationResult res = { .pid = -1, .log = NULL };
		return res;
	}
	StringInputStream* is = malloc(sizeof(StringInputStream));
	wchar_t* cp = malloc(sizeof(wchar_t) * (wcslen(wstr) + 1));
	wcscpy(cp, wstr);
	cp[wcslen(wstr)] = L'\0';
	is->wstr = cp;
	is->offset = 0;
	is->is.get = SIS_get;
	is->is.unget = SIS_unget;
	is->is.close = SIS_close;
	return yoyoc(env, (InputStream*) is, L"<eval>");
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
	env->env.parse = YoyoC_parse;
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

CompilationResult yoyoc(YoyoCEnvironment* env, InputStream* is, wchar_t* name) {
	YParser parser;
	FILE* errfile = tmpfile();
	parser.err_file = errfile!=NULL ? errfile : stdout;
	yparse(env, ylex(env, is, name), &parser);
	if (parser.err_flag || parser.root == NULL) {
		if (parser.root != NULL)
			parser.root->free(parser.root);
		fflush(parser.err_file);
		rewind(parser.err_file);
		char* buffer = NULL;
		size_t len = 0;
		int ch;
		while ((ch = fgetc(parser.err_file)) != EOF) {
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
		if (errfile!=NULL)
			fclose(errfile);

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
	int32_t pid = ycompile(env, parser.root, parser.err_file);
	fflush(parser.err_file);
	wchar_t* wlog = NULL;
	if (ftell(parser.err_file) != 0) {
		rewind(parser.err_file);
		size_t len = 0;
		char* log = NULL;
		int ch;
		while ((ch = fgetc(parser.err_file)) != EOF) {
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
	if (errfile!=NULL)
		fclose(errfile);
	CompilationResult res = { .log = wlog, .pid = pid };
	return res;
}
