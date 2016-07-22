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
YOYO_FUNCTION(_Common_toString) {
	INIT
	;
	wchar_t* wstr = toString(value, th);
	YValue* ystr = newString(wstr, th);
	free(wstr);
	return ystr;
}
YOYO_FUNCTION(_Common_hashCode) {
	INIT
	;
	return newInteger(value->type->oper.hashCode(value, th), th);
}
YOYO_FUNCTION(_Common_equals) {
	INIT
	;
	return newBoolean(CHECK_EQUALS(value, args[0], th), th);
}
YOYO_FUNCTION(Common_getType) {
	INIT
	;
	return (YValue*) value->type->TypeConstant;
}
#undef INIT

YValue* Common_readProperty(int32_t key, YValue* v, YThread* th) {
	NEW_METHOD(TO_STRING, _Common_toString, 0, v);
	NEW_METHOD(HASHCODE, _Common_hashCode, 0, v);
	NEW_METHOD(EQUALS, _Common_equals, 1, v);
	NEW_METHOD(L"type", Common_getType, 0, v);
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
