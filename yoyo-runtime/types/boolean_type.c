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

wchar_t* Boolean_toString(YValue* v, YThread* th) {
	wchar_t* src = NULL;
	if (((YBoolean*) v)->value)
		src = L"true";
	else
		src = L"false";
	wchar_t* out = malloc(sizeof(wchar_t) * (wcslen(src) + 1));
	wcscpy(out, src);
	return out;
}

int Boolean_compare(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type != &th->runtime->BooleanType || v2->type != &th->runtime->BooleanType)
		return false;
	return ((YBoolean*) v1)->value == ((YBoolean*) v2)->value ?
			COMPARE_EQUALS : COMPARE_NOT_EQUALS;
}

YValue* Boolean_not(YValue* v1, YThread* th) {
	return newBoolean(!((YBoolean*) v1)->value, th);
}

YValue* Boolean_and(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type == &th->runtime->BooleanType && v2->type == &th->runtime->BooleanType)
		return newBoolean(((YBoolean*) v1)->value && ((YBoolean*) v2)->value,
				th);
	return getNull(th);
}

YValue* Boolean_or(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type == &th->runtime->BooleanType && v2->type == &th->runtime->BooleanType)
		return newBoolean(((YBoolean*) v1)->value || ((YBoolean*) v2)->value,
				th);
	return getNull(th);
}

YValue* Boolean_readProperty(int32_t key, YValue* v, YThread* th) {
	return Common_readProperty(key, v, th);
}

void Boolean_type_init(YRuntime* runtime) {
	YThread* th = yoyo_thread(runtime);
	runtime->BooleanType.wstring = L"boolean";
	runtime->BooleanType.TypeConstant = newAtomicType(&th->runtime->BooleanType,
			yoyo_thread(runtime));
	runtime->BooleanType.oper.add_operation = concat_operation;
	runtime->BooleanType.oper.subtract_operation = undefined_binary_operation;
	runtime->BooleanType.oper.multiply_operation = undefined_binary_operation;
	runtime->BooleanType.oper.divide_operation = undefined_binary_operation;
	runtime->BooleanType.oper.modulo_operation = undefined_binary_operation;
	runtime->BooleanType.oper.power_operation = undefined_binary_operation;
	runtime->BooleanType.oper.and_operation = Boolean_and;
	runtime->BooleanType.oper.or_operation = Boolean_or;
	runtime->BooleanType.oper.xor_operation = undefined_binary_operation;
	runtime->BooleanType.oper.shr_operation = undefined_binary_operation;
	runtime->BooleanType.oper.shl_operation = undefined_binary_operation;
	runtime->BooleanType.oper.compare = Boolean_compare;
	runtime->BooleanType.oper.not_operation = Boolean_not;
	runtime->BooleanType.oper.negate_operation = undefined_unary_operation;
	runtime->BooleanType.oper.toString = Boolean_toString;
	runtime->BooleanType.oper.readProperty = Boolean_readProperty;
	runtime->BooleanType.oper.hashCode = Common_hashCode;
	runtime->BooleanType.oper.readIndex = NULL;
	runtime->BooleanType.oper.writeIndex = NULL;
	runtime->BooleanType.oper.removeIndex = NULL;
	runtime->BooleanType.oper.subseq = NULL;
	runtime->BooleanType.oper.iterator = NULL;
}
