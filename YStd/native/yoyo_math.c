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

#include "yoyo-runtime.h"

YOYO_FUNCTION(YSTD_MATH_ACOS) {
	return newFloat(acos(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_ASIN) {
	return newFloat(asin(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_ATAN) {
	return newFloat(atan(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_ATAN2) {
	return newFloat(atan2(getFloat(args[0], th), getFloat(args[1], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_COS) {
	return newFloat(cos(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_COSH) {
	return newFloat(cosh(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_SIN) {
	return newFloat(sin(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_SINH) {
	return newFloat(sinh(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_TANH) {
	return newFloat(tanh(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_EXP) {
	return newFloat(exp(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_LDEXP) {
	return newFloat(ldexp(getFloat(args[0], th), getInteger(args[1], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_LOG) {
	return newFloat(log(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_LOG10) {
	return newFloat(log10(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_POW) {
	return newFloat(pow(getFloat(args[0], th), getFloat(args[1], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_SQRT) {
	return newFloat(sqrt(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_CEIL) {
	return newFloat(ceil(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_FABS) {
	return newFloat(fabs(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_FLOOR) {
	return newFloat(floor(getFloat(args[0], th)), th);
}

YOYO_FUNCTION(YSTD_MATH_FMOD) {
	return newFloat(fmod(getFloat(args[0], th), getFloat(args[1], th)), th);
}
