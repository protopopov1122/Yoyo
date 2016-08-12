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

#include "types.h"

int8_t compare_numbers(YValue* v1, YValue* v2, YThread* th) {
	if (!CHECK_TYPES(IntegerT, FloatT, v1, v2))
		return -2;
	if (CHECK_TYPE(FloatT, v1, v2)) {
		double i1 = getFloat(v1);
		double i2 = getFloat(v2);
		return (i1 < i2) ? -1 : (i1 > i2);
	} else {
		int64_t i1 = getInteger(v1);
		int64_t i2 = getInteger(v2);
		return (i1 < i2) ? -1 : (i1 > i2);
	}
}

int number_compare(YValue* v1, YValue* v2, YThread* th) {
	int8_t res = compare_numbers(v1, v2, th);
	int8_t out = 0;
	if (res == 0) {
		out |= COMPARE_EQUALS;
		out |= COMPARE_PTR_EQUALS;
	}
	if (res != 0)
		out |= COMPARE_NOT_EQUALS;
	if (res == -1)
		out |= COMPARE_LESSER;
	if (res == 1)
		out |= COMPARE_GREATER;
	if (res < 1)
		out |= COMPARE_LESSER_OR_EQUALS;
	if (res > -1)
		out |= COMPARE_GREATER_OR_EQUALS;
	if (res > 1 || res < -1)
		out |= COMPARE_NOT_EQUALS;
	return out;
}

YValue* number_add(YValue* v1, YValue* v2, YThread* th) {
	if (!CHECK_TYPES(IntegerT, FloatT, v1, v2))
		return concat_operation(v1, v2, th);
	else if (CHECK_TYPE(FloatT, v1, v2)) {
		double d1 = getFloat(v1);
		double d2 = getFloat(v2);
		return newFloat(d1 + d2, th);
	} else {
		int64_t i1 = getInteger(v1);
		int64_t i2 = getInteger(v2);
		return newInteger(i1 + i2, th);
	}
}

YValue* number_sub(YValue* v1, YValue* v2, YThread* th) {
	if (!CHECK_TYPES(IntegerT, FloatT, v1, v2))
		return getNull(th);
	if (CHECK_TYPE(FloatT, v1, v2)) {
		double d1 = getFloat(v1);
		double d2 = getFloat(v2);
		return newFloat(d1 - d2, th);
	} else {
		int64_t i1 = getInteger(v1);
		int64_t i2 = getInteger(v2);
		return newInteger(i1 - i2, th);
	}
}

YValue* number_mul(YValue* v1, YValue* v2, YThread* th) {
	if (!CHECK_TYPES(IntegerT, FloatT, v1, v2))
		return getNull(th);
	if (CHECK_TYPE(FloatT, v1, v2)) {
		double d1 = getFloat(v1);
		double d2 = getFloat(v2);
		return newFloat(d1 * d2, th);
	} else {
		int64_t i1 = getInteger(v1);
		int64_t i2 = getInteger(v2);
		return newInteger(i1 * i2, th);
	}
}

YValue* number_div(YValue* v1, YValue* v2, YThread* th) {
	if (!CHECK_TYPES(IntegerT, FloatT, v1, v2))
		return getNull(th);
	if (CHECK_TYPE(FloatT, v1, v2)) {
		double d1 = getFloat(v1);
		double d2 = getFloat(v2);
		return newFloat(d1 / d2, th);
	} else {
		int64_t i1 = getInteger(v1);
		int64_t i2 = getInteger(v2);
		if (i2==0) {
				throwException(L"DivisionByZero", NULL, 0, th);
				return getNull(th);
		}
		return newInteger(i1 / i2, th);
	}
}

YValue* number_mod(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type->type == IntegerT && v2->type->type == IntegerT) {
		int64_t i1 = getInteger(v1);
		int64_t i2 = getInteger(v2);
		if (i2==0) {
				throwException(L"DivisionByZero", NULL, 0, th);
				return getNull(th);
		}
		return newInteger(i1 % i2, th);
	} else
		return getNull(th);
}

YValue* number_pow(YValue* v1, YValue* v2, YThread* th) {
	if (!CHECK_TYPES(IntegerT, FloatT, v1, v2))
		return getNull(th);
	if (CHECK_TYPE(FloatT, v1, v2)) {
		double d1 = getFloat(v1);
		double d2 = getFloat(v2);
		return newFloat(pow(d1, d2), th);
	} else {
		int64_t i1 = getInteger(v1);
		int64_t i2 = getInteger(v2);
		return newInteger(pow(i1, i2), th);
	}
}

YValue* number_shr(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type->type == IntegerT && v2->type->type == IntegerT) {
		int64_t i1 = getInteger(v1);
		int64_t i2 = getInteger(v2);
		return newInteger(i1 >> i2, th);
	} else
		return getNull(th);
}

YValue* number_shl(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type->type == IntegerT && v2->type->type == IntegerT) {
		int64_t i1 = getInteger(v1);
		int64_t i2 = getInteger(v2);
		return newInteger(i1 << i2, th);
	} else
		return getNull(th);
}

YValue* number_and(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type->type == IntegerT && v2->type->type == IntegerT) {
		int64_t i1 = getInteger(v1);
		int64_t i2 = getInteger(v2);
		return newInteger(i1 & i2, th);
	} else
		return getNull(th);
}

YValue* number_or(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type->type == IntegerT && v2->type->type == IntegerT) {
		int64_t i1 = getInteger(v1);
		int64_t i2 = getInteger(v2);
		return newInteger(i1 | i2, th);
	} else
		return getNull(th);
}

YValue* number_xor(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type->type == IntegerT && v2->type->type == IntegerT) {
		int64_t i1 = getInteger(v1);
		int64_t i2 = getInteger(v2);
		return newInteger(i1 ^ i2, th);
	} else
		return getNull(th);
}
YValue* number_neg(YValue* v, YThread* th) {
	if (v->type->type == IntegerT)
		return newInteger(-((YInteger*) v)->value, th);
	else if (v->type->type == FloatT)
		return newFloat(-((YFloat*) v)->value, th);

	return getNull(th);
}
YValue* number_not(YValue* v, YThread* th) {
	if (v->type->type == IntegerT)
		return newInteger(~((YInteger*) v)->value, th);
	else
		return getNull(th);
}
wchar_t* Int_toString(YValue* data, YThread* th) {
	int64_t i = ((YInteger*) data)->value;
	int tsize = snprintf(0, 0, "%"PRId64, i) + 1;
	char* out = malloc(sizeof(char) * tsize);
	sprintf(out, "%"PRId64, i);
	int32_t size = sizeof(wchar_t) * tsize;
	wchar_t* dst = malloc(size);
	mbstowcs(dst, out, size);
	free(out);
	return dst;
}
wchar_t* Float_toString(YValue* data, YThread* th) {
	double i = ((YFloat*) data)->value;
	int tsize = snprintf(0, 0, "%lf", i) + 1;
	char* out = malloc(sizeof(char) * tsize);
	sprintf(out, "%lf", i);
	int32_t size = sizeof(wchar_t) * tsize;
	wchar_t* dst = malloc(size);
	mbstowcs(dst, out, size);
	free(out);
	return dst;
}
bool Int_equals(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type->type != IntegerT || v2->type->type != IntegerT)
		return false;
	return ((YInteger*) v1)->value == ((YInteger*) v2)->value;
}
bool Float_equals(YValue* v1, YValue* v2, YThread* th) {
	if (v1->type->type != FloatT || v2->type->type != FloatT)
		return false;
	return ((YFloat*) v1)->value == ((YFloat*) v2)->value;
}

#define INT_INIT NativeLambda* nlam = (NativeLambda*) lambda;\
                int64_t value = ((YInteger*) nlam->object)->value;
#define INT_INIT2 NativeLambda* nlam = (NativeLambda*) lambda;\
                    YValue* yvalue = (YValue*) nlam->object;

YOYO_FUNCTION(_Int_toFloat) {
	INT_INIT
	;
	return newFloat((double) value, th);
}
YOYO_FUNCTION(_Int_toString) {
	INT_INIT2
	;
	wchar_t* wstr = toString(yvalue, th);
	YValue* out = newString(wstr, th);
	free(wstr);
	return out;
}
YOYO_FUNCTION(_Int_toHexString) {
	INT_INIT
	;
	const char* fmt = "%"PRIx64;
	size_t sz = snprintf(NULL, 0, fmt, value);
	char* cstr = malloc(sizeof(char) * (sz + 1));
	sprintf(cstr, fmt, value);
	cstr[sz] = '\0';
	wchar_t* wstr = malloc(sizeof(wchar_t) * (sz + 1));
	mbstowcs(wstr, cstr, sz);
	wstr[sz] = L'\0';
	free(cstr);
	YValue* out = newString(wstr, th);
	free(wstr);
	return out;
}
YOYO_FUNCTION(_Int_toBinString) {
	INT_INIT
	;
	StringBuilder* sb = newStringBuilder(L"");
	int i = 63;
	while (((value >> i) & 1) == 0 && i > 0)
		i--;
	for (; i >= 0; i--) {
		int v = (value >> i) & 1;
		if (v)
			sb->append(sb, L"1");
		else
			sb->append(sb, L"0");
	}
	YValue* out = newString(sb->string, th);
	sb->free(sb);
	return out;
}
YOYO_FUNCTION(_Int_rotate) {
	INT_INIT
	;
	if (args[0]->type->type != IntegerT)
		return getNull(th);
	int64_t rot = ((YInteger*) args[0])->value;
	if (rot > 0) {
		int64_t lowest = 0;
		for (int64_t i = 0; i < rot; i++) {
			lowest <<= 1;
			lowest |= (value >> i) & 1;
		}
		value >>= rot;
		int h = 63;
		while (((value >> h) & 1) == 0)
			h--;
		lowest <<= h + 1;
		value |= lowest;
	} else if (rot < 0) {
		rot = -rot;
		int offset = 63;
		while (((value >> offset) & 1) == 0)
			offset--;
		int64_t highest = 0;
		for (int i = offset; i > offset - rot; i--) {
			highest <<= 1;
			highest |= (value >> i) & 1;
		}
		value <<= rot;
		value |= highest;
		int64_t out = 0;
		for (int i = offset; i >= 0; i--) {
			out <<= 1;
			out |= (value >> i) & 1;
		}
		value = out;
	}
	return newInteger(value, th);
}

YOYO_FUNCTION(_Int_absolute) {
	INT_INIT
	;
	return newInteger(value < 0 ? -value : value, th);
}

YOYO_FUNCTION(_Int_sign) {
	INT_INIT
	;
	return newInteger(value == 0 ? 0 : value < 0 ? -1 : 1, th);
}

YOYO_FUNCTION(_Int_getBit) {
	INT_INIT
	;
	if (args[0]->type->type != IntegerT)
		return getNull(th);
	int64_t offset = ((YInteger*) args[0])->value;
	if (offset < 0)
		return getNull(th);
	return newBoolean(((value >> offset) & 1) == 1, th);
}

YOYO_FUNCTION(_Int_setBit) {
	INT_INIT
	;
	if (args[0]->type->type != IntegerT || args[1]->type->type != BooleanT)
		return getNull(th);
	int64_t offset = ((YInteger*) args[0])->value;
	int val = ((YBoolean*) args[1])->value ? 1 : 0;
	if (offset < 0)
		return getNull(th);
	return newInteger(value | (val << offset), th);
}

YOYO_FUNCTION(_Int_clearBit) {
	INT_INIT
	;
	if (args[0]->type->type != IntegerT)
		return getNull(th);
	int64_t offset = ((YInteger*) args[0])->value;
	if (offset < 0)
		return getNull(th);
	return newInteger(value & ~(1 << offset), th);
}

YOYO_FUNCTION(_Int_getBitCount) {
	INT_INIT
	;
	int count = 63;
	while (((value >> count) & 1) == 0)
		count--;
	return newInteger(count, th);
}

YOYO_FUNCTION(_Int_asFloat) {
	INT_INIT
	;
	union {
		int64_t i;
		double f;
	} conv;
	conv.i = value;
	return newFloat(conv.f, th);
}

YOYO_FUNCTION(Int_toChar) {
	INT_INIT
	;
	wchar_t wstr[] = { (wchar_t) value, L'\0' };
	return newString(wstr, th);
}

#undef INT_INIT
#undef INT_INIT2

#define FLOAT_INIT NativeLambda* nlam = (NativeLambda*) lambda;\
                double value = ((YFloat*) nlam->object)->value;
#define FLOAT_INIT2 NativeLambda* nlam = (NativeLambda*) lambda;\
                    YValue* yvalue = (YValue*) nlam->object;

YOYO_FUNCTION(_Float_toInteger) {
	FLOAT_INIT
	;
	return newInteger((int64_t) value, th);
}
YOYO_FUNCTION(_Float_isNaN) {
	FLOAT_INIT
	;
	return newBoolean(isnan(value), th);
}
YOYO_FUNCTION(_Float_isInfinity) {
	FLOAT_INIT
	;
	return newBoolean(!isfinite(value) && !isnan(value), th);
}
YOYO_FUNCTION(_Float_toIntBits) {
	FLOAT_INIT
	;
	union {
		int64_t i;
		double f;
	} conv;
	conv.f = value;
	return newInteger(conv.i, th);
}

#undef FLOAT_INIT
#undef FLOAT_INIT2

YValue* Int_readProperty(int32_t key, YValue* v, YThread* th) {
	NEW_METHOD(L"float", _Int_toFloat, 0, v);
	NEW_METHOD(L"hex", _Int_toHexString, 0, v);
	NEW_METHOD(L"bin", _Int_toBinString, 0, v);
	NEW_METHOD(L"rotate", _Int_rotate, 1, v);
	NEW_METHOD(L"abs", _Int_absolute, 0, v);
	NEW_METHOD(L"sign", _Int_sign, 0, v);
	NEW_METHOD(L"bit", _Int_getBit, 1, v);
	NEW_METHOD(L"set", _Int_setBit, 2, v);
	NEW_METHOD(L"clear", _Int_clearBit, 1, v);
	NEW_METHOD(L"bitCount", _Int_getBitCount, 0, v);
	NEW_METHOD(L"asFloat", _Int_asFloat, 0, v);
	NEW_METHOD(L"toChar", Int_toChar, 0, v);
	return Common_readProperty(key, v, th);
}
YValue* Float_readProperty(int32_t key, YValue* v, YThread* th) {
	NEW_METHOD(L"int", _Float_toInteger, 0, v);
	NEW_METHOD(L"isNaN", _Float_isNaN, 0, v);
	NEW_METHOD(L"isInfinity", _Float_isInfinity, 0, v);
	NEW_METHOD(L"asInt", _Float_toIntBits, 0, v);
	return Common_readProperty(key, v, th);
}
uint64_t Int_hashCode(YValue* v, YThread* th) {
	union {
		uint64_t u64;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) v)->value;
	return un.u64;
}
uint64_t Float_hashCode(YValue* v, YThread* th) {
	union {
		uint64_t u64;
		double fp64;
	} un;
	un.fp64 = ((YFloat*) v)->value;
	return un.u64;
}
void Int_type_init(YRuntime* runtime) {
	runtime->IntType.type = IntegerT;
	runtime->IntType.TypeConstant = newAtomicType(IntegerT,
			yoyo_thread(runtime));
	runtime->IntType.oper.add_operation = number_add;
	runtime->IntType.oper.subtract_operation = number_sub;
	runtime->IntType.oper.multiply_operation = number_mul;
	runtime->IntType.oper.divide_operation = number_div;
	runtime->IntType.oper.modulo_operation = number_mod;
	runtime->IntType.oper.power_operation = number_pow;
	runtime->IntType.oper.and_operation = number_and;
	runtime->IntType.oper.or_operation = number_or;
	runtime->IntType.oper.xor_operation = number_xor;
	runtime->IntType.oper.shr_operation = number_shr;
	runtime->IntType.oper.shl_operation = number_shl;
	runtime->IntType.oper.negate_operation = number_neg;
	runtime->IntType.oper.not_operation = number_not;
	runtime->IntType.oper.compare = number_compare;
	runtime->IntType.oper.toString = Int_toString;
	runtime->IntType.oper.readProperty = Int_readProperty;
	runtime->IntType.oper.hashCode = Int_hashCode;
	runtime->IntType.oper.readIndex = NULL;
	runtime->IntType.oper.writeIndex = NULL;
	runtime->IntType.oper.removeIndex = NULL;
	runtime->IntType.oper.subseq = NULL;
	runtime->IntType.oper.iterator = NULL;
}

void Float_type_init(YRuntime* runtime) {
	runtime->FloatType.type = FloatT;
	runtime->FloatType.TypeConstant = newAtomicType(FloatT,
			yoyo_thread(runtime));
	runtime->FloatType.oper.add_operation = number_add;
	runtime->FloatType.oper.subtract_operation = number_sub;
	runtime->FloatType.oper.multiply_operation = number_mul;
	runtime->FloatType.oper.divide_operation = number_div;
	runtime->FloatType.oper.modulo_operation = number_mod;
	runtime->FloatType.oper.power_operation = number_pow;
	runtime->FloatType.oper.and_operation = number_and;
	runtime->FloatType.oper.or_operation = number_or;
	runtime->FloatType.oper.xor_operation = number_xor;
	runtime->FloatType.oper.shr_operation = number_shr;
	runtime->FloatType.oper.shl_operation = number_shl;
	runtime->FloatType.oper.negate_operation = number_neg;
	runtime->FloatType.oper.not_operation = number_not;
	runtime->FloatType.oper.compare = number_compare;
	runtime->FloatType.oper.toString = Float_toString;
	runtime->FloatType.oper.readProperty = Float_readProperty;
	runtime->FloatType.oper.hashCode = Float_hashCode;
	runtime->FloatType.oper.readIndex = NULL;
	runtime->FloatType.oper.writeIndex = NULL;
	runtime->FloatType.oper.removeIndex = NULL;
	runtime->FloatType.oper.subseq = NULL;
	runtime->FloatType.oper.iterator = NULL;
}
