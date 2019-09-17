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

#ifndef YOYO_PARSER_BASE_H_
#define YOYO_PARSER_BASE_H_

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
#include "util.h"

typedef struct yconstant_t {
	enum {
		Int64Constant, Fp64Constant, WcsConstant, BoolConstant
	} type;
	union {
		int64_t i64;
		double fp64;
		bool boolean;
		wchar_t* wcs;
	} value;
} yconstant_t;

#endif
