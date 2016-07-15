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

#ifndef YILI_COLLECTIONS_H
#define YILI_COLLECTIONS_H

#include "value.h"

typedef struct YoyoSet {
	HeapObject o;

	void (*add)(struct YoyoSet*, YValue*, YThread*);
	bool (*contains)(struct YoyoSet*, YValue*, YThread*);
	size_t (*size)(struct YoyoSet*, YThread*);
	YValue* (*entry)(struct YoyoSet*, size_t, YThread*);
	YoyoIterator* (*iter)(struct YoyoSet*, YThread*);
} YoyoSet;

typedef struct YoyoMap {
	HeapObject o;

	YValue* (*get)(struct YoyoMap*, YValue*, YThread*);
	void (*put)(struct YoyoMap*, YValue*, YValue*, YThread*);
	bool (*contains)(struct YoyoMap*, YValue*, YThread*);
	void (*remove)(struct YoyoMap*, YValue*, YThread*);
	size_t (*size)(struct YoyoMap*, YThread*);
	YValue* (*key)(struct YoyoMap*, size_t, YThread* th);
	void (*clear)(struct YoyoMap*, YThread*);
	YoyoSet* (*keySet)(struct YoyoMap*, YThread*);
} YoyoMap;

YoyoMap* newHashMap(YThread*);
YoyoSet* newHashSet(YThread*);
YArray* newList(YThread*);

#endif // YILI_COLLECTIONS_H
