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

#ifndef YOYO_RUNTIME_ARRAY_H
#define YOYO_RUNTIME_ARRAY_H

#include "collections.h"
#include "value.h"

void Array_addAll(YArray*, YArray*, YThread*);
void Array_insertAll(YArray*, YArray*, size_t, YThread*);
YArray* Array_slice(YArray*, size_t, size_t, YThread*);
YArray* Array_flat(YArray*, YThread*);
void Array_each(YArray*, YLambda*, YThread*);
YArray* Array_map(YArray*, YLambda*, YThread*);
YValue* Array_reduce(YArray*, YLambda*, YValue*, YThread*);
YArray* Array_reverse(YArray*, YThread*);
YArray* Array_filter(YArray*, YLambda*, YThread*);
YArray* Array_compact(YArray*, YThread*);
YArray* Array_unique(YArray*, YThread*);
YArray* Array_sort(YArray*, YLambda*, YThread*);
YArray* Array_find(YArray*, YValue*, YThread*);
YArray* Array_tuple(YArray*, YThread*);
YoyoIterator* Array_iter(YArray* array, YThread* th);

#endif
