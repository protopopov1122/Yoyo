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

#ifndef YILI_GC_H
#define YILI_GC_H

#include "core.h"

typedef struct YoyoObject {
	bool marked;
	void (*mark)(struct YoyoObject*);
	uint16_t linkc;
	void (*free)(struct YoyoObject*);

	clock_t age;
	uint32_t cycle;
} YoyoObject;

typedef struct GarbageCollector {
	void (*collect)(struct GarbageCollector*);
	void (*free)(struct GarbageCollector*);
	void (*registrate)(struct GarbageCollector*, YoyoObject*);

	MUTEX access_mutex;
	bool panic;
} GarbageCollector;

YoyoObject* initYoyoObject(YoyoObject*, void (*)(YoyoObject*),
		void (*)(YoyoObject*));

GarbageCollector* newPlainGC(size_t);
GarbageCollector* newGenerationalGC(size_t, uint16_t);
YoyoObject* initAtomicYoyoObject(YoyoObject*, void (*)(YoyoObject*));

#define MARK(ptr) if (ptr!=NULL&&!((YoyoObject*) ptr)->marked) ((YoyoObject*) ptr)->mark((YoyoObject*) ptr);

#endif
