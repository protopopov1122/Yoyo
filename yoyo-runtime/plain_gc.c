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

#include "gc.h"

YoyoObject* initYoyoObject(YoyoObject* o, void (*mark)(YoyoObject*),
		void (*hfree)(YoyoObject*)) {
	if (o == NULL)
		o = malloc(sizeof(YoyoObject));
	o->marked = false;
	o->linkc = 0;
	o->free = hfree;
	o->mark = mark;
	o->age = clock();
	return o;
}

typedef struct PlainGC {
	GarbageCollector gc;
	MUTEX mutex;

	YoyoObject** objects;
	size_t size;
	size_t capacity;

	YoyoObject** temporary;
	size_t temporary_size;
	size_t temporary_capacity;
	bool collecting;
} PlainGC;

void plain_gc_collect(GarbageCollector* _gc) {
	PlainGC* gc = (PlainGC*) _gc;
	gc->temporary_size = 0;
	gc->temporary_capacity = 1000;
	gc->temporary = malloc(sizeof(YoyoObject*) * gc->temporary_capacity);
	gc->collecting = true;
	for (size_t i=0;i<gc->size;i++) {
		YoyoObject* ptr = gc->objects[i];
		if (ptr->linkc!=0)
				MARK(ptr);
	}
	const clock_t MAX_AGE = CLOCKS_PER_SEC;
	YoyoObject** newHeap = malloc(gc->capacity * sizeof(YoyoObject*)); /* Pointers to all necesarry objects are
	 moved to another memory area to
	 prevent pointer array fragmentation*/
	size_t nextIndex = 0;
	for (size_t i = 0; i < gc->size; i++) {
		YoyoObject* ptr = gc->objects[i];
		if (ptr != NULL) {
			if ((!ptr->marked) && ptr->linkc == 0 && clock()-ptr->age >= MAX_AGE)
					/*Project is garbage only if:
					 * it's unmarked
					 * it has zero link count on it
					 * it was created MAX_AGE processor clocks ago*/
					{
				ptr->free(ptr);
			} else    // Object isn't garbage
			{
				newHeap[nextIndex++] = ptr;
				ptr->marked = false;
			}
		}
	}
	size_t freed = gc->size - nextIndex;
	gc->size = nextIndex;

	free(gc->objects);
	gc->objects = newHeap;
	gc->collecting = false;
	MUTEX_LOCK(&gc->mutex);
	if (gc->capacity<=gc->size+gc->temporary_size) {
		gc->capacity = gc->size+gc->temporary_size+100;
		gc->objects = realloc(gc->objects, sizeof(YoyoObject*) * gc->capacity);
	}
	for (size_t i=0;i<gc->temporary_size;i++)
		gc->objects[gc->size++] = gc->temporary[i];
	if (gc->size * 1.5 < gc->capacity && gc->size > 1000) {
		gc->capacity = gc->size * 1.5;
		newHeap = realloc(newHeap, gc->capacity * sizeof(YoyoObject*));
	}
	gc->gc.panic = false;
	if (freed<gc->temporary_size)
		gc->gc.panic = true;
	free(gc->temporary);
	MUTEX_UNLOCK(&gc->mutex);
}

void plain_gc_free(GarbageCollector* _gc) {
	PlainGC* gc = (PlainGC*) _gc;
	for (size_t i = 0; i < gc->capacity; i++) {
		if (gc->objects[i] != NULL)
			gc->objects[i]->free(gc->objects[i]);
	}
	free(gc->objects);
	DESTROY_MUTEX(&gc->mutex);
	free(gc);
}

void plain_gc_registrate(GarbageCollector* _gc, YoyoObject* o) {
	PlainGC* gc = (PlainGC*) _gc;
	MUTEX_LOCK(&gc->mutex);
	if (!gc->collecting) {
		if (gc->size+2>=gc->capacity) {
			gc->capacity = gc->capacity*1.1+100;
			gc->objects = realloc(gc->objects, sizeof(YoyoObject*) * gc->capacity);
		}
		gc->objects[gc->size++] = o;
		o->marked = true;
	}
	else {
		if (gc->temporary_size+2>=gc->temporary_capacity) {
			gc->temporary_capacity = gc->temporary_capacity*1.1 + 10;
			gc->temporary = realloc(gc->temporary, sizeof(YoyoObject*) * gc->temporary_capacity);
		}
		gc->temporary[gc->temporary_size++] = o;
		o->marked = true;
	}
	MUTEX_UNLOCK(&gc->mutex);
}
GarbageCollector* newPlainGC(size_t icapacity) {
	PlainGC* gc = malloc(sizeof(PlainGC));

	gc->collecting = false;
	gc->capacity = icapacity;
	gc->size = 0;
	gc->objects = calloc(1, sizeof(YoyoObject*) * icapacity);
	gc->gc.block = false;
	NEW_MUTEX(&gc->mutex);
	gc->gc.collect = plain_gc_collect;
	gc->gc.free = plain_gc_free;
	gc->gc.registrate = plain_gc_registrate;

	return (GarbageCollector*) gc;
}
void markAtomic(YoyoObject* o) {
	o->marked = true;
}
YoyoObject* initAtomicYoyoObject(YoyoObject* o, void (*ofree)(YoyoObject*)) {
	return initYoyoObject(o, markAtomic, ofree);
}
