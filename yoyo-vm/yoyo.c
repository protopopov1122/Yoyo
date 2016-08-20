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

#define STR_VALUE(arg)      #arg
#define TOWCS(name) L""STR_VALUE(name)
// Configuration in compile-time
#ifdef YSTD 
#define YSTD_PATH TOWCS(YSTD)
#else
#define YSTD_PATH L"."
#endif

#ifdef OBJECTS
#define OBJ_TYPE TOWCS(OBJECTS)
#else
#define OBJ_TYPE L"hash"
#endif 

void Signal_handler(int sig) {
	printf("Yoyo aborting. Please report bug at https://github.com/protopopov1122/Yoyo\n");
	switch (sig) {
		case SIGSEGV:
			printf("Segmentation fault");
		break;
		case SIGFPE:
			printf("Floating-point exception");
		break;
		default:
			printf("Fatal error");
		break;
	}
	#ifdef OS_UNIX
	printf(". Backtrace:\n");
	void * buffer[255];
	const int calls = backtrace(buffer,
		sizeof(buffer) / sizeof(void *));
	backtrace_symbols_fd(buffer, calls, 1);
	#elif defined(OS_WIN)
	printf(". On Windows backtrace isn't available.\n");
	#else
	printf(".\n");
	#endif

	exit(EXIT_FAILURE);
}

/*
 * Search file in current runtime path, translate it and
 * execute bytecode by current runtime. Return boolean
 * indicating if translation was sucessful.
 * */
bool Yoyo_interpret_file(ILBytecode* bc, YRuntime* runtime, wchar_t* wpath) {
	Environment* env = runtime->env;
	FILE* file = env->getFile(env, wpath);
	if (file == NULL) {
		yerror(ErrorFileNotFound, wpath, yoyo_thread(runtime));
		return false;
	} else {
		YThread* th = yoyo_thread(runtime);
		CompilationResult res = yoyoc((YoyoCEnvironment*) env,
			file_input_stream(env->getFile(env, wpath)), wpath);
		if (res.pid != -1) {
			invoke(res.pid, bc, runtime->global_scope, NULL, th);
			ILProcedure* proc = bc->procedures[res.pid];
			proc->free(proc, bc);
			if (th->exception != NULL) {
				YValue* e = th->exception;
				th->exception = NULL;
				wchar_t* wstr = toString(e, th);
				fprintf(th->runtime->env->out_stream, "%ls\n", wstr);
				if (e->type == &th->runtime->ObjectType) {
					YObject* obj = (YObject*) e;
					if (OBJECT_HAS(obj, L"trace", th)) {
						YValue* trace = OBJECT_GET(obj, L"trace", th);
						wchar_t* wcs = toString(trace, th);
						fprintf(runtime->env->out_stream, "%ls\n", wcs);
						free(wcs);
					}
				}
				free(wstr);
				return false;
			}
			return true;
		}
		fprintf(runtime->env->out_stream, "%ls\n", res.log);
		free(res.log);
	}
	return false;
}

/*
 * Create runtime and compilation environment and set up it
 * according to arguments. Executes 'core.yoyo' from standart library
 * to set up minimal runtime environment and file specified in arguments.
 * */
void Yoyo_main(char** argv, int argc) {
	setlocale(LC_ALL, "");
	signal(SIGSEGV, Signal_handler);
	signal(SIGFPE, Signal_handler);

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
				else if (strcmp(arg, "help") == 0) {
					printf("Yoyo v"VERSION" - tiny dynamic language.\n"
								"View code examples in repository\n"
								"Use:\n\tyoyo [OPTIONS] FILE arg1 arg2 ...\n"
								"Options:\n"
								"\t--debug - execute in debug mode\n"
								"\t--help - display brief help\n"
								"\t--version - display project version\n"
								"\t-Ppath - add path to file search path\n"
								"\t-Dkey=value - define key\n"
								"Most useful keys:\n"
								"\tystd - specify standart library location\n"
								"\tobjects = hash/tree - specify used object implementation\n"
								"\tIntPool - specify integer pool size(Strongly affects on memory use and performance)\n"
								"\tIntCache - specify integer cache size(Strongly affects on memory use and performance)\n"
								"\tworkdir - specify working directory\n"
								"\nAuthor: Eugene Protopopov <protopopov1122@yandex.ru>\n"
								"License: Program, standart library and examples are distributed under the terms of GNU GPLv3 or any later version.\n"
								"Program repo: https://github.com/protopopov1122/Yoyo");
					exit(0);
				}
				else if (strcmp(arg, "version") == 0) {
					printf("v"VERSION"\n");
					exit(0);
				}
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
	char* mbs_wd = calloc(1, sizeof(wchar_t) * (wcslen(workdir) + 1));
	wcstombs(mbs_wd, workdir, sizeof(wchar_t) * wcslen(workdir));
	chdir(mbs_wd);
	free(mbs_wd);
	env->addPath(env, workdir);
	env->addPath(env, libdir == NULL ? YSTD_PATH : libdir);
	env->define(env, L"objects", OBJ_TYPE);
#ifdef GCGenerational
	if (env->getDefined(env, L"GCPlain")==NULL)
		env->define(env, L"GCGenerational", L"");
#endif

	JitCompiler* jit = NULL;
	if (env->getDefined(env, L"yjit")!=NULL) {
		wchar_t* wcs = env->getDefined(env, L"yjit");
		char* mbs = calloc(1, sizeof(wchar_t)*(wcslen(wcs)+1));
		wcstombs(mbs, wcs, sizeof(wchar_t) * wcslen(wcs));
		void* handle = dlopen(mbs, RTLD_NOW);
		if (handle!=NULL) {
			void* ptr = dlsym(handle, "getYoyoJit");
			if (ptr!=NULL) {
				JitGetter* getter_ptr = (JitGetter*) &ptr;
				JitGetter getter = *getter_ptr;
				jit = getter();
			}
			else
				fprintf(env->err_stream, "%s\n", dlerror());
		}
		else
			fprintf(env->err_stream, "%s\n", dlerror());
		free(mbs);
	}

	YRuntime* runtime = newRuntime(env, NULL);
	ycenv->bytecode = newBytecode(&runtime->symbols);
	if (dbg)
		debug = newDefaultDebugger(ycenv->bytecode);

	OBJECT_NEW(runtime->global_scope, L"sys",
			Yoyo_SystemObject(ycenv->bytecode, yoyo_thread(runtime)),
			yoyo_thread(runtime));

	/* Executes specified file only if 'core.yoyo' is found and valid */
	if (Yoyo_interpret_file(ycenv->bytecode, runtime, L"core.yoyo")) {
		runtime->debugger = debug;
        ycenv->jit = jit;
		if (file != NULL) {
			Yoyo_interpret_file(ycenv->bytecode, runtime, file);
			free(file);
		} else {
			Yoyo_interpret_file(ycenv->bytecode, runtime, L"repl.yoyo");
		}
	}

	/* Waits all threads to finish and frees resources */
	YThread* current_th = yoyo_thread(runtime);
	current_th->free(current_th);
	runtime->wait(runtime);
	runtime->free(runtime);
	if (debug != NULL)
		debug->free(debug);
	if (jit!=NULL)
		jit->free(jit);
}
