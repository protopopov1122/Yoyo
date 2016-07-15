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

#include "../headers/yoyo/gc.h"

HeapObject* initHeapObject(HeapObject* o, void (*mark)(HeapObject*),
		void (*hfree)(HeapObject*)) {
	if (o == NULL)
		o = malloc(sizeof(HeapObject));
	o->marked = false;
	o->linkc = 0;
	o->free = hfree;
	o->mark = mark;
	o->age = clock();
	o->cycle = 0;
	return o;
}

typedef struct PlainGC {
	GarbageCollector gc;

	HeapObject** objects;
	size_t used;
	size_t size;
} PlainGC;

void plain_gc_collect(GarbageCollector* _gc) {
	PlainGC* gc = (PlainGC*) _gc;
	const clock_t MAX_AGE = CLOCKS_PER_SEC;
	HeapObject** newHeap = malloc(gc->size * sizeof(HeapObject*)); /* Pointers to all necesarry objects are
	 moved to another memory area to
	 prevent pointer array fragmentation*/
	size_t nextIndex = 0;
	for (size_t i = gc->used - 1; i < gc->used; i--) {
		if (gc->objects[i] != NULL) {
			if ((!gc->objects[i]->marked) && gc->objects[i]->linkc == 0
					&& clock() - gc->objects[i]->age >= MAX_AGE)
					/*Project is garbage only if:
					 * it's unmarked
					 * it has zero link count on it
					 * it was created MAX_AGE processor clocks ago*/
					{
				gc->objects[i]->free(gc->objects[i]);
			} else    // Object isn't garbage
			{
				newHeap[nextIndex++] = gc->objects[i];
				gc->objects[i]->marked = false;
				gc->objects[i] = NULL;
			}
		}
	}
	gc->used = nextIndex;

	if (gc->used * 2.5 < gc->size && gc->used > 1000) {
		gc->size = gc->used * 2.5;
		newHeap = realloc(newHeap, gc->size * sizeof(HeapObject*));
	}
	memset(&newHeap[gc->used], 0, sizeof(HeapObject*) * (gc->size - gc->used));
	free(gc->objects);
	gc->objects = newHeap;
}

void plain_gc_mark(GarbageCollector* _gc, HeapObject** roots, size_t rootc) {
	PlainGC* gc = (PlainGC*) _gc;
	for (size_t i = 0; i < gc->size; i++) {
		if (gc->objects[i] != NULL)
			gc->objects[i]->marked = false;
	}
	for (size_t i = 0; i < rootc; i++)
		roots[i]->mark(roots[i]);
}

void plain_gc_free(GarbageCollector* _gc) {
	PlainGC* gc = (PlainGC*) _gc;
	for (size_t i = 0; i < gc->size; i++) {
		if (gc->objects[i] != NULL)
			gc->objects[i]->free(gc->objects[i]);
	}
	free(gc->objects);
	DESTROY_MUTEX(&gc->gc.access_mutex);
	free(gc);
}

void plain_gc_registrate(GarbageCollector* _gc, HeapObject* o) {
	PlainGC* gc = (PlainGC*) _gc;
	MUTEX_LOCK(&gc->gc.access_mutex);
	bool all = false;
	for (size_t i = gc->used; i < gc->size; i++) {
		if (gc->objects[i] == NULL) {
			gc->objects[i] = o;
			all = true;
			break;
		}
	}
	if (!all) {
		size_t newSize = gc->size + gc->size / 10;
		gc->objects = realloc(gc->objects, sizeof(HeapObject*) * newSize);
		memset(&gc->objects[gc->size + 1], 0,
				sizeof(HeapObject*) * (newSize - gc->size - 1));
		gc->objects[gc->size] = o;
		gc->size = newSize;
	}
	gc->used++;
	MUTEX_UNLOCK(&gc->gc.access_mutex);
}
GarbageCollector* newPlainGC(size_t isize) {
	PlainGC* gc = malloc(sizeof(PlainGC));

	gc->size = isize;
	gc->used = 0;
	gc->objects = calloc(1, sizeof(HeapObject*) * isize);
	NEW_MUTEX(&gc->gc.access_mutex);
	gc->gc.collect = plain_gc_collect;
	gc->gc.free = plain_gc_free;
	gc->gc.registrate = plain_gc_registrate;

	return (GarbageCollector*) gc;
}
void markAtomic(HeapObject* o) {
	o->marked = true;
}
HeapObject* initAtomicHeapObject(HeapObject* o, void (*ofree)(HeapObject*)) {
	return initHeapObject(o, markAtomic, ofree);
}
