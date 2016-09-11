#include "yoyo.h"

YOYO_FUNCTION(YSTD_UNSAFE_PTR) {
	union {
		void* ptr;
		int64_t i64;
	} un;
	un.ptr = args[0];
	return newInteger(un.i64, th);
}

YOYO_FUNCTION(YSTD_UNSAFE_UNPTR) {
	union {
		void* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	return (YValue*) un.ptr;
}

YOYO_FUNCTION(YSTD_UNSAFE_ALLOC) {
	uint8_t* ptr = calloc(1, sizeof(uint8_t) * ((YInteger*) args[0])->value);
	union {
		void* ptr;
		int64_t i64;
	} un;
	un.ptr = ptr;
	return newInteger(un.i64, th);
}

YOYO_FUNCTION(YSTD_UNSAFE_FREE) {
	union {
		void* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	free(un.ptr);
	return getNull(th);
}

YOYO_FUNCTION(YSTD_UNSAFE_GET_I64) {
	union {
		int64_t* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	return newInteger(un.ptr[((YInteger*) args[1])->value], th);
}

YOYO_FUNCTION(YSTD_UNSAFE_SET_I64) {
	union {
		int64_t* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	un.ptr[((YInteger*) args[1])->value] = ((YInteger*) args[2])->value;
	return getNull(th);
}

YOYO_FUNCTION(YSTD_UNSAFE_GET_I32) {
	union {
		int32_t* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	return newInteger(un.ptr[((YInteger*) args[1])->value], th);
}

YOYO_FUNCTION(YSTD_UNSAFE_SET_I32) {
	union {
		int32_t* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	un.ptr[((YInteger*) args[1])->value] = (int32_t) ((YInteger*) args[2])->value;
	return getNull(th);
}

YOYO_FUNCTION(YSTD_UNSAFE_GET_I16) {
	union {
		int16_t* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	return newInteger(un.ptr[((YInteger*) args[1])->value], th);
}

YOYO_FUNCTION(YSTD_UNSAFE_SET_I16) {
	union {
		int16_t* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	un.ptr[((YInteger*) args[1])->value] = (int16_t) ((YInteger*) args[2])->value;
	return getNull(th);
}

YOYO_FUNCTION(YSTD_UNSAFE_GET_I8) {
	union {
		int8_t* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	return newInteger(un.ptr[((YInteger*) args[1])->value], th);
}

YOYO_FUNCTION(YSTD_UNSAFE_SET_I8) {
	union {
		int8_t* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	un.ptr[((YInteger*) args[1])->value] = (int8_t) ((YInteger*) args[2])->value;
	return getNull(th);
}

YOYO_FUNCTION(YSTD_UNSAFE_GET_FLOAT) {
	union {
		float* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	return newFloat(un.ptr[((YInteger*) args[1])->value], th);
}

YOYO_FUNCTION(YSTD_UNSAFE_SET_FLOAT) {
	union {
		float* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	un.ptr[((YInteger*) args[1])->value] = (float) ((YFloat*) args[2])->value;
	return getNull(th);
}

YOYO_FUNCTION(YSTD_UNSAFE_GET_DOUBLE) {
	union {
		double* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	return newFloat(un.ptr[((YInteger*) args[1])->value], th);
}

YOYO_FUNCTION(YSTD_UNSAFE_SET_DOUBLE) {
	union {
		double* ptr;
		int64_t i64;
	} un;
	un.i64 = ((YInteger*) args[0])->value;
	un.ptr[((YInteger*) args[1])->value] = ((YFloat*) args[2])->value;
	return getNull(th);
}
