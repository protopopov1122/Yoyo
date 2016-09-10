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

#ifndef YOYO_RUNTIME_WRAPPERS_H
#define YOYO_RUNTIME_WRAPPERS_H

#include "value.h"
#include "set.h"
#include "map.h"

YObject* newReadonlyObject(YObject*, YThread*);
YArray* newTuple(YArray*, YThread*);
YArray* newSlice(YArray*, size_t, size_t, YThread*);
YObject* newYoyoMapWrapper(AbstractYoyoMap*, YThread*);
YObject* newYoyoSetWrapper(AbstractYoyoSet*, YThread*);
YArray* newArrayObject(YObject*, YThread*);
YoyoIterator* newYoyoIterator(YObject*, YThread*);
void YoyoIterator_init(YoyoIterator*, YThread*);

extern YType* IteratorType;

#endif
