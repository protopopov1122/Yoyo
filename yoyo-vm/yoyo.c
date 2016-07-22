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

static YRuntime* RUNTIME = NULL;

/*
 * Search file in current runtime path, translate it and
 * execute bytecode by current runtime. Return boolean
 * indicating if translation was sucessful.
 * */
bool Yoyo_interpret_file(ILBytecode* bc, YRuntime* runtime, wchar_t* wpath) {
	Environment* env = runtime->env;
	FILE* file = env->getFile(env, wpath);
	if (file == NULL) {
		yerror(ErrorFileNotFound, wpath, runtime->CoreThread);
		return false;
	} else {
		YThread* th = newThread(runtime);
		runtime->env->eval(runtime->env, runtime,
			file_input_stream(runtime->env->getFile(runtime->env, wpath)), wpath,
			NULL); 
		if (th->exception!=NULL) {
			YValue* e = th->exception;
			th->exception = NULL;
			wchar_t* wstr = toString(e, th);
			fprintf(th->runtime->env->out_stream, "%ls\n", wstr);
			free(wstr);
		}
	}
	return true;
}

/*
 * Create runtime and compilation environment and set up it
 * according to arguments. Executes 'core.yoyo' from standart library
 * to set up minimal runtime environment and file specified in arguments.
 * */
void Yoyo_main(char** argv, int argc) {
	setlocale(LC_ALL, "");
	YoyoCEnvironment* ycenv = newYoyoCEnvironment(NULL);
	Environment* env = (Environment*) ycenv;
	YDebug* debug = NULL;
	bool dbg = false;

	env->argv = NULL;
	env->argc = 0;
	wchar_t* file = NULL;

	for (size_t i = 1; i < argc; i++) {
		char* arg = argv[i];
		if (arg[0] == '-' && strlen(arg) > 1) {
			arg++;
			if (arg[0] == '-') {
				/* Arguments like --... (e.g. --debug) */
				arg++;
				if (strcmp(arg, "debug") == 0)
					dbg = true;
			} else if (arg[0] == 'D') {
				/* Environment variable definitions.
				 * Format: -Dkey=value */
				arg++;
				char* key = NULL;
				char* value = NULL;
				size_t key_len = 0;
				size_t value_len = 0;
				while (*arg != '=' && *arg != '\0') {
					key = realloc(key, sizeof(char) * (++key_len));
					key[key_len - 1] = *(arg++);
				}
				key = realloc(key, sizeof(char) * (++key_len));
				key[key_len - 1] = '\0';
				while (*arg != '\0') {
					arg++;
					value = realloc(value, sizeof(char) * (++value_len));
					value[value_len - 1] = *arg;
				}
				wchar_t* wkey = calloc(1, sizeof(wchar_t) * (strlen(key) + 1));
				wchar_t* wvalue = calloc(1,
						sizeof(wchar_t)
								* (strlen(value != NULL ? value : "none") + 1));
				mbstowcs(wkey, key, strlen(key));
				mbstowcs(wvalue, value != NULL ? value : "none",
						strlen(value != NULL ? value : "none"));
				env->define(env, wkey, wvalue);
				free(wkey);
				free(wvalue);
				free(key);
				free(value);
			} else if (arg[0] == 'P') {
				/*Runtime path definitions.
				 * Format: -Ppath*/
				arg++;
				wchar_t* wpath = malloc(sizeof(wchar_t) * (strlen(arg) + 1));
				mbstowcs(wpath, arg, strlen(arg));
				wpath[strlen(arg)] = L'\0';
				env->addPath(env, wpath);
				free(wpath);
			}
		} else {
			/* If file to execute is not defined, then current
			 * argument is file. Else it is an argument to
			 * yoyo program. */
			if (file == NULL) {
				file = malloc(sizeof(wchar_t) * (strlen(arg) + 1));
				mbstowcs(file, arg, strlen(arg));
				file[strlen(arg)] = L'\0';
			} else {
				wchar_t* warg = malloc(sizeof(wchar_t) * (strlen(arg) + 1));
				mbstowcs(warg, arg, strlen(arg));
				warg[strlen(arg)] = L'\0';
				env->argc++;
				env->argv = realloc(env->argv, sizeof(wchar_t*) * env->argc);
				env->argv[env->argc - 1] = warg;
			}
		}
	}
	/* Adds minimal path: working directory(by default is '.') and
	 * standart library directory(by default is working directory). */
	wchar_t* workdir = env->getDefined(env, L"workdir");
	wchar_t* libdir = env->getDefined(env, L"ystd");
	workdir = workdir == NULL ? L"." : workdir;
	char* mbs_wd = malloc(sizeof(wchar_t) * (wcslen(workdir) + 1));
	wcstombs(mbs_wd, workdir, wcslen(workdir));
	chdir(mbs_wd);
	free(mbs_wd);
	env->addPath(env, workdir);
	env->addPath(env, libdir == NULL ? workdir : libdir);

	YRuntime* runtime = newRuntime(env, NULL);
	ycenv->bytecode = newBytecode(&runtime->symbols);
	if (dbg)
		debug = newDefaultDebugger(ycenv->bytecode);
	RUNTIME = runtime;

	OBJECT_NEW(runtime->global_scope, L"sys", Yoyo_SystemObject(ycenv->bytecode, runtime->CoreThread),
		runtime->CoreThread);

	/* Executes specified file only if 'core.yoyo' is found and valid */
	if (true) {//Yoyo_interpret_file(ycenv->bytecode, runtime, L"core.yoyo")) {
		runtime->debugger = debug;
		Yoyo_interpret_file(ycenv->bytecode, runtime, file);
	}

	/* Waits all threads to finish and frees resources */
	YThread* current_th = newThread(runtime);
	current_th->free(current_th);
	runtime->wait(runtime);
	runtime->free(runtime);
	if (debug != NULL)
		debug->free(debug);

}
