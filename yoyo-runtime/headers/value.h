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

#ifndef YILI_VALUE_H
#define YILI_VALUE_H

#include "core.h"
#include "gc.h"
#include "runtime.h"

typedef struct YValue {
	YoyoObject o;
	YType* type;
} YValue;

typedef struct YInteger {
	YValue parent;
	int64_t value;
} YInteger;

typedef struct YFloat {
	YValue parent;
	double value;
} YFloat;

typedef struct YBoolean {
	YValue parent;
	bool value;
} YBoolean;

typedef struct YString {
	YValue parent;
	wchar_t* value;
} YString;

typedef struct YObject {
	YValue parent;

	YValue* (*get)(struct YObject*, int32_t, YThread*);
	void (*put)(struct YObject*, int32_t, YValue*, bool, YThread*);
	bool (*contains)(struct YObject*, int32_t, YThread*);
	void (*remove)(struct YObject*, int32_t, YThread*);
	void (*setType)(struct YObject*, int32_t, struct YoyoType*, YThread*);
	YoyoType* (*getType)(struct YObject*, int32_t, YThread*);

	bool iterator;
} YObject;

typedef struct YArray {
	YValue parent;

	YValue* (*get)(struct YArray*, size_t, YThread*);
	void (*set)(struct YArray*, size_t, YValue*, YThread*);
	void (*remove)(struct YArray*, size_t, YThread*);
	void (*add)(struct YArray*, YValue*, YThread*);
	void (*insert)(struct YArray*, size_t, YValue*, YThread*);
	size_t (*size)(struct YArray*, YThread*);
	void (*clear)(struct YArray*, YThread*);

	wchar_t* (*toString)(struct YArray*, YThread*);
} YArray;

typedef struct YoyoIterator {
	YObject o;

	bool (*hasNext)(struct YoyoIterator*, YThread*);
	YValue* (*next)(struct YoyoIterator*, YThread*);
	void (*reset)(struct YoyoIterator*, YThread*);
} YoyoIterator;

typedef YValue* (*YCallable)(struct YLambda*, YObject*, YValue**, size_t,
		YThread*);

typedef struct YLambda {
	YValue parent;

	YoyoType* (*signature)(YLambda*, YThread*);
	struct YoyoLambdaSignature* sig;
	YCallable execute;
} YLambda;

YValue* getNull(YThread*);
double getFloat(YValue*, YThread*);
int64_t getInteger(YValue*, YThread*);

YValue* newIntegerValue(int64_t, YThread*);
YValue* newBooleanValue(bool, YThread*);

YValue* newInteger(int64_t, YThread*);
YValue* newFloat(double, YThread*);
YValue* newBoolean(bool, YThread*);
YValue* newString(wchar_t*, YThread*);

YObject* newTreeObject(YObject*, YThread*);
YObject* newHashObject(YObject*, YThread*);
YObject* newComplexObject(YObject*, YObject**, size_t, YThread*);

YArray* newArray(YThread* th);

YLambda* newNativeLambda(size_t,
		YValue* (*)(YLambda*, YObject*, YValue**, size_t, YThread*),
		YoyoObject*, YThread*);
YLambda* newOverloadedLambda(YLambda**, size_t, YLambda*, YThread*);
typedef struct YoyoType {
	struct YValue parent;

	bool (*verify)(struct YoyoType*, YValue*, YThread*);
	bool (*compatible)(struct YoyoType*, struct YoyoType*, YThread*);
	wchar_t* string;

	enum {
		AtomicDT, InterfaceDT, LambdaSignatureDT, ArrayDT, TypeMixDT, NotNullDT
	} type;
} YoyoType;

typedef struct YoyoLambdaSignature {
	YoyoType type;

	bool method;
	int32_t argc;
	bool vararg;
	YoyoType** args;
	YoyoType* ret;
} YoyoLambdaSignature;

typedef struct YoyoAttribute {
	int32_t id;
	struct YoyoType* type;
} YoyoAttribute;

typedef struct YoyoInterface {
	struct YoyoType type;

	struct YoyoInterface** parents;
	size_t parent_count;

	YoyoAttribute* attrs;
	size_t attr_count;
} YoyoInterface;

YoyoType* newAtomicType(YType*, YThread*);
YoyoType* newArrayType(YoyoType**, size_t, YThread*);
YoyoType* newInterface(YoyoInterface**, size_t, YoyoAttribute*, size_t,
		YThread*);
YoyoType* newTypeMix(YoyoType**, size_t, YThread*);
YoyoLambdaSignature* newLambdaSignature(bool, int32_t, bool, YoyoType**,
		YoyoType*, YThread*);
YoyoType* newNotNullType(YoyoType*, YThread*);

typedef struct NativeLambda {
	YLambda lambda;

	YoyoObject* object;
} NativeLambda;

typedef struct YRawPointer {
	YObject obj;

	void* ptr;
	void (*free)(void*);
} YRawPointer;

YValue* newRawPointer(void*, void (*)(void*), YThread*);

#endif
