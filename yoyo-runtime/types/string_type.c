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

wchar_t* String_toString(YValue* v, YThread* th) {
	wchar_t* src = ((YString*) v)->value;
	wchar_t* out = malloc(sizeof(wchar_t) * (wcslen(src) + 1));
	wcscpy(out, src);
	out[wcslen(src)] = L'\0';
	return out;
}

int String_compare(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type->type != StringT || v2->type->type != StringT)
		return COMPARE_NOT_EQUALS;
	return wcscmp(((YString*) v1)->value, ((YString*) v2)->value) == 0 ?
			COMPARE_EQUALS : COMPARE_NOT_EQUALS;
}

YValue* String_mul(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type->type == StringT && v2->type->type == IntegerT) {
		wchar_t* wstr = ((YString*) v1)->value;
		StringBuilder* sb = newStringBuilder(wstr);
		uint32_t count = (uint32_t) ((YInteger*) v2)->value;
		for (uint32_t i = 1; i < count; i++)
			sb->append(sb, wstr);
		YValue* out = newString(sb->string, th);
		sb->free(sb);
		return out;
	} else
		return getNull(th);
}
typedef struct CharSequence {
	YArray array;

	YString* str;
} CharSequence;
void CharSequence_mark(YoyoObject* ptr) {
	ptr->marked = true;
	CharSequence* seq = (CharSequence*) ptr;
	MARK(seq->str);
}
void CharSequence_free(YoyoObject* ptr) {
	free(ptr);
}
size_t CharSequence_size(YArray* a, YThread* th) {
	CharSequence* seq = (CharSequence*) a;
	return (uint32_t) wcslen(seq->str->value);
}
YValue* CharSequence_get(YArray* a, size_t index, YThread* th) {
	CharSequence* seq = (CharSequence*) a;
	wchar_t* wstr = seq->str->value;
	if (index < wcslen(wstr)) {
		wchar_t out[] = { wstr[index], L'\0' };
		return newString(out, th);
	}
	return getNull(th);
}
void CharSequence_set(YArray* a, size_t index, YValue* v, YThread* th) {
	wchar_t* wstr = ((CharSequence*) a)->str->value;
	wchar_t* vwstr = toString(v, th);
	for (uint32_t i = 0; i < wcslen(vwstr); i++) {
		if (index + i >= wcslen(wstr))
			return;
		wstr[i + index] = vwstr[i];
	}
	free(vwstr);
}
void CharSequence_add(YArray* a, YValue* v, YThread* th) {
	wchar_t* vwstr = toString(v, th);
	wchar_t** wstr = &((CharSequence*) a)->str->value;
	*wstr = realloc(*wstr,
			sizeof(wchar_t) * (wcslen(*wstr) + wcslen(vwstr) + 1));
	wcscat(*wstr, vwstr);
	free(vwstr);
}
void CharSequence_insert(YArray* a, size_t index, YValue* v, YThread* th) {
	wchar_t** wstr = &((CharSequence*) a)->str->value;
	if (index >= wcslen(*wstr))
		return;
	size_t oldLen = wcslen(*wstr);
	wchar_t* vwstr = toString(v, th);
	*wstr = realloc(*wstr,
			sizeof(wchar_t) * (wcslen(*wstr) + wcslen(vwstr) + 1));
	for (uint32_t i = oldLen; i >= index; i--)
		(*wstr)[i + wcslen(vwstr)] = (*wstr)[i];
	for (uint32_t i = 0; i < wcslen(vwstr); i++)
		(*wstr)[i + index] = vwstr[i];
	free(vwstr);
}
void CharSequence_remove(YArray* a, size_t index, YThread* th) {
	wchar_t** wstr = &((CharSequence*) a)->str->value;
	for (uint32_t i = index; i < wcslen(*wstr); i++)
		(*wstr)[i] = (*wstr)[i + 1];
	*wstr = realloc(*wstr, sizeof(wchar_t) * wcslen(*wstr));
}

YArray* newCharSequence(YString* str, YThread* th) {
	CharSequence* cseq = malloc(sizeof(CharSequence));
	initYoyoObject((YoyoObject*) cseq, CharSequence_mark, CharSequence_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) cseq);
	cseq->array.parent.type = &th->runtime->ArrayType;

	cseq->str = str;

	cseq->array.size = CharSequence_size;
	cseq->array.get = CharSequence_get;
	cseq->array.set = CharSequence_set;
	cseq->array.add = CharSequence_add;
	cseq->array.insert = CharSequence_insert;
	cseq->array.remove = CharSequence_remove;
	cseq->array.toString = NULL;

	return (YArray*) cseq;
}

#define STR_INIT NativeLambda* nlam = (NativeLambda*) lambda;\
                YString* str = (YString*) nlam->object;

YOYO_FUNCTION(_String_charAt) {
	STR_INIT
	wchar_t* wstr = str->value;
	if (args[0]->type->type == IntegerT) {
		int64_t index = ((YInteger*) args[0])->value;
		if (index > -1 && index < wcslen(wstr)) {
			wchar_t out[] = { wstr[index], L'\0' };
			return newString(out, th);
		}
	}
	return getNull(th);
}
YOYO_FUNCTION(_String_chars) {
	STR_INIT
	return (YValue*) newCharSequence(str, th);
}
YOYO_FUNCTION(_String_substring) {
	STR_INIT
	wchar_t* wstr = str->value;
	if (args[0]->type->type == IntegerT && args[1]->type->type == IntegerT) {
		uint32_t start = ((YInteger*) args[0])->value;
		uint32_t end = ((YInteger*) args[1])->value;
		wchar_t* out = malloc(sizeof(wchar_t) * (end - start + 1));
		memcpy(out, &wstr[start], sizeof(wchar_t) * (end - start));
		out[end - start] = L'\0';
		YValue* value = newString(out, th);
		free(out);
		return value;
	}
	return getNull(th);
}
YOYO_FUNCTION(_String_toLowerCase) {
	STR_INIT
	wchar_t* wstr = str->value;
	wchar_t* lstr = malloc(sizeof(wchar_t) * (wcslen(wstr) + 1));
	for (uint32_t i = 0; i < wcslen(wstr); i++)
		lstr[i] = towlower(wstr[i]);
	lstr[wcslen(wstr)] = L'\0';
	YValue* out = newString(lstr, th);
	free(lstr);
	return out;
}
YOYO_FUNCTION(_String_toUpperCase) {
	STR_INIT
	wchar_t* wstr = str->value;
	wchar_t* lstr = malloc(sizeof(wchar_t) * (wcslen(wstr) + 1));
	for (uint32_t i = 0; i < wcslen(wstr); i++)
		lstr[i] = towupper(wstr[i]);
	lstr[wcslen(wstr)] = L'\0';
	YValue* out = newString(lstr, th);
	free(lstr);
	return out;
}
YOYO_FUNCTION(_String_trim) {
	STR_INIT
	wchar_t* wstr = str->value;
	size_t start = 0;
	while (iswspace(wstr[start]))
		start++;
	size_t end = wcslen(wstr) - 1;
	while (iswspace(wstr[end]))
		end--;
	end++;
	if (end - start == 0)
		return newString(L"", th);
	wchar_t* out = malloc(sizeof(wchar_t) * (end - start + 1));
	memcpy(out, &wstr[start], sizeof(wchar_t) * (end - start));
	out[end - start] = L'\0';
	YValue* vout = newString(out, th);
	free(out);
	return vout;
}
YOYO_FUNCTION(_String_startsWith) {
	STR_INIT
	wchar_t* wstr = str->value;
	if (args[0]->type->type == StringT) {
		wchar_t* str2 = ((YString*) args[0])->value;
		if (wcslen(str2) > wcslen(wstr))
			return newBoolean(false, th);
		for (uint32_t i = 0; i < wcslen(str2); i++)
			if (wstr[i] != str2[i])
				return newBoolean(false, th);
		return newBoolean(true, th);
	}
	return newBoolean(false, th);
}
YOYO_FUNCTION(_String_endsWith) {
	STR_INIT
	wchar_t* wstr = str->value;
	if (args[0]->type->type == StringT) {
		wchar_t* str2 = ((YString*) args[0])->value;
		if (wcslen(str2) > wcslen(wstr))
			return newBoolean(false, th);
		for (uint32_t i = 1; i <= wcslen(str2); i++)
			if (wstr[wcslen(wstr) - i] != str2[wcslen(str2) - i])
				return newBoolean(false, th);
		return newBoolean(true, th);
	}
	return newBoolean(false, th);
}

#undef STR_INIT
YValue* String_readProperty(int32_t key, YValue* val, YThread* th) {
	YString* str = (YString*) val;
	NEW_PROPERTY(L"length", newInteger(wcslen(str->value), th));
	NEW_METHOD(L"charAt", _String_charAt, 1, str);
	NEW_METHOD(L"chars", _String_chars, 0, str);
	NEW_METHOD(L"substring", _String_substring, 2, str);
	NEW_METHOD(L"toLowerCase", _String_toLowerCase, 0, str);
	NEW_METHOD(L"toUpperCase", _String_toUpperCase, 0, str);
	NEW_METHOD(L"trim", _String_trim, 0, str);
	NEW_METHOD(L"startsWith", _String_startsWith, 1, str);
	NEW_METHOD(L"endsWith", _String_endsWith, 1, str);
	return Common_readProperty(key, val, th);

}
uint64_t String_hashCode(YValue* v, YThread* th) {
	wchar_t* wstr = ((YString*) v)->value;
	uint64_t out = 0;
	for (size_t i = 0; i < wcslen(wstr); i++)
		out += wstr[i] * 31 ^ (wcslen(wstr) - 1 - i);
	return out;
}

YValue* String_readIndex(YValue* v, YValue* i, YThread* th) {
	wchar_t* wstr = ((YString*) v)->value;
	if (i->type->type == IntegerT) {
		size_t index = (size_t) ((YInteger*) i)->value;
		if (index < wcslen(wstr)) {
			wchar_t wstr2[] = { wstr[index], L'\0' };
			return newString(wstr2, th);
		}
	}
	return getNull(th);
}

YValue* String_subseq(YValue* v, size_t start, size_t end, YThread* th) {
	wchar_t* wstr = ((YString*) v)->value;
	wchar_t* out = malloc(sizeof(wchar_t) * (end - start + 1));
	memcpy(out, &wstr[start], sizeof(wchar_t) * (end - start));
	out[end - start] = L'\0';
	YValue* value = newString(out, th);
	free(out);
	return value;
}

void String_type_init(YRuntime* runtime) {
	runtime->StringType.type = StringT;
	runtime->StringType.TypeConstant = newAtomicType(StringT,
			runtime->CoreThread);
	runtime->StringType.oper.add_operation = concat_operation;
	runtime->StringType.oper.subtract_operation = undefined_binary_operation;
	runtime->StringType.oper.multiply_operation = String_mul;
	runtime->StringType.oper.divide_operation = undefined_binary_operation;
	runtime->StringType.oper.modulo_operation = undefined_binary_operation;
	runtime->StringType.oper.power_operation = undefined_binary_operation;
	runtime->StringType.oper.and_operation = undefined_binary_operation;
	runtime->StringType.oper.or_operation = undefined_binary_operation;
	runtime->StringType.oper.xor_operation = undefined_binary_operation;
	runtime->StringType.oper.shr_operation = undefined_binary_operation;
	runtime->StringType.oper.shl_operation = undefined_binary_operation;
	runtime->StringType.oper.compare = String_compare;
	runtime->StringType.oper.negate_operation = undefined_unary_operation;
	runtime->StringType.oper.not_operation = undefined_unary_operation;
	runtime->StringType.oper.toString = String_toString;
	runtime->StringType.oper.readProperty = String_readProperty;
	runtime->StringType.oper.hashCode = String_hashCode;
	runtime->StringType.oper.readIndex = String_readIndex;
	runtime->StringType.oper.writeIndex = NULL;
	runtime->StringType.oper.subseq = String_subseq;
	runtime->StringType.oper.iterator = NULL;
}
