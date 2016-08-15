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

#include "yoyo-runtime.h"

/*Procedures to build different types of yoyo values*/

// Memory allocator for integer values
MemoryAllocator* IntAlloc = NULL;

/*Procedures to free different values*/
void freeAtomicData(YoyoObject* o) {
	free(o);
}

void freeStringData(YoyoObject* o) {
	YString* str = (YString*) o;
	free(str->value);
	free(str);
}

void freeInteger(YoyoObject* ptr) {
	IntAlloc->unuse(IntAlloc, ptr);
}

/*Check if there is null value in runtime.
 * If not create and assign it to runtime.
 * Return it*/
YValue* getNull(YThread* th) {
	if (th->runtime->Constants.NullPtr != NULL)
		return th->runtime->Constants.NullPtr;
	YValue* out = malloc(sizeof(YValue));
	initAtomicYoyoObject(&out->o, freeAtomicData);
	th->runtime->gc->registrate(th->runtime->gc, &out->o);
	out->type = &th->runtime->NullType;
	th->runtime->Constants.NullPtr = out;
	return out;
}

/*Procedures check value type and return certain C type
 * based on value. If value type is not valid
 * return default value(0)*/
double getFloat(YValue* v, YThread* th) {
	if (v->type == &th->runtime->FloatType)
		return ((YFloat*) v)->value;
	else if (v->type == &th->runtime->IntType)
		return (double) ((YInteger*) v)->value;
	return 0;
}

inline int64_t getInteger(YValue* v, YThread* th) {
	if (v->type == &th->runtime->IntType)
		return ((YInteger*) v)->value;
	else if (v->type == &th->runtime->FloatType)
		return (int64_t) ((YFloat*) v)->value;
	return 0;
}

/*Create integer value*/
YValue* newIntegerValue(int64_t value, YThread* th) {
	if (IntAlloc == NULL)
		IntAlloc = newMemoryAllocator(sizeof(YInteger), 100);
	YInteger* out = IntAlloc->alloc(IntAlloc);
	initAtomicYoyoObject(&out->parent.o, freeInteger);
	th->runtime->gc->registrate(th->runtime->gc, &out->parent.o);
	out->parent.type = &th->runtime->IntType;
	out->value = value;
	return (YValue*) out;
}
/*If integer cache contains integer it's returned from cache.
 * Else it is created by newIntegerValue, saved to cache
 * and returned.*/
YValue* newInteger(int64_t value, YThread* th) {
	RuntimeConstants* cnsts = &th->runtime->Constants;
	size_t pool_size = cnsts->IntPoolSize;
	if (value >= -pool_size/2 && value<pool_size/2) {
		size_t index = ((uint_fast64_t) value % (pool_size - 1));
		YInteger* i = cnsts->IntPool[index];
		if (i==NULL) {
			i = (YInteger*) newIntegerValue(value, th);
			th->runtime->Constants.IntPool[index] = i;
		}
		return (YValue*) i;
	}
	size_t index = ((uint_fast64_t) value % (cnsts->IntCacheSize - 1));
	YInteger* i = cnsts->IntCache[index];
	if (i == NULL || i->value != value) {
		i = (YInteger*) newIntegerValue(value, th);
		cnsts->IntCache[index] = i;
	}
	return (YValue*) i;
}
YValue* newFloat(double value, YThread* th) {
	YFloat* out = malloc(sizeof(YFloat));
	initAtomicYoyoObject(&out->parent.o, freeAtomicData);
	th->runtime->gc->registrate(th->runtime->gc, &out->parent.o);
	out->parent.type = &th->runtime->FloatType;
	out->value = value;
	return (YValue*) out;
}
YValue* newBooleanValue(bool value, YThread* th) {
	YBoolean* out = malloc(sizeof(YBoolean));
	initAtomicYoyoObject(&out->parent.o, freeAtomicData);
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
	initAtomicYoyoObject(&out->parent.o, freeStringData);
	th->runtime->gc->registrate(th->runtime->gc, &out->parent.o);
	out->parent.type = &th->runtime->StringType;

	out->value = malloc(sizeof(wchar_t) * (wcslen(value) + 1));
	wcscpy(out->value, value);
	out->value[wcslen(value)] = L'\0';

	return (YValue*) out;
}

void NativeLambda_mark(YoyoObject* ptr) {
	ptr->marked = true;
	NativeLambda* lmbd = (NativeLambda*) ptr;
	if (lmbd->object != NULL)
		MARK(lmbd->object);
	MARK(lmbd->lambda.sig);
}

void NativeLambda_free(YoyoObject* ptr) {
	NativeLambda* lmbd = (NativeLambda*) ptr;
	free(lmbd);
}

YoyoType* NativeLambda_signature(YLambda* l, YThread* th) {
	return (YoyoType*) l->sig;
}

YLambda* newNativeLambda(size_t argc,
		YValue* (*exec)(YLambda*, YObject*, YValue**, size_t, YThread*),
		YoyoObject* obj, YThread* th) {
	NativeLambda* out = malloc(sizeof(NativeLambda));
	initYoyoObject(&out->lambda.parent.o, NativeLambda_mark, NativeLambda_free);
	th->runtime->gc->registrate(th->runtime->gc, &out->lambda.parent.o);
	out->lambda.parent.type = &th->runtime->LambdaType;
	out->lambda.execute = exec;
	out->lambda.sig = newLambdaSignature(false, argc, false, NULL, NULL, th);
	out->lambda.signature = NativeLambda_signature;
	out->object = NULL;
	out->object = obj;
	return (YLambda*) out;
}

YValue* RawPointer_get(YObject* o, int32_t id, YThread* th) {
	return getNull(th);
}

bool RawPointer_contains(YObject* o, int32_t id, YThread* th) {
	return false;
}

void RawPointer_put(YObject* o, int32_t id, YValue* v, bool n, YThread* th) {
	return;
}

void RawPointer_remove(YObject* o, int32_t id, YThread* th) {
	return;
}

void RawPointer_setType(YObject* o, int32_t id, YoyoType* t, YThread* th) {
	return;
}

YoyoType* RawPointer_getType(YObject* o, int32_t id, YThread* th) {
	return th->runtime->NullType.TypeConstant;
}
void RawPointer_free(YoyoObject* ptr) {
	YRawPointer* raw = (YRawPointer*) ptr;
	raw->free(raw->ptr);
	free(raw);
}

YValue* newRawPointer(void* ptr, void (*freeptr)(void*), YThread* th) {
	YRawPointer* raw = malloc(sizeof(YRawPointer));
	initAtomicYoyoObject((YoyoObject*) raw, RawPointer_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) raw);
	raw->obj.parent.type = &th->runtime->ObjectType;

	raw->ptr = ptr;
	raw->free = freeptr;
	raw->obj.iterator = false;
	raw->obj.get = RawPointer_get;
	raw->obj.contains = RawPointer_contains;
	raw->obj.put = RawPointer_put;
	raw->obj.remove = RawPointer_remove;
	raw->obj.setType = RawPointer_setType;
	raw->obj.getType = RawPointer_getType;

	return (YValue*) raw;
}
