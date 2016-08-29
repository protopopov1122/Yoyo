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
	o->marked = false;
	o->linkc = 0;
	o->free = hfree;
	o->mark = mark;
	o->age = clock();
	o->stage = 0;
	o->prev = NULL;
	return o;
}

typedef struct PlainGC {
	GarbageCollector gc;
	MUTEX mutex;
	
	YoyoObject* objects;
	YoyoObject* temporary;
	size_t size;
	size_t temporary_size;

	bool collecting;
} PlainGC;

void plain_gc_collect(GarbageCollector* _gc) {
	PlainGC* gc = (PlainGC*) _gc;
	gc->temporary_size = 0;
	gc->temporary = NULL;
	gc->collecting = true;
	for (YoyoObject* ptr = gc->objects; ptr!=NULL; ptr = ptr->prev) {
		if (ptr->linkc!=0)
				MARK(ptr);
	}
	YoyoObject* newPool = NULL;
	size_t newSize = 0;
	const clock_t MAX_AGE = CLOCKS_PER_SEC;
	for (YoyoObject* ptr = gc->objects; ptr!=NULL;) {
			YoyoObject* prev = ptr->prev;
			if ((!ptr->marked) && ptr->linkc == 0 && clock()-ptr->age >= MAX_AGE)
					/*Project is garbage only if:
					 * it's unmarked
					 * it has zero link count on it
					 * it was created MAX_AGE processor clocks ago*/
					{
				ptr->free(ptr);
			} else    // Object isn't garbage
			{
				ptr->prev = newPool;
				ptr->marked = false;
				newSize++;
				newPool = ptr;
			}
			ptr = prev;
	}
	size_t freed = gc->size - newSize;
	gc->size = newSize;


	gc->objects = newPool;
	gc->collecting = false;
	MUTEX_LOCK(&gc->mutex);
	for (YoyoObject* ptr=gc->temporary; ptr!=NULL;) {
		YoyoObject* prev = ptr->prev;
		ptr->prev = gc->objects;
		gc->objects = ptr;
		ptr = prev;
	}
	gc->gc.panic = false;
	if (freed<gc->temporary_size)
		gc->gc.panic = true;
	MUTEX_UNLOCK(&gc->mutex);
}

void plain_gc_free(GarbageCollector* _gc) {
	PlainGC* gc = (PlainGC*) _gc;
	for (YoyoObject* ptr = gc->objects; ptr!=NULL;) {
		YoyoObject* prev = ptr->prev;
		ptr->free(ptr);
		ptr = prev;
	}
	DESTROY_MUTEX(&gc->mutex);
	free(gc);
}

void plain_gc_registrate(GarbageCollector* _gc, YoyoObject* o) {
	PlainGC* gc = (PlainGC*) _gc;
	MUTEX_LOCK(&gc->mutex);
	o->marked = true;
	if (!gc->collecting) {
		o->prev = gc->objects;
		gc->objects = o;
		gc->size++;
	}
	else {
		o->prev = gc->temporary;
		gc->temporary = o;
		gc->temporary_size++;
	}
	MUTEX_UNLOCK(&gc->mutex);
}
GarbageCollector* newPlainGC() {
	PlainGC* gc = malloc(sizeof(PlainGC));

	gc->objects = NULL;
	gc->temporary = NULL;
	gc->collecting = false;
	gc->size = 0;
	gc->gc.block = false;
	gc->gc.panic = false;
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
