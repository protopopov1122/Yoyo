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

wchar_t* Decl_toString(YValue* val, YThread* th) {
	wchar_t* wcs = ((YoyoType*) val)->string;
	wchar_t* out = malloc(sizeof(wchar_t) * (wcslen(wcs) + 1));
	wcscpy(out, wcs);
	return out;
}

YOYO_FUNCTION(Decl_check) {
	YoyoType* type = (YoyoType*) ((NativeLambda*) l)->object;
	for (size_t i = 0; i < argc; i++)
		if (!type->verify(type, args[i], th))
			return newBoolean(false, th);
	return newBoolean(true, th);
}

YOYO_FUNCTION(Decl_verify) {
	YoyoType* type = (YoyoType*) ((NativeLambda*) l)->object;
	for (size_t i = 0; i < argc; i++)
		if (!type->verify(type, args[i], th)) {
			throwException(L"TypeError", NULL, 0, th);
			return getNull(th);
		}
	return getNull(th);
}

YOYO_FUNCTION(Decl_not_null) {
	YoyoType* type = (YoyoType*) ((NativeLambda*) l)->object;
	return (YValue*) newNotNullType(type, th);
}

YOYO_FUNCTION(Decl_setName) {
	YoyoType* type = (YoyoType*) ((NativeLambda*) l)->object;
	wchar_t* name = toString(args[0], th);
	free(type->string);
	type->string = name;
	return (YValue*) type;
}

YValue* Decl_readProperty(int32_t key, YValue* val, YThread* th) {
	NEW_METHOD(L"check", Decl_check, -1, val);
	NEW_METHOD(L"verify", Decl_verify, -1, val);
	NEW_METHOD(L"notNull", Decl_not_null, 0, val);
	NEW_METHOD(L"name", Decl_setName, 1, val);
	return Common_readProperty(key, val, th);
}

YValue* Decl_or(YValue* v1, YValue* v2, YThread* th) {
	YoyoType* t1 = NULL;
	YoyoType* t2 = NULL;
	if (v1->type->type == DeclarationT)
		t1 = (YoyoType*) v1;
	else
		t1 = v1->type->TypeConstant;
	if (v2->type->type == DeclarationT)
		t2 = (YoyoType*) v2;
	else
		t2 = v2->type->TypeConstant;
	YoyoType* arr[] = { t1, t2 };
	return (YValue*) newTypeMix(arr, 2, th);
}

void Declaration_type_init(YRuntime* runtime) {
	runtime->DeclarationType.type = DeclarationT;
	runtime->DeclarationType.TypeConstant = newAtomicType(DeclarationT,
			runtime->CoreThread);
	runtime->DeclarationType.oper.add_operation = concat_operation;
	runtime->DeclarationType.oper.subtract_operation = undefined_binary_operation;
	runtime->DeclarationType.oper.multiply_operation = undefined_binary_operation;
	runtime->DeclarationType.oper.divide_operation = undefined_binary_operation;
	runtime->DeclarationType.oper.modulo_operation = undefined_binary_operation;
	runtime->DeclarationType.oper.power_operation = undefined_binary_operation;
	runtime->DeclarationType.oper.and_operation = undefined_binary_operation;
	runtime->DeclarationType.oper.or_operation = Decl_or;
	runtime->DeclarationType.oper.xor_operation = undefined_binary_operation;
	runtime->DeclarationType.oper.shr_operation = undefined_binary_operation;
	runtime->DeclarationType.oper.shl_operation = undefined_binary_operation;
	runtime->DeclarationType.oper.compare = compare_operation;
	runtime->DeclarationType.oper.negate_operation = undefined_unary_operation;
	runtime->DeclarationType.oper.not_operation = undefined_unary_operation;
	runtime->DeclarationType.oper.toString = Decl_toString;
	runtime->DeclarationType.oper.readProperty = Decl_readProperty;
	runtime->DeclarationType.oper.hashCode = Common_hashCode;
	runtime->DeclarationType.oper.readIndex = NULL;
	runtime->DeclarationType.oper.writeIndex = NULL;
	runtime->DeclarationType.oper.subseq = NULL;
	runtime->DeclarationType.oper.iterator = NULL;
}
