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

#ifndef YOYO_RUNTIME_YOYO_H
#define YOYO_RUNTIME_YOYO_H

#include "array.h"
#include "core.h"
#include "debug.h"
#include "exceptions.h"
#include "memalloc.h"
#include "runtime.h"
#include "stringbuilder.h"
#include "value.h"
#include "wrappers.h"
#include "yerror.h"

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
#define OBJECT_REMOVE(obj, id, th) obj->remove(obj, YOYOID(id, th), th)
#define OBJECT(super, th) th->runtime->newObject(super, th)

#define LAMBDA(fn, argc, ptr, th) newNativeLambda(argc, fn, (YoyoObject*) ptr, th)

#define METHOD(obj, name, fn, argc, th) OBJECT_NEW(obj, name,\
													LAMBDA(fn, argc, \
															obj, th),\
                                                    th)
#define CAST_INTEGER(arg) (((YInteger*) arg)->value)
#define CAST_FLOAT(arg) (((YFloat*) arg)->value)
#define CAST_BOOLEAN(arg) (((YBoolean*) arg)->value)
#define CAST_STRING(arg) (((YString*) arg)->value)
#define CAST_LAMBDA(arg) ((YLambda*) arg)
#define CAST_OBJECT(arg) ((YObject*) arg)
#define CAST_ARRAY(arg) ((YArray*) arg)

#define ASSERT_TYPE(arg, tp, th) if (arg->type!=tp) {\
																if (arg->type==&th->runtime->NullType) {\
																	throwException(L"NullException", NULL, 0, th);\
																} else {\
																	wchar_t* wcs = toString(arg, th);\
																	throwException(L"WrongType", &wcs, 1, th);\
																	free(wcs);\
																}\
																return getNull(th);\
														 }

#endif // YOYO_RUNTIME_YOYO_H
