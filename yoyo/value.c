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

#include "../headers/yoyo/value.h"

#include "../headers/yoyo/memalloc.h"

/*Procedures to build different types of yoyo values*/

// Memory allocator for integer values
MemoryAllocator* IntAlloc = NULL;

/*Procedures to free different values*/
void freeAtomicData(HeapObject* o) {
	free(o);
}

void freeStringData(HeapObject* o) {
	YString* str = (YString*) o;
	free(str->value);
	free(str);
}

void freeInteger(HeapObject* ptr) {
	IntAlloc->unuse(IntAlloc, ptr);
}

/*Check if there is null value in runtime.
 * If not create and assign it to runtime.
 * Return it*/
YValue* getNull(YThread* th) {
	if (th->runtime->Constants.NullPtr != NULL)
		return th->runtime->Constants.NullPtr;
	YValue* out = malloc(sizeof(YValue));
	initAtomicHeapObject(&out->o, freeAtomicData);
	th->runtime->gc->registrate(th->runtime->gc, &out->o);
	out->type = &th->runtime->NullType;
	th->runtime->Constants.NullPtr = out;
	return out;
}

/*Procedures check value type and return certain C type
 * based on value. If value type is not valid
 * return default value(0)*/
double getFloat(YValue* v) {
	if (v->type->type == FloatT)
		return ((YFloat*) v)->value;
	else if (v->type->type == IntegerT)
		return (double) ((YInteger*) v)->value;
	return 0;
}

inline int64_t getInteger(YValue* v) {
	if (v->type->type == IntegerT)
		return ((YInteger*) v)->value;
	else if (v->type->type == FloatT)
		return (int64_t) ((YFloat*) v)->value;
	return 0;
}

/*Create integer value*/
YValue* newIntegerValue(int64_t value, YThread* th) {
	if (IntAlloc == NULL)
		IntAlloc = newMemoryAllocator(sizeof(YInteger), 100);
	YInteger* out = IntAlloc->alloc(IntAlloc);
	initAtomicHeapObject(&out->parent.o, freeInteger);
	th->runtime->gc->registrate(th->runtime->gc, &out->parent.o);
	out->parent.type = &th->runtime->IntType;
	out->value = value;
	return (YValue*) out;
}
/*If integer cache contains integer it's returned from cache.
 * Else it is created by newIntegerValue, saved to cache
 * and returned.*/
YValue* newInteger(int64_t value, YThread* th) {
	size_t index = (value % INT_POOL_SIZE) * (value < 0 ? -1 : 1);
	YInteger* i = th->runtime->Constants.IntPool[index];
	if (i == NULL || i->value != value) {
		i = (YInteger*) newIntegerValue(value, th);
		th->runtime->Constants.IntPool[index] = i;
	}
	return (YValue*) i;
}
YValue* newFloat(double value, YThread* th) {
	YFloat* out = malloc(sizeof(YFloat));
	initAtomicHeapObject(&out->parent.o, freeAtomicData);
	th->runtime->gc->registrate(th->runtime->gc, &out->parent.o);
	out->parent.type = &th->runtime->FloatType;
	out->value = value;
	return (YValue*) out;
}
YValue* newBooleanValue(bool value, YThread* th) {
	YBoolean* out = malloc(sizeof(YBoolean));
	initAtomicHeapObject(&out->parent.o, freeAtomicData);
	th->runtime->gc->registrate(th->runtime->gc, &out->parent.o);
	out->parent.type = &th->runtime->BooleanType;
	out->value = value;
	return (YValue*) out;
}
YValue* newBoolean(bool b, YThread* th) {
	return (YValue*) (
			b ? th->runtime->Constants.TrueValue : th->runtime->Constants.FalseValue);
}
YValue* newString(wchar_t* value, YThread* th) {
	YString* out = malloc(sizeof(YString));
	initAtomicHeapObject(&out->parent.o, freeStringData);
	th->runtime->gc->registrate(th->runtime->gc, &out->parent.o);
	out->parent.type = &th->runtime->StringType;

	out->value = malloc(sizeof(wchar_t) * (wcslen(value) + 1));
	wcscpy(out->value, value);
	out->value[wcslen(value)] = L'\0';

	return (YValue*) out;
}

void NativeLambda_mark(HeapObject* ptr) {
	ptr->marked = true;
	NativeLambda* lmbd = (NativeLambda*) ptr;
	if (lmbd->object != NULL)
		MARK(lmbd->object);
	MARK(lmbd->lambda.sig);
}

void NativeLambda_free(HeapObject* ptr) {
	NativeLambda* lmbd = (NativeLambda*) ptr;
	free(lmbd);
}

YoyoType* NativeLambda_signature(YLambda* l, YThread* th) {
	return (YoyoType*) l->sig;
}

YLambda* newNativeLambda(size_t argc,
		YValue* (*exec)(YLambda*, YValue**, size_t, YThread*), HeapObject* obj,
		YThread* th) {
	NativeLambda* out = malloc(sizeof(NativeLambda));
	initHeapObject(&out->lambda.parent.o, NativeLambda_mark, NativeLambda_free);
	th->runtime->gc->registrate(th->runtime->gc, &out->lambda.parent.o);
	out->lambda.parent.type = &th->runtime->LambdaType;
	out->lambda.execute = exec;
	out->lambda.sig = newLambdaSignature(argc, false, NULL, NULL, th);
	out->lambda.signature = NativeLambda_signature;
	out->object = NULL;
	out->object = obj;
	return (YLambda*) out;
}
