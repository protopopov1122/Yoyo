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

#include "../yoyo/core.h"

typedef struct HeapObject {
	bool marked;
	void (*mark)(struct HeapObject*);
	uint16_t linkc;
	void (*free)(struct HeapObject*);

	clock_t age;
	uint32_t cycle;
} HeapObject;

typedef struct GarbageCollector {
	void (*collect)(struct GarbageCollector*);
	void (*free)(struct GarbageCollector*);
	void (*registrate)(struct GarbageCollector*, HeapObject*);

	MUTEX access_mutex;
} GarbageCollector;

HeapObject* initHeapObject(HeapObject*, void (*)(HeapObject*),
		void (*)(HeapObject*));

GarbageCollector* newPlainGC(size_t);
GarbageCollector* newGenerationalGC(size_t, uint16_t);
HeapObject* initAtomicHeapObject(HeapObject*, void (*)(HeapObject*));

#define MARK(ptr) if (ptr!=NULL&&!((HeapObject*) ptr)->marked) ((HeapObject*) ptr)->mark((HeapObject*) ptr);

#endif
