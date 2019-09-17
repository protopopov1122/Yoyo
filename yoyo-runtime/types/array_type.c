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

#include "types.h"

wchar_t* Array_toString(YValue* v, YThread* th) {
	YArray* arr = (YArray*) v;
	if (arr->toString != NULL)
		return arr->toString(arr, th);
	StringBuilder* sb = newStringBuilder(L"[");

	for (size_t i = 0; i < arr->size(arr, th); i++) {
		YValue* val = arr->get(arr, i, th);
		wchar_t* wstr = toString(val, th);
		sb->append(sb, wstr);
		free(wstr);
		if (i + 1 < arr->size(arr, th))
			sb->append(sb, L", ");
	}

	sb->append(sb, L"]");
	wchar_t* res = sb->string;
	free(sb);
	return res;
}

#define ARR_INIT  NativeLambda* nlam = (NativeLambda*) lambda;\
  YArray* array = (YArray*) nlam->object;

YOYO_FUNCTION(Array_type_size) {
	ARR_INIT
	return newInteger(array->size(array, th), th);
}
YOYO_FUNCTION(Array_type_add) {
	ARR_INIT
	array->add(array, args[0], th);
	return (YValue*) array;
}
YOYO_FUNCTION(Array_type_pop) {
	ARR_INIT
	if (array->size(array, th) == 0) {
		throwException(L"EmptyArray", NULL, 0, th);
		return getNull(th);
	}
	size_t index = array->size(array, th) - 1;
	YValue* val = array->get(array, index, th);
	array->remove(array, index, th);
	return val;
}
YOYO_FUNCTION(Array_type_addAll) {
	ARR_INIT
	YValue* val = args[0];
	if (val->type == &th->runtime->ArrayType) {
		Array_addAll(array, (YArray*) val, th);
	} else {
		wchar_t* wcs = toString(val, th);
		throwException(L"NotAnArray", &wcs, 1, th);
		free(wcs);
	}
	return (YValue*) array;
}
YOYO_FUNCTION(Array_type_insert) {
	ARR_INIT
	if (args[0]->type == &th->runtime->IntType) {
		size_t index = ((YInteger*) args[0])->value;
		array->insert(array, index, args[1], th);
	} else {
		wchar_t* wcs = toString(args[0], th);
		throwException(L"NotAnInteger", &wcs, 1, th);
		free(wcs);
	}
	return (YValue*) array;
}
YOYO_FUNCTION(Array_type_insertAll) {
	ARR_INIT
	if (args[0]->type == &th->runtime->IntType && args[1]->type == &th->runtime->ArrayType) {
		size_t index = ((YInteger*) args[0])->value;
		YArray* src = (YArray*) args[1];
		Array_insertAll(array, src, index, th);
	} else {
		if (args[0]->type != &th->runtime->IntType) {
			wchar_t* wcs = toString(args[0], th);
			throwException(L"NotAnInteger", &wcs, 1, th);
			free(wcs);
		}
		if (args[1]->type != &th->runtime->ArrayType) {
			wchar_t* wcs = toString(args[1], th);
			throwException(L"NotAnArray", &wcs, 1, th);
			free(wcs);
		}
	}
	return (YValue*) array;
}
YOYO_FUNCTION(Array_type_remove) {
	ARR_INIT
	if (args[0]->type == &th->runtime->IntType) {
		size_t index = ((YInteger*) args[0])->value;
		array->remove(array, index, th);
	} else {
		wchar_t* wcs = toString(args[0], th);
		throwException(L"NotAnInteger", &wcs, 1, th);
		free(wcs);
	}
	return (YValue*) array;
}
YOYO_FUNCTION(Array_type_clear) {
	ARR_INIT
	array->clear(array, th);
	return (YValue*) array;
}
YOYO_FUNCTION(Array_type_isEmpty) {
	ARR_INIT
	return newBoolean(array->size(array, th) == 0, th);
}
YOYO_FUNCTION(Array_type_slice) {
	ARR_INIT
	if (args[0]->type == &th->runtime->IntType && args[1]->type == &th->runtime->IntType) {
		YArray* slice = newSlice(array, ((YInteger*) args[0])->value,
				((YInteger*) args[1])->value, th);
		return (YValue*) slice;
	} else {
		if (args[0]->type != &th->runtime->IntType) {
			wchar_t* wcs = toString(args[0], th);
			throwException(L"NotAnInteger", &wcs, 1, th);
			free(wcs);
		}
		if (args[1]->type != &th->runtime->IntType) {
			wchar_t* wcs = toString(args[1], th);
			throwException(L"NotAnInteger", &wcs, 1, th);
			free(wcs);
		}
	}
	return getNull(th);
}
YOYO_FUNCTION (Array_type_flat) {
	ARR_INIT
	return (YValue*) Array_flat(array, th);
}
YOYO_FUNCTION(Array_type_each) {
	ARR_INIT
	if (args[0]->type == &th->runtime->LambdaType) {
		Array_each(array, (YLambda*) args[0], th);
	} else {
		wchar_t* wcs = toString(args[0], th);
		throwException(L"NotALambda", &wcs, 1, th);
		free(wcs);
	}
	return (YValue*) array;
}
YOYO_FUNCTION(Array_type_map) {
	ARR_INIT
	if (args[0]->type == &th->runtime->LambdaType) {
		return (YValue*) Array_map(array, (YLambda*) args[0], th);
	} else {
		wchar_t* wcs = toString(args[0], th);
		throwException(L"NotALambda", &wcs, 1, th);
		free(wcs);
	}
	return getNull(th);
}
YOYO_FUNCTION(Array_type_reduce) {
	ARR_INIT
	if (args[1]->type == &th->runtime->LambdaType) {
		return Array_reduce(array, (YLambda*) args[1], args[0], th);
	} else {
		wchar_t* wcs = toString(args[0], th);
		throwException(L"NotALambda", &wcs, 1, th);
		free(wcs);
	}
	return getNull(th);
}
YOYO_FUNCTION(Array_type_reverse) {
	ARR_INIT
	return (YValue*) Array_reverse(array, th);
}
YOYO_FUNCTION(Array_type_filter) {
	ARR_INIT
	if (args[0]->type == &th->runtime->LambdaType)
		return (YValue*) Array_filter(array, (YLambda*) args[0], th);
	else {
		wchar_t* wcs = toString(args[0], th);
		throwException(L"NotALambda", &wcs, 1, th);
		free(wcs);
	}
	return getNull(th);
}
YOYO_FUNCTION(Array_type_compact) {
	ARR_INIT
	return (YValue*) Array_compact(array, th);
}
YOYO_FUNCTION(Array_type_poll) {
	ARR_INIT
	YValue* out = array->get(array, 0, th);
	array->remove(array, 0, th);
	return out;
}
YOYO_FUNCTION(Array_type_removeAll) {
	ARR_INIT
	if (args[0]->type == &th->runtime->ArrayType) {
		YArray* arr = (YArray*) args[0];
		for (size_t i = 0; i < arr->size(arr, th); i++) {
			YValue* val = arr->get(arr, i, th);
			for (size_t j = 0; j < array->size(array, th); j++) {
				YValue* val2 = array->get(array, j, th);
				if (CHECK_EQUALS(val, val2, th)) {
					array->remove(array, j, th);
					break;
				}
			}
		}
	}
	else {
		wchar_t* wcs = toString(args[0], th);
		throwException(L"NotAnArray", &wcs, 1, th);
		free(wcs);
	}
	return (YValue*) array;
}
YOYO_FUNCTION(Array_type_clone) {
	ARR_INIT
	YArray* arr = newArray(th);
	for (size_t i = 0; i < array->size(array, th); i++)
		arr->add(arr, array->get(array, i, th), th);
	return (YValue*) arr;
}
YOYO_FUNCTION(Array_type_unique) {
	ARR_INIT
	return (YValue*) Array_unique(array, th);
}
YOYO_FUNCTION(Array_type_sort) {
	ARR_INIT
	if (args[0]->type == &th->runtime->LambdaType) {
		YLambda* lmbd = (YLambda*) args[0];
		return (YValue*) Array_sort(array, lmbd, th);
	} else {
		wchar_t* wcs = toString(args[0], th);
		throwException(L"NotALambda", &wcs, 1, th);
		free(wcs);
	}
	return getNull(th);
}
YOYO_FUNCTION(Array_type_find) {
	ARR_INIT
	return (YValue*) Array_find(array, args[0], th);
}
YOYO_FUNCTION(Array_type_tuple) {
	ARR_INIT
	return (YValue*) newTuple(array, th);
}
YOYO_FUNCTION(Array_type_iter) {
	ARR_INIT
	;
	return (YValue*) Array_iter(array, th);
}
YOYO_FUNCTION(Array_types) {
	ARR_INIT
	;
	YoyoType** types = malloc(sizeof(YoyoType*) * array->size(array, th));
	for (size_t i = 0; i < array->size(array, th); i++) {
		YValue* val = array->get(array, i, th);
		if (val->type == &th->runtime->DeclarationType)
			types[i] = (YoyoType*) val;
		else
			types[i] = val->type->TypeConstant;
	}
	YoyoType* type = newArrayType(types, array->size(array, th), th);
	free(types);
	return (YValue*) type;
}
#undef ARR_INIT

YValue* Array_readProperty(int32_t key, YValue* v, YThread* th) {
	YArray* arr = (YArray*) v;
	NEW_METHOD(L"size", Array_type_size, 0, arr);
	NEW_METHOD(L"add", Array_type_add, 1, arr)
	NEW_METHOD(L"push", Array_type_add, 1, arr)
	NEW_METHOD(L"pop", Array_type_pop, 0, arr)
	NEW_METHOD(L"addAll", Array_type_addAll, 1, arr);
	NEW_METHOD(L"insert", Array_type_insert, 2, arr);
	NEW_METHOD(L"insertAll", Array_type_insertAll, 2, arr);
	NEW_METHOD(L"remove", Array_type_remove, 1, arr);
	NEW_METHOD(L"clear", Array_type_clear, 0, arr);
	NEW_METHOD(L"isEmpty", Array_type_isEmpty, 0, arr);
	NEW_METHOD(L"slice", Array_type_slice, 2, arr);
	NEW_METHOD(L"each", Array_type_each, 1, arr);
	NEW_METHOD(L"flat", Array_type_flat, 0, arr);
	NEW_METHOD(L"map", Array_type_map, 1, arr);
	NEW_METHOD(L"reduce", Array_type_reduce, 2, arr);
	NEW_METHOD(L"reverse", Array_type_reverse, 0, arr);
	NEW_METHOD(L"compact", Array_type_compact, 0, arr);
	NEW_METHOD(L"filter", Array_type_filter, 1, arr);
	NEW_METHOD(L"poll", Array_type_poll, 0, arr);
	NEW_METHOD(L"removeAll", Array_type_removeAll, 1, arr);
	NEW_METHOD(L"clone", Array_type_clone, 0, arr);
	NEW_METHOD(L"unique", Array_type_unique, 0, arr);
	NEW_METHOD(L"find", Array_type_find, 1, arr);
	NEW_METHOD(L"sort", Array_type_sort, 1, arr);
	NEW_METHOD(L"tuple", Array_type_tuple, 0, arr);
	NEW_METHOD(L"iter", Array_type_iter, 0, arr);
	NEW_METHOD(L"types", Array_types, 0, arr);
	return Common_readProperty(key, v, th);
}

YValue* Array_subseq(YValue* val, size_t from, size_t to, YThread* th) {
	YArray* arr = (YArray*) val;
	return (YValue*) newSlice(arr, from, to, th);
}
YoyoIterator* Array_iterator(YValue* v, YThread* th) {
	YArray* arr = (YArray*) v;
	return Array_iter(arr, th);
}

void Array_type_init(YRuntime* runtime) {
	YThread* th = yoyo_thread(runtime);
	runtime->ArrayType.wstring = L"array";
	runtime->ArrayType.TypeConstant = newAtomicType(&th->runtime->ArrayType,
			th);
	runtime->ArrayType.oper.add_operation = concat_operation;
	runtime->ArrayType.oper.subtract_operation = undefined_binary_operation;
	runtime->ArrayType.oper.multiply_operation = undefined_binary_operation;
	runtime->ArrayType.oper.divide_operation = undefined_binary_operation;
	runtime->ArrayType.oper.modulo_operation = undefined_binary_operation;
	runtime->ArrayType.oper.power_operation = undefined_binary_operation;
	runtime->ArrayType.oper.and_operation = undefined_binary_operation;
	runtime->ArrayType.oper.or_operation = undefined_binary_operation;
	runtime->ArrayType.oper.xor_operation = undefined_binary_operation;
	runtime->ArrayType.oper.shr_operation = undefined_binary_operation;
	runtime->ArrayType.oper.shl_operation = undefined_binary_operation;
	runtime->ArrayType.oper.compare = compare_operation;
	runtime->ArrayType.oper.negate_operation = undefined_unary_operation;
	runtime->ArrayType.oper.not_operation = undefined_unary_operation;
	runtime->ArrayType.oper.toString = Array_toString;
	runtime->ArrayType.oper.readProperty = Array_readProperty;
	runtime->ArrayType.oper.hashCode = Common_hashCode;
	runtime->ArrayType.oper.readIndex = NULL;
	runtime->ArrayType.oper.writeIndex = NULL;
	runtime->ArrayType.oper.removeIndex = NULL;
	runtime->ArrayType.oper.subseq = Array_subseq;
	runtime->ArrayType.oper.iterator = Array_iterator;
}
