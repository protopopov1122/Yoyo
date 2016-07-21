#ifndef HEADERS_BASE_H_
#define HEADERS_BASE_H_


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include <math.h>
#include <locale.h>
#include <limits.h>
#include <unistd.h>

typedef struct yconstant_t {
	enum {
		Int64Constant, Fp64Constant,
		WcsConstant, BoolConstant
	} type;
	union {
		int64_t i64;
		double fp64;
		bool boolean;
		wchar_t* wcs;
	} value;
} yconstant_t;

#endif
