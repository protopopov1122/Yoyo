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

YValue* undefined_binary_operation(YValue* v1, YValue* v2, YThread* th) {
	wchar_t* args[] = { v1->type->TypeConstant->string,
			v2->type->TypeConstant->string };
	throwException(L"UndefinedBinaryOperation", args, 2, th);
	return getNull(th);
}
YValue* undefined_unary_operation(YValue*v1, YThread* th) {
	wchar_t* args[] = { v1->type->TypeConstant->string };
	throwException(L"UndefinedUnaryOperation", args, 1, th);
	return getNull(th);
}
YValue* concat_operation(YValue* v1, YValue* v2, YThread* th) {
	wchar_t* wstr1 = toString(v1, th);
	wchar_t* wstr2 = toString(v2, th);
	wchar_t* wstr = malloc(
			sizeof(wchar_t) * (wcslen(wstr1) + wcslen(wstr2) + 1));
	wcscpy(wstr, wstr1);
	wcscat(wstr, wstr2);
	wstr[wcslen(wstr1) + wcslen(wstr2)] = L'\0';
	YValue* ystr = newString(wstr, th);
	free(wstr1);
	free(wstr2);
	free(wstr);
	return ystr;
}
int compare_operation(YValue* arg1, YValue* arg2, YThread* th) {
	return arg1 == arg2 ?
	COMPARE_PTR_EQUALS | COMPARE_EQUALS :
							COMPARE_NOT_EQUALS;
}

#define INIT NativeLambda* nlam = (NativeLambda*) lambda;\
                    YValue* value = (YValue*) nlam->object;
YOYO_FUNCTION(Common_property_toString) {
	INIT
	;
	wchar_t* wstr = toString(value, th);
	YValue* ystr = newString(wstr, th);
	free(wstr);
	return ystr;
}
YOYO_FUNCTION(Common_property_hashCode) {
	INIT
	;
	return newInteger(value->type->oper.hashCode(value, th), th);
}
YOYO_FUNCTION(Common_property_equals) {
	INIT
	;
	return newBoolean(CHECK_EQUALS(value, args[0], th), th);
}
YOYO_FUNCTION(Common_property_getType) {
	INIT
	;
	return (YValue*) value->type->TypeConstant;
}
#undef INIT

YValue* Common_readProperty(int32_t key, YValue* v, YThread* th) {
	if (key==YOYOID(L"prototype", th)) {
		if (v->type->prototype==NULL)
			v->type->prototype = th->runtime->newObject(NULL, th);
		return (YValue*) v->type->prototype;
	}
	if (v->type->prototype!=NULL
		&&v->type->prototype->contains(v->type->prototype, key, th)) {
		return v->type->prototype->get(v->type->prototype, key, th);
	}
	NEW_METHOD(TO_STRING, Common_property_toString, 0, v);
	NEW_METHOD(HASHCODE, Common_property_hashCode, 0, v);
	NEW_METHOD(EQUALS, Common_property_equals, 1, v);
	NEW_METHOD(L"type", Common_property_getType, 0, v);
	return NULL;
}

uint64_t Common_hashCode(YValue* v, YThread* th) {
	union {
		uint64_t u64;
		void* ptr;
	} un;
	un.ptr = v;
	return un.u64;
}

wchar_t* Common_toString(YValue* v, YThread* th) {
	const char* fmt = "%ls@%p";
	size_t size = snprintf(NULL, 0, fmt, v->type->wstring, (void*) v);
	char* mbs = calloc(size+1, sizeof(char));
	sprintf(mbs, fmt, v->type->wstring, (void*) v);
	wchar_t* wcs = calloc(strlen(mbs) + 1, sizeof(wchar_t));
	mbstowcs(wcs, mbs, strlen(mbs));
	free(mbs);
	return wcs;
}

void Type_init(YType* type, YThread* th) {
		type->wstring = NULL;
		type->TypeConstant = NULL;
		type->oper.add_operation = undefined_binary_operation;
		type->oper.subtract_operation = undefined_binary_operation;
		type->oper.multiply_operation = undefined_binary_operation;
		type->oper.divide_operation = undefined_binary_operation;
		type->oper.modulo_operation = undefined_binary_operation;
		type->oper.power_operation = undefined_binary_operation;
		type->oper.and_operation = undefined_binary_operation;
		type->oper.or_operation = undefined_binary_operation;
		type->oper.xor_operation = undefined_binary_operation;
		type->oper.shl_operation = undefined_binary_operation;
		type->oper.shr_operation = undefined_binary_operation;
		type->oper.not_operation = undefined_unary_operation;
		type->oper.negate_operation = undefined_unary_operation;
		type->oper.readIndex = NULL;
		type->oper.removeIndex = NULL;
		type->oper.writeIndex = NULL; 
		type->oper.subseq = NULL;
		type->oper.hashCode = Common_hashCode;
		type->oper.compare = compare_operation;
		type->oper.toString = Common_toString;
		type->oper.readProperty = Common_readProperty;
		type->oper.iterator = NULL;

}
