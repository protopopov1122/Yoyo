/*
 * Copyright (C) 2016  Jevgenijs Protopopovs <protopopov1122@yandex.ru>
 */
/*This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 3 as published by
 the Free Software Foundation.


 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#ifndef YOYOC_YOYOC_H
#define YOYOC_YOYOC_H

#include "yoyo-runtime.h"
#include "interpreter.h"
#include "jit.h"
#include "analyze.h"

typedef struct EnvEntry {
	wchar_t* key;
	wchar_t* value;
} EnvEntry;

typedef struct YoyoCEnvironment {
	Environment env;
	ILBytecode* bytecode;
	JitCompiler* jit;

	EnvEntry* envvars;
	size_t envvars_size;
	wchar_t** PATH;
	size_t PATH_size;
	wchar_t** files;
	size_t files_size;

	bool preprocess_bytecode;
	bool analyze_bytecode;
} YoyoCEnvironment;

YoyoCEnvironment* newYoyoCEnvironment(ILBytecode*);
CompilationResult yoyoc(YoyoCEnvironment*, InputStream*, wchar_t*);

#endif
