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

#ifndef YILI_YILI_H
#define YILI_YILI_H

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

#ifndef __cplusplus
#define YOYO_FUNCTION(name) YValue* name(YLambda* lambda, YObject* scope,\
	YValue** args, size_t argc, YThread* th)
#else
#define YOYO_FUNCTION(name) extern "C" YValue* name(YLambda* lambda,\
	YValue** args, size_t argc, YThread* th)
#endif
#define FUN_OBJECT (((NativeLambda*) lambda)->object)
#define YOYOID(wstr, th) getSymbolId(&th->runtime->symbols, wstr)
#define YOYOSYM(id, th) getSymbolById(&th->runtime->symbols, id)

#define TYPE(value, t) value->type==t
#define OBJECT_GET(obj, id, th) obj->get(obj, YOYOID(id, th), th)
#define OBJECT_PUT(obj, id, value, th) obj->put(obj, YOYOID(id, th),\
										(YValue*) value, false, th)
#define OBJECT_NEW(obj, id, value, th) obj->put(obj, YOYOID(id, th),\
										(YValue*) value, true, th)
#define OBJECT_HAS(obj, id, th) obj->contains(obj, YOYOID(id, th), th)
#define OBJECT(super, th) th->runtime->newObject(super, th)

#define LAMBDA(fn, argc, ptr, th) newNativeLambda(argc, fn, (YoyoObject*) ptr, th)

#define METHOD(obj, name, fn, argc, th) OBJECT_NEW(obj, name,\
													LAMBDA(fn, argc, \
															obj, th),\
                                                    th)
#define CAST_INTEGER(name, value) int64_t name = getInteger((YValue*) value);
#define CAST_FLOAT(name, value) double name = getFloat((YValue*) value);
#define CAST_BOOLEAN(name, val) bool name = ((YBoolean*) val)->value;
#define CAST_STRING(name, val) wchar_t* name = ((YString*) val)->value;
#define CAST_OBJECT(name, val) YObject* name = (YObject*) val;
#define CAST_ARRAY(name, val) YArray* name = (YArray*) val;
#define CAST_LAMBDA(name, val) YLambda* name = (YLambda*) val;

#endif
