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

#include "../../yoyo/types/types.h"

wchar_t* Lambda_toString(YValue* obj, YThread* th) {
	YLambda* l = (YLambda*) obj;
	wchar_t* src = l->signature(l, th)->string;
	wchar_t* out = calloc(1, sizeof(wchar_t) * (wcslen(src) + 1));
	wcscpy(out, src);
	return out;
}

bool Lambda_equals(YValue* v1, YValue* v2, YThread* th) {
	return newBoolean(v1 == v2, th);
}

YOYO_FUNCTION(Lambda_signature) {
	YLambda* lmbd = (YLambda*) ((NativeLambda*) l)->object;
	return (YValue*) lmbd->signature(lmbd, th);
}

YValue* Lambda_readProperty(int32_t key, YValue* v, YThread* th) {
	NEW_METHOD(L"signature", Lambda_signature, 0, v);
	return Common_readProperty(key, v, th);
}

void Lambda_type_init(YRuntime* runtime) {
	runtime->LambdaType.type = LambdaT;
	runtime->LambdaType.TypeConstant = newAtomicType(LambdaT,
			runtime->CoreThread);
	runtime->LambdaType.oper.add_operation = concat_operation;
	runtime->LambdaType.oper.subtract_operation = undefined_binary_operation;
	runtime->LambdaType.oper.multiply_operation = undefined_binary_operation;
	runtime->LambdaType.oper.divide_operation = undefined_binary_operation;
	runtime->LambdaType.oper.modulo_operation = undefined_binary_operation;
	runtime->LambdaType.oper.power_operation = undefined_binary_operation;
	runtime->LambdaType.oper.and_operation = undefined_binary_operation;
	runtime->LambdaType.oper.or_operation = undefined_binary_operation;
	runtime->LambdaType.oper.xor_operation = undefined_binary_operation;
	runtime->LambdaType.oper.shr_operation = undefined_binary_operation;
	runtime->LambdaType.oper.shl_operation = undefined_binary_operation;
	runtime->LambdaType.oper.compare = compare_operation;
	runtime->LambdaType.oper.negate_operation = undefined_unary_operation;
	runtime->LambdaType.oper.not_operation = undefined_unary_operation;
	runtime->LambdaType.oper.toString = Lambda_toString;
	runtime->LambdaType.oper.readProperty = Lambda_readProperty;
	runtime->LambdaType.oper.hashCode = Common_hashCode;
	runtime->LambdaType.oper.readIndex = NULL;
	runtime->LambdaType.oper.writeIndex = NULL;
	runtime->LambdaType.oper.subseq = NULL;
	runtime->LambdaType.oper.iterator = NULL;
}
