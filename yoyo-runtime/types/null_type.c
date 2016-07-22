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

#include "types/types.h"

wchar_t* Null_toString(YValue* v, YThread* th) {
	wchar_t* wstr = L"null";
	wchar_t* out = calloc(1, sizeof(wchar_t) * (wcslen(wstr) + 1));
	wcscpy(out, wstr);
	return out;
}

YValue* Null_readProperty(int32_t id, YValue* v, YThread* th) {
	return Common_readProperty(id, v, th);
}

int Null_compare(YValue* v1, YValue* v2, YThread* th) {
	return v1->type->type == AnyT && v2->type->type == AnyT ?
			COMPARE_EQUALS : COMPARE_NOT_EQUALS;
}

void Null_type_init(YRuntime* runtime) {
	runtime->NullType.type = AnyT;
	runtime->NullType.TypeConstant = newAtomicType(AnyT, runtime->CoreThread);
	runtime->NullType.oper.add_operation = concat_operation;
	runtime->NullType.oper.subtract_operation = undefined_binary_operation;
	runtime->NullType.oper.multiply_operation = undefined_binary_operation;
	runtime->NullType.oper.divide_operation = undefined_binary_operation;
	runtime->NullType.oper.modulo_operation = undefined_binary_operation;
	runtime->NullType.oper.power_operation = undefined_binary_operation;
	runtime->NullType.oper.and_operation = undefined_binary_operation;
	runtime->NullType.oper.or_operation = undefined_binary_operation;
	runtime->NullType.oper.xor_operation = undefined_binary_operation;
	runtime->NullType.oper.shr_operation = undefined_binary_operation;
	runtime->NullType.oper.shl_operation = undefined_binary_operation;
	runtime->NullType.oper.compare = Null_compare;
	runtime->NullType.oper.negate_operation = undefined_unary_operation;
	runtime->NullType.oper.not_operation = undefined_unary_operation;
	runtime->NullType.oper.toString = Null_toString;
	runtime->NullType.oper.readProperty = Null_readProperty;
	runtime->NullType.oper.hashCode = Common_hashCode;
	runtime->NullType.oper.readIndex = NULL;
	runtime->NullType.oper.writeIndex = NULL;
	runtime->NullType.oper.subseq = NULL;
	runtime->NullType.oper.iterator = NULL;
}
