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

/*File contains default array implementation
 * and useful methods to work with
 * any abstract arrays*/

typedef struct DefaultArray {
	YArray parent;

	YValue** array;
	size_t capacity;
	size_t size;

	MUTEX access_mutex;
} DefaultArray;

void DefaultArray_mark(YoyoObject* ptr) {
	ptr->marked = true;
	DefaultArray* arr = (DefaultArray*) ptr;
	MUTEX_LOCK(&arr->access_mutex);
	for (size_t i = 0; i < arr->size; i++) {
		if (arr->array[i] != NULL)
			MARK(arr->array[i]);
	}
	MUTEX_UNLOCK(&arr->access_mutex);
}
void DefaultArray_free(YoyoObject* ptr) {
	DefaultArray* arr = (DefaultArray*) ptr;
	DESTROY_MUTEX(&arr->access_mutex);
	free(arr->array);
	free(arr);
}

void DefaultArray_prepare(DefaultArray* arr, size_t index, YThread* th) {
	if (index >= arr->size) {
		if (index + 1 < arr->capacity)
			arr->size = index + 1;
		else {
			size_t nextC = index + 1;
			arr->array = realloc(arr->array, sizeof(YValue*) * nextC);
			for (size_t i = arr->capacity; i < nextC; i++)
				arr->array[i] = getNull(th);
			arr->size = nextC;
			arr->capacity = nextC;
		}
	}
}

size_t DefaultArray_size(YArray* a, YThread* th) {
	DefaultArray* arr = (DefaultArray*) a;
	return arr->size;
}
YValue* DefaultArray_get(YArray* a, size_t index, YThread* th) {
	DefaultArray* arr = (DefaultArray*) a;
	MUTEX_LOCK(&arr->access_mutex);
	if (index >= arr->size) {
		YValue* yint = newInteger(index, th);
		wchar_t* wstr = toString(yint, th);
		throwException(L"WrongArrayIndex", &wstr, 1, th);
		free(wstr);
		MUTEX_UNLOCK(&arr->access_mutex);
		return getNull(th);
	}
	YValue* val = arr->array[index];
	MUTEX_UNLOCK(&arr->access_mutex);
	return val;
}
void DefaultArray_add(YArray* a, YValue* val, YThread* th) {
	DefaultArray* arr = (DefaultArray*) a;
	MUTEX_LOCK(&arr->access_mutex);
	DefaultArray_prepare(arr, arr->size, th);
	arr->array[arr->size - 1] = val;
	MUTEX_UNLOCK(&arr->access_mutex);
}
void DefaultArray_insert(YArray* arr, size_t index, YValue* val, YThread* th) {
	DefaultArray* array = (DefaultArray*) arr;
	if (index > array->size) {
		YValue* yint = newInteger(index, th);
		wchar_t* wstr = toString(yint, th);
		throwException(L"WrongArrayIndex", &wstr, 1, th);
		free(wstr);
		return;
	}
	MUTEX_LOCK(&array->access_mutex);
	DefaultArray_prepare(array, array->size, th);
	for (size_t i = array->size - 1; i > index; i--) {
		array->array[i] = array->array[i - 1];
	}
	array->array[index] = val;

	MUTEX_UNLOCK(&array->access_mutex);
}
void DefaultArray_set(YArray* a, size_t index, YValue* value, YThread* th) {
	DefaultArray* arr = (DefaultArray*) a;
	MUTEX_LOCK(&arr->access_mutex);
	DefaultArray_prepare(arr, index, th);
	arr->array[index] = value;
	MUTEX_UNLOCK(&arr->access_mutex);
}
void DefaultArray_remove(YArray* a, size_t index, YThread* th) {
	DefaultArray* arr = (DefaultArray*) a;
	MUTEX_LOCK(&arr->access_mutex);

	size_t newCap = arr->capacity - 1;
	size_t newSize = arr->size - 1;
	YValue** newArr = newCap==0 ? NULL : malloc(sizeof(YValue*) * newCap);
	for (size_t i = 0; i < index; i++)
		newArr[i] = arr->array[i];
	for (size_t i = index + 1; i < arr->size; i++)
		newArr[i - 1] = arr->array[i];
	for (size_t i = newSize; i < newCap; i++)
		newArr[i] = NULL;
	arr->size = newSize;
	arr->capacity = newCap;
	free(arr->array);
	arr->array = newArr;

	MUTEX_UNLOCK(&arr->access_mutex);
}

void DefaultArray_clear(YArray* a, YThread* th) {
	DefaultArray* arr = (DefaultArray*) a;
	MUTEX_LOCK(&arr->access_mutex);
	free(arr->array);
	arr->size = 0;
	arr->capacity = 1;
	arr->array = malloc(sizeof(YValue*) * arr->capacity);
	MUTEX_UNLOCK(&arr->access_mutex);
}

typedef struct ArrayIterator {
	YoyoIterator iter;

	YArray* array;
	size_t index;
} ArrayIterator;
void ArrayIterator_mark(YoyoObject* ptr) {
	ptr->marked = true;
	ArrayIterator* iter = (ArrayIterator*) ptr;
	YoyoObject* ho = (YoyoObject*) iter->array;
	MARK(ho);
}
void ArrayIterator_reset(YoyoIterator* i, YThread* th) {
	((ArrayIterator*) i)->index = 0;
}
bool ArrayIterator_hasNext(YoyoIterator* i, YThread* th) {
	ArrayIterator* iter = (ArrayIterator*) i;
	return iter->index < iter->array->size(iter->array, th);
}
YValue* ArrayIterator_next(YoyoIterator* i, YThread* th) {
	ArrayIterator* iter = (ArrayIterator*) i;
	if (iter->index < iter->array->size(iter->array, th))
		return iter->array->get(iter->array, iter->index++, th);
	else
		return getNull(th);
}
YoyoIterator* Array_iter(YArray* array, YThread* th) {
	ArrayIterator* iter = malloc(sizeof(ArrayIterator));
	initYoyoObject((YoyoObject*) iter, ArrayIterator_mark,
			(void (*)(YoyoObject*)) free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) iter);
	iter->array = array;
	iter->index = 0;
	iter->iter.reset = ArrayIterator_reset;
	iter->iter.next = ArrayIterator_next;
	iter->iter.hasNext = ArrayIterator_hasNext;

	YoyoIterator_init((YoyoIterator*) iter, th);

	return (YoyoIterator*) iter;
}

YArray* newArray(YThread* th) {
	DefaultArray* arr = malloc(sizeof(DefaultArray));
	initYoyoObject(&arr->parent.parent.o, DefaultArray_mark, DefaultArray_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) arr);
	arr->parent.parent.type = &th->runtime->ArrayType;
	arr->size = 0;
	arr->capacity = 10;
	arr->array = malloc(sizeof(YValue*) * arr->capacity);
	memset(arr->array, 0, sizeof(YValue*) * arr->capacity);
	arr->parent.toString = NULL;
	NEW_MUTEX(&arr->access_mutex);

	arr->parent.size = DefaultArray_size;
	arr->parent.add = DefaultArray_add;
	arr->parent.insert = DefaultArray_insert;
	arr->parent.get = DefaultArray_get;
	arr->parent.set = DefaultArray_set;
	arr->parent.remove = DefaultArray_remove;
	arr->parent.clear = DefaultArray_clear;

	return (YArray*) arr;
}

void Array_addAll(YArray* dst, YArray* src, YThread* th) {
	((YoyoObject*) dst)->linkc++;
	((YoyoObject*) src)->linkc++;
	for (size_t i = 0; i < src->size(src, th); i++)
		dst->add(dst, src->get(src, i, th), th);
	((YoyoObject*) dst)->linkc--;
	((YoyoObject*) src)->linkc--;
}
void Array_insertAll(YArray* dst, YArray* src, size_t index, YThread* th) {
	((YoyoObject*) dst)->linkc++;
	((YoyoObject*) src)->linkc++;
	for (size_t i = src->size(src, th) - 1; i < src->size(src, th); i--)
		dst->insert(dst, index, src->get(src, i, th), th);
	((YoyoObject*) dst)->linkc--;
	((YoyoObject*) src)->linkc--;
}

YArray* Array_flat(YArray* arr, YThread* th) {
	YArray* out = newArray(th);
	out->parent.o.linkc++;

	for (size_t i = 0; i < arr->size(arr, th); i++) {
		YValue* val = arr->get(arr, i, th);
		if (val->type == &th->runtime->ArrayType) {
			Array_addAll(out, Array_flat((YArray*) val, th), th);
		} else {
			out->add(out, val, th);
		}
	}

	out->parent.o.linkc--;
	return out;
}
void Array_each(YArray* arr, YLambda* lmbd, YThread* th) {
	if (lmbd->sig->argc != 1)
		return;
	arr->parent.o.linkc++;
	lmbd->parent.o.linkc++;
	for (size_t i = 0; i < arr->size(arr, th); i++) {
		YValue* arg = arr->get(arr, i, th);
		invokeLambda(lmbd, NULL, &arg, 1, th);
	}
	arr->parent.o.linkc--;
	lmbd->parent.o.linkc--;
}
YArray* Array_map(YArray* arr, YLambda* lmbd, YThread* th) {
	if (lmbd->sig->argc != 1)
		return newArray(th);
	arr->parent.o.linkc++;
	lmbd->parent.o.linkc++;
	YArray* out = newArray(th);
	out->parent.o.linkc++;
	for (size_t i = 0; i < arr->size(arr, th); i++) {
		YValue* val = arr->get(arr, i, th);
		out->add(out, invokeLambda(lmbd, NULL, &val, 1, th), th);
	}
	out->parent.o.linkc--;
	arr->parent.o.linkc--;
	lmbd->parent.o.linkc--;
	return out;
}
YValue* Array_reduce(YArray* arr, YLambda* lmbd, YValue* val, YThread* th) {
	if (lmbd->sig->argc != 2)
		return getNull(th);
	arr->parent.o.linkc++;
	lmbd->parent.o.linkc++;
	for (size_t i = 0; i < arr->size(arr, th); i++) {
		YValue* v = arr->get(arr, i, th);
		YValue* args[] = { val, v };
		val = invokeLambda(lmbd, NULL, args, 2, th);
	}
	arr->parent.o.linkc--;
	lmbd->parent.o.linkc--;
	return val;
}
YArray* Array_reverse(YArray* arr, YThread* th) {
	YArray* out = newArray(th);
	for (size_t i = arr->size(arr, th) - 1; i < arr->size(arr, th); i--)
		out->add(out, arr->get(arr, i, th), th);
	return out;
}
YArray* Array_filter(YArray* arr, YLambda* lmbd, YThread* th) {
	arr->parent.o.linkc++;
	lmbd->parent.o.linkc++;
	YArray* out = newArray(th);
	out->parent.o.linkc++;
	for (size_t i = 0; i < arr->size(arr, th); i++) {
		YValue* val = arr->get(arr, i, th);
		YValue* res = invokeLambda(lmbd, NULL, &val, 1, th);
		if (res->type == &th->runtime->BooleanType && ((YBoolean*) res)->value)
			out->add(out, val, th);
	}
	out->parent.o.linkc--;
	arr->parent.o.linkc--;
	lmbd->parent.o.linkc--;
	return out;
}
YArray* Array_compact(YArray* arr, YThread* th) {
	YArray* out = newArray(th);
	for (size_t i = 0; i < arr->size(arr, th); i++) {
		YValue* val = arr->get(arr, i, th);
		if (val->type != &th->runtime->NullType)
			out->add(out, val, th);
	}
	return out;
}
YArray* Array_unique(YArray* arr, YThread* th) {
	arr->parent.o.linkc++;
	YArray* out = newArray(th);
	out->parent.o.linkc++;

	for (size_t i = 0; i < arr->size(arr, th); i++) {
		YValue* val = arr->get(arr, i, th);
		bool cont = false;
		for (size_t j = 0; j < out->size(out, th); j++) {
			YValue* val2 = out->get(out, j, th);
			if (CHECK_EQUALS(val, val2, th)) {
				cont = true;
				break;
			}
		}
		if (!cont)
			out->add(out, val, th);
	}
	out->parent.o.linkc--;
	arr->parent.o.linkc--;
	return out;
}
YArray* Array_sort(YArray* arr, YLambda* lmbd, YThread* th) {
	if (lmbd->sig->argc != 2)
		return arr;
	if (arr->size(arr, th) == 0)
		return arr;
	arr->parent.o.linkc++;
	lmbd->parent.o.linkc++;
	YArray* out = newArray(th);
	out->parent.o.linkc++;

	YArray* sub1 = newArray(th);
	YArray* sub2 = newArray(th);
	YArray* sub3 = newArray(th);
	sub1->parent.o.linkc++;
	sub2->parent.o.linkc++;
	sub3->parent.o.linkc++;
	YValue* val = arr->get(arr, arr->size(arr, th) / 2, th);
	for (size_t i = 0; i < arr->size(arr, th); i++) {
		YValue* v = arr->get(arr, i, th);
		YValue* args[] = { val, v };
		YValue* res = invokeLambda(lmbd, NULL, args, 2, th);
		if (res->type != &th->runtime->IntType)
			break;
		int64_t ires = ((YInteger*) res)->value;
		if (ires == 0)
			sub2->add(sub2, v, th);
		else if (ires < 0)
			sub3->add(sub3, v, th);
		else
			sub1->add(sub1, v, th);
	}
	sub1->parent.o.linkc--;
	Array_addAll(out, Array_sort(sub1, lmbd, th), th);
	sub2->parent.o.linkc--;
	Array_addAll(out, sub2, th);
	sub3->parent.o.linkc--;
	Array_addAll(out, Array_sort(sub3, lmbd, th), th);

	arr->parent.o.linkc--;
	lmbd->parent.o.linkc--;
	out->parent.o.linkc--;
	return out;
}
YArray* Array_find(YArray* arr, YValue* val, YThread* th) {
	arr->parent.o.linkc++;
	val->o.linkc++;
	YArray* out = newArray(th);
	out->parent.o.linkc++;
	for (size_t i = 0; i < arr->size(arr, th); i++) {
		YValue* v = arr->get(arr, i, th);
		if (CHECK_EQUALS(val, v, th))
			out->add(out, newInteger((int64_t) i, th), th);
	}
	out->parent.o.linkc--;
	val->o.linkc--;
	arr->parent.o.linkc--;
	return out;
}

