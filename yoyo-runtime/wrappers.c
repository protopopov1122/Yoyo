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

#include "wrappers.h"

typedef struct ArraySlice {
	YArray array;

	YArray* source;
	size_t start;
	size_t end;
} ArraySlice;
void Slice_mark(YoyoObject* ptr) {
	ptr->marked = true;
	ArraySlice* slice = (ArraySlice*) ptr;
	MARK(slice->source);
}
void Slice_free(YoyoObject* ptr) {
	free(ptr);
}
size_t Slice_size(YArray* arr, YThread* th) {
	ArraySlice* slice = (ArraySlice*) arr;
	return slice->end - slice->start;
}
YValue* Slice_get(YArray* arr, size_t index, YThread* th) {
	ArraySlice* slice = (ArraySlice*) arr;
	if (index >= slice->end - slice->start) {
		YValue* yint = newInteger(index, th);
		wchar_t* wstr = toString(yint, th);
		throwException(L"WrongArrayIndex", &wstr, 1, th);
		free(wstr);
		return getNull(th);
	}
	size_t real = index + slice->start;
	return slice->source->get(slice->source, real, th);
}
void Slice_set(YArray* arr, size_t index, YValue* v, YThread* th) {
	ArraySlice* slice = (ArraySlice*) arr;
	if (index >= slice->end - slice->start) {
		YValue* yint = newInteger(index, th);
		wchar_t* wstr = toString(yint, th);
		throwException(L"WrongArrayIndex", &wstr, 1, th);
		free(wstr);
		return;
	}
	size_t real = index + slice->start;
	slice->source->set(slice->source, real, v, th);
}
void Slice_add(YArray* arr, YValue* v, YThread* th) {
	ArraySlice* slice = (ArraySlice*) arr;
	slice->source->insert(slice->source, slice->end, v, th);
	slice->end++;
}
void Slice_remove(YArray* arr, size_t index, YThread* th) {
	ArraySlice* slice = (ArraySlice*) arr;
	if (index >= slice->end - slice->start) {
		YValue* yint = newInteger(index, th);
		wchar_t* wstr = toString(yint, th);
		throwException(L"WrongArrayIndex", &wstr, 1, th);
		free(wstr);
		return;
	}
	size_t real = index + slice->start;
	slice->source->remove(slice->source, real, th);
	slice->end--;
}
void Slice_insert(YArray* arr, size_t index, YValue* val, YThread* th) {
	ArraySlice* slice = (ArraySlice*) arr;
	if (index > slice->end - slice->start) {
		YValue* yint = newInteger(index, th);
		wchar_t* wstr = toString(yint, th);
		throwException(L"WrongArrayIndex", &wstr, 1, th);
		free(wstr);
		return;
	}
	size_t real = index + slice->start;
	slice->source->insert(slice->source, real, val, th);
	slice->end++;
}
YArray* newSlice(YArray* array, size_t start, size_t end, YThread* th) {
	ArraySlice* slice = malloc(sizeof(ArraySlice));
	initYoyoObject((YoyoObject*) slice, Slice_mark, Slice_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) slice);
	slice->array.parent.type = &th->runtime->ArrayType;
	slice->array.size = Slice_size;
	slice->array.toString = NULL;
	slice->array.get = Slice_get;
	slice->array.set = Slice_set;
	slice->array.add = Slice_add;
	slice->array.remove = Slice_remove;
	slice->array.insert = Slice_insert;
	slice->source = array;
	slice->start = start;
	slice->end = end;
	return ((YArray*) slice);
}
typedef struct ArrayTuple {
	YArray array;

	YArray* source;
} ArrayTuple;
void ArrayTuple_mark(YoyoObject* ptr) {
	ptr->marked = true;
	ArrayTuple* tuple = (ArrayTuple*) ptr;
	MARK(tuple->source);
}
void ArrayTuple_free(YoyoObject* ptr) {
	free(ptr);
}
size_t ArrayTuple_size(YArray* a, YThread* th) {
	ArrayTuple* tuple = (ArrayTuple*) a;
	return tuple->source->size(tuple->source, th);
}
YValue* ArrayTuple_get(YArray* a, size_t index, YThread* th) {
	ArrayTuple* tuple = (ArrayTuple*) a;
	return tuple->source->get(tuple->source, index, th);
}
void ArrayTuple_set(YArray* a, size_t index, YValue* val, YThread* th) {
	throwException(L"TupleModification", NULL, 0, th);
	return;
}
void ArrayTuple_add(YArray* a, YValue* val, YThread* th) {
	throwException(L"TupleModification", NULL, 0, th);
	return;
}
void ArrayTuple_insert(YArray* a, size_t index, YValue* val, YThread* th) {
	throwException(L"TupleModification", NULL, 0, th);
	return;
}
void ArrayTuple_remove(YArray* a, size_t index, YThread* th) {
	throwException(L"TupleModification", NULL, 0, th);
	return;
}
void ArrayTuple_clear(YArray* a, YThread* th) {
	throwException(L"TupleModification", NULL, 0, th);
	return;
}
YArray* newTuple(YArray* arr, YThread* th) {
	ArrayTuple* tuple = malloc(sizeof(ArrayTuple));
	initYoyoObject((YoyoObject*) tuple, ArrayTuple_mark, ArrayTuple_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) tuple);
	tuple->array.parent.type = &th->runtime->ArrayType;
	tuple->array.toString = NULL;
	tuple->source = arr;
	tuple->array.size = ArrayTuple_size;
	tuple->array.get = ArrayTuple_get;
	tuple->array.remove = ArrayTuple_remove;
	tuple->array.set = ArrayTuple_set;
	tuple->array.add = ArrayTuple_add;
	tuple->array.insert = ArrayTuple_insert;
	tuple->array.clear = ArrayTuple_clear;
	tuple->array.toString = NULL;
	return (YArray*) tuple;
}
typedef struct ROObject {
	YObject object;

	YObject* src;
} ROObject;

void ROObject_mark(YoyoObject* ptr) {
	ptr->marked = true;
	ROObject* obj = (ROObject*) ptr;
	MARK(obj->src);
}
void ROObject_free(YoyoObject* ptr) {
	free(ptr);
}
YValue* ROObject_get(YObject* o, int32_t id, YThread* th) {
	ROObject* robj = (ROObject*) o;
	return robj->src->get(robj->src, id, th);
}
bool ROObject_contains(YObject* o, int32_t id, YThread* th) {
	ROObject* robj = (ROObject*) o;
	return robj->src->contains(robj->src, id, th);
}
void ROObject_put(YObject* o, int32_t id, YValue* val, bool n, YThread* th) {
	throwException(L"ReadonlyObjectModification", NULL, 0, th);
	return;
}
void ROObject_remove(YObject* o, int32_t id, YThread* th) {
	throwException(L"ReadonlyObjectModification", NULL, 0, th);
	return;
}
void ROObject_setType(YObject* o, int32_t id, YoyoType* type, YThread* th) {
	throwException(L"ReadonlyObjectModification", NULL, 0, th);
	return;
}
YoyoType* ROObject_getType(YObject* o, int32_t id, YThread* th) {
	ROObject* robj = (ROObject*) o;
	return robj->src->getType(robj->src, id, th);
}

YObject* newReadonlyObject(YObject* o, YThread* th) {
	ROObject* obj = malloc(sizeof(ROObject));
	initYoyoObject((YoyoObject*) obj, ROObject_mark, ROObject_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) obj);
	obj->object.parent.type = &th->runtime->ObjectType;

	obj->src = o;

	obj->object.iterator = false;

	obj->object.get = ROObject_get;
	obj->object.contains = ROObject_contains;
	obj->object.put = ROObject_put;
	obj->object.remove = ROObject_remove;
	obj->object.getType = ROObject_getType;
	obj->object.setType = ROObject_setType;

	return (YObject*) obj;
}

typedef struct IteratorWrapper {
	YoyoIterator iter;

	YObject* object;
} IteratorWrapper;

void IteratorWrapper_mark(YoyoObject* ptr) {
	ptr->marked = true;
	IteratorWrapper* iter = (IteratorWrapper*) ptr;
	MARK(iter->object);
}
YValue* IteratorWrapper_next(YoyoIterator* i, YThread* th) {
	IteratorWrapper* iter = (IteratorWrapper*) i;
	int32_t id = getSymbolId(&th->runtime->symbols, L"next");
	if (iter->object->contains(iter->object, id, th)) {
		YValue* v = iter->object->get(iter->object, id, th);
		if (v->type->type == LambdaT) {
			YLambda* exec = (YLambda*) v;
			return invokeLambda(exec, iter->object, NULL, 0, th);
		}
	}
	return getNull(th);
}
bool IteratorWrapper_hasNext(YoyoIterator* i, YThread* th) {
	IteratorWrapper* iter = (IteratorWrapper*) i;
	int32_t id = getSymbolId(&th->runtime->symbols, L"hasNext");
	if (iter->object->contains(iter->object, id, th)) {
		YValue* v = iter->object->get(iter->object, id, th);
		if (v->type->type == LambdaT) {
			YLambda* exec = (YLambda*) v;
			YValue* ret = invokeLambda(exec, iter->object, NULL, 0, th);
			if (ret->type->type == BooleanT)
				return ((YBoolean*) ret)->value;
		}
	}
	return false;
}
void IteratorWrapper_reset(YoyoIterator* i, YThread* th) {
	IteratorWrapper* iter = (IteratorWrapper*) i;
	int32_t id = getSymbolId(&th->runtime->symbols, L"reset");
	if (iter->object->contains(iter->object, id, th)) {
		YValue* v = iter->object->get(iter->object, id, th);
		if (v->type->type == LambdaT) {
			YLambda* exec = (YLambda*) v;
			invokeLambda(exec, iter->object, NULL, 0, th);
		}
	}
}

YoyoIterator* newYoyoIterator(YObject* obj, YThread* th) {
	IteratorWrapper* iter = malloc(sizeof(IteratorWrapper));
	initYoyoObject((YoyoObject*) iter, IteratorWrapper_mark,
			(void (*)(YoyoObject*)) free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) iter);

	iter->object = obj;
	iter->iter.next = IteratorWrapper_next;
	iter->iter.hasNext = IteratorWrapper_hasNext;
	iter->iter.reset = IteratorWrapper_reset;

	YoyoIterator_init((YoyoIterator*) iter, th);

	return (YoyoIterator*) iter;
}

YOYO_FUNCTION(DefYoyoIterator_next) {
	YoyoIterator* iter = (YoyoIterator*) ((NativeLambda*) lambda)->object;
	return iter->next(iter, th);
}
YOYO_FUNCTION(DefYoyoIterator_hasNext) {
	YoyoIterator* iter = (YoyoIterator*) ((NativeLambda*) lambda)->object;
	return newBoolean(iter->hasNext(iter, th), th);
}
YOYO_FUNCTION(DefYoyoIterator_reset) {
	YoyoIterator* iter = (YoyoIterator*) ((NativeLambda*) lambda)->object;
	iter->reset(iter, th);
	return getNull(th);
}
YValue* YoyoIterator_get(YObject* o, int32_t id, YThread* th) {
	YoyoIterator* iter = (YoyoIterator*) o;

	int32_t next_id = getSymbolId(&th->runtime->symbols, L"next");
	int32_t hasNext_id = getSymbolId(&th->runtime->symbols, L"hasNext");
	int32_t reset_id = getSymbolId(&th->runtime->symbols, L"reset");

	if (id == next_id)
		return (YValue*) newNativeLambda(0, DefYoyoIterator_next,
				(YoyoObject*) iter, th);
	if (id == hasNext_id)
		return (YValue*) newNativeLambda(0, DefYoyoIterator_hasNext,
				(YoyoObject*) iter, th);
	if (id == reset_id)
		return (YValue*) newNativeLambda(0, DefYoyoIterator_reset,
				(YoyoObject*) iter, th);
	return getNull(th);
}
bool YoyoIterator_contains(YObject* o, int32_t id, YThread* th) {
	int32_t next_id = getSymbolId(&th->runtime->symbols, L"next");
	int32_t hasNext_id = getSymbolId(&th->runtime->symbols, L"hasNext");
	int32_t reset_id = getSymbolId(&th->runtime->symbols, L"reset");

	return id == next_id || id == hasNext_id || id == reset_id;
}
void YoyoIterator_set(YObject* o, int32_t id, YValue* v, bool newF, YThread* th) {
	return;
}
void YoyoIterator_remove(YObject* o, int32_t id, YThread* th) {
	return;
}
void YoyoIterator_setType(YObject* o, int32_t id, YoyoType* type, YThread* th) {
	return;
}
YoyoType* YoyoIterator_getType(YObject* o, int32_t id, YThread* th) {
	return th->runtime->NullType.TypeConstant;
}

void YoyoIterator_init(YoyoIterator* iter, YThread* th) {
	YObject* obj = (YObject*) iter;

	obj->parent.type = &th->runtime->ObjectType;
	obj->iterator = true;

	obj->get = YoyoIterator_get;
	obj->put = YoyoIterator_set;
	obj->contains = YoyoIterator_contains;
	obj->remove = YoyoIterator_remove;
	obj->setType = YoyoIterator_setType;
	obj->getType = YoyoIterator_getType;
}

typedef struct ArrayObject {
	YArray array;

	YObject* object;
} ArrayObject;

void ArrayObject_mark(YoyoObject* ptr) {
	ptr->marked = true;
	ArrayObject* array = (ArrayObject*) ptr;
	MARK(array->object);
}
size_t ArrayObject_size(YArray* a, YThread* th) {
	ArrayObject* array = (ArrayObject*) a;
	int32_t id = getSymbolId(&th->runtime->symbols, L"size");
	if (array->object->contains(array->object, id, th)) {
		YValue* v = (YValue*) array->object->get(array->object, id, th);
		if (v->type->type == LambdaT) {
			YLambda* exec = (YLambda*) v;
			YValue* out = invokeLambda(exec, array->object, NULL, 0, th);
			if (out->type->type == IntegerT)
				return (size_t) ((YInteger*) out)->value;
		}
	}
	return 0;
}
YValue* ArrayObject_get(YArray* a, size_t index, YThread* th) {
	ArrayObject* array = (ArrayObject*) a;
	int32_t id = getSymbolId(&th->runtime->symbols, L"get");
	if (array->object->contains(array->object, id, th)) {
		YValue* v = (YValue*) array->object->get(array->object, id, th);
		if (v->type->type == LambdaT) {
			YLambda* exec = (YLambda*) v;
			YValue* arg = newInteger(index, th);
			return invokeLambda(exec, array->object, &arg, 1, th);
		}
	}
	return 0;
}
void ArrayObject_set(YArray* a, size_t index, YValue* value, YThread* th) {
	ArrayObject* array = (ArrayObject*) a;
	int32_t id = getSymbolId(&th->runtime->symbols, L"set");
	if (array->object->contains(array->object, id, th)) {
		YValue* v = (YValue*) array->object->get(array->object, id, th);
		if (v->type->type == LambdaT) {
			YLambda* exec = (YLambda*) v;
			YValue* args[] = { newInteger(index, th), value };
			invokeLambda(exec, array->object, args, 2, th);
		}
	}
}
void ArrayObject_add(YArray* a, YValue* value, YThread* th) {
	ArrayObject* array = (ArrayObject*) a;
	int32_t id = getSymbolId(&th->runtime->symbols, L"add");
	if (array->object->contains(array->object, id, th)) {
		YValue* v = (YValue*) array->object->get(array->object, id, th);
		if (v->type->type == LambdaT) {
			YLambda* exec = (YLambda*) v;
			invokeLambda(exec, array->object, &value, 1, th);
		}
	}
}
void ArrayObject_insert(YArray* a, size_t index, YValue* value, YThread* th) {
	ArrayObject* array = (ArrayObject*) a;
	int32_t id = getSymbolId(&th->runtime->symbols, L"insert");
	if (array->object->contains(array->object, id, th)) {
		YValue* v = (YValue*) array->object->get(array->object, id, th);
		if (v->type->type == LambdaT) {
			YLambda* exec = (YLambda*) v;
			YValue* args[] = { newInteger(index, th), value };
			invokeLambda(exec, array->object, args, 2, th);
		}
	}
}
void ArrayObject_remove(YArray* a, size_t index, YThread* th) {
	ArrayObject* array = (ArrayObject*) a;
	int32_t id = getSymbolId(&th->runtime->symbols, L"remove");
	if (array->object->contains(array->object, id, th)) {
		YValue* v = (YValue*) array->object->get(array->object, id, th);
		if (v->type->type == LambdaT) {
			YLambda* exec = (YLambda*) v;
			YValue* args[] = { newInteger(index, th) };
			invokeLambda(exec, array->object, args, 1, th);
		}
	}
}

YArray* newArrayObject(YObject* obj, YThread* th) {
	ArrayObject* array = malloc(sizeof(ArrayObject));
	initYoyoObject((YoyoObject*) array, ArrayObject_mark,
			(void (*)(YoyoObject*)) free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) array);
	array->array.parent.type = &th->runtime->ArrayType;
	array->object = obj;

	array->array.size = ArrayObject_size;
	array->array.get = ArrayObject_get;
	array->array.set = ArrayObject_set;
	array->array.add = ArrayObject_add;
	array->array.remove = ArrayObject_remove;
	array->array.insert = ArrayObject_insert;
	array->array.toString = NULL;

	return (YArray*) array;
}
