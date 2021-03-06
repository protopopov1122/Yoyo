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

#ifndef YOYO_RUNTIME_TYPE_TYPES_H
#define YOYO_RUNTIME_TYPE_TYPES_H

#include "yoyo-runtime.h"

#define CHECK_TYPES(t1, t2, v1, v2) ((v1->type==t1||v1->type==t2)&&(v2->type==t1||v2->type==t2))
#define CHECK_TYPE(t1, v1, v2) (v1->type==t1||v2->type==t1)

#define NEW_PROPERTY(name, prop) if (key==getSymbolId(&th->runtime->symbols, name)) return prop;
#define NEW_METHOD(name, proc, argc, ptr) NEW_PROPERTY(name, (YValue*) newNativeLambda(argc, proc, (YoyoObject*) ptr, th))

void Int_type_init(YRuntime*);
void Float_type_init(YRuntime*);
void Boolean_type_init(YRuntime*);
void String_type_init(YRuntime*);
void Array_type_init(YRuntime*);
void Object_type_init(YRuntime*);
void Lambda_type_init(YRuntime*);
void Declaration_type_init(YRuntime*);
void Null_type_init(YRuntime*);

wchar_t* Common_toString(YValue*, YThread*);
YValue* Common_readProperty(int32_t, YValue*, YThread*);
uint64_t Common_hashCode(YValue*, YThread*);
YValue* undefined_binary_operation(YValue*, YValue*, YThread*);
YValue* undefined_unary_operation(YValue*, YThread*);
YValue* concat_operation(YValue*, YValue*, YThread*);
int8_t compare_numbers(YValue*, YValue*, YThread*);
int compare_operation(YValue*, YValue*, YThread*);
void Type_init(YType*, YThread*);

#endif
