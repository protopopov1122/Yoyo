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

#ifndef YOYO_RUNTIME_YOYO_RUNTIME_H
#define YOYO_RUNTIME_YOYO_RUNTIME_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include <math.h>
#include <locale.h>
#include <limits.h>
#include "util.h"
#include "external.h"

typedef struct GarbageCollector GarbageCollector;
typedef struct YType YType;
typedef struct YoyoType YoyoType;
typedef struct YDebug YDebug;
typedef struct YRuntime YRuntime;
typedef struct ILBytecode ILBytecode;
typedef struct YThread YThread;
typedef struct YValue YValue;
typedef struct YInteger YInteger;
typedef struct YFloat YFloat;
typedef struct YBoolean YBoolean;
typedef struct YString YString;
typedef struct YObject YObject;
typedef struct YArray YArray;
typedef struct YLambda YLambda;
typedef struct YoyoIterator YoyoIterator;
typedef struct YoyoMap YoyoMap;
typedef struct YoyoSet YoyoSet;


typedef struct SymbolMapEntry {
	int32_t id;
	wchar_t* symbol;
} SymbolMapEntry;

typedef struct SymbolMap {
	SymbolMapEntry* map;
	size_t size;
	MUTEX mutex;
} SymbolMap;

int32_t getSymbolId(SymbolMap*, wchar_t*);
wchar_t* getSymbolById(SymbolMap*, int32_t);

FILE* search_file(wchar_t*, wchar_t**, size_t);

#endif
