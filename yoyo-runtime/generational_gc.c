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

typedef struct ObjectGeneration {
	MUTEX mutex;
	
	YoyoObject* objects;
	YoyoObject* temporary;
	
	size_t size;	
	size_t temporary_size;
	
	bool collecting;

	struct ObjectGeneration* next;
} ObjectGeneration;

typedef struct GenerationalGC {
	GarbageCollector gc;

	ObjectGeneration* generations;
	size_t generation_count;

	size_t gap;
} GenerationalGC;

void generation_registrate(ObjectGeneration* gen, YoyoObject* ptr) {
	MUTEX_LOCK(&gen->mutex);
	if (!gen->collecting) {
		ptr->prev = gen->objects;
		gen->objects = ptr;
		gen->size++;
		ptr->marked = true;
		ptr->stage = 0;
	} else {
		ptr->prev = gen->temporary;
		gen->temporary = ptr;
		gen->temporary_size++;
		ptr->marked = true;
		ptr->stage = 0;
	}
	MUTEX_UNLOCK(&gen->mutex);
}

void generation_collect(GenerationalGC* gc, ObjectGeneration* gen) {
	gen->temporary_size = 0;
	gen->temporary = NULL;
	gen->collecting = true;

	for (YoyoObject* ptr = gen->objects; ptr!=NULL; ptr = ptr->prev)
		if (ptr->linkc!=0)
			MARK(ptr);
	YoyoObject* newPool = NULL;
	size_t newSize = 0;
	const clock_t MAX_AGE = CLOCKS_PER_SEC;
	for (YoyoObject* ptr = gen->objects; ptr!=NULL;) {
		YoyoObject* prev = ptr->prev;
		if ((!ptr->marked) && clock()-ptr->age >= MAX_AGE) {
			ptr->free(ptr);
		} else if (ptr->stage>=gc->gap&&gen->next!=NULL) {
			generation_registrate(gen->next, ptr);
		} else {
			ptr->prev = newPool;
			ptr->marked = false;
			newSize++;
			newPool = ptr;
			ptr->stage++;
		}
		ptr = prev;
	}

	size_t freed = gen->size - newSize;
	gen->size = newSize;
	gen->objects = newPool;
	gen->collecting = false;

	MUTEX_LOCK(&gen->mutex);
	for (YoyoObject* ptr=gen->temporary; ptr!=NULL;) {
		YoyoObject* prev = ptr->prev;
		ptr->prev = gen->objects;
		gen->objects = ptr;
		ptr = prev;
		gen->size++;
	}
	gc->gc.panic = false;
	if (freed<gen->temporary_size)
		gc->gc.panic = true;
	MUTEX_UNLOCK(&gen->mutex);
}

void generation_free(ObjectGeneration* gen) {
	for (YoyoObject* ptr = gen->objects; ptr!=NULL;) {
		YoyoObject* prev = ptr->prev;
		ptr->free(ptr);
		ptr = prev;
	}
	DESTROY_MUTEX(&gen->mutex);
}

void GenerationalGC_registrate(GarbageCollector* _gc, YoyoObject* ptr) {
	GenerationalGC* gc = (GenerationalGC*) _gc;
	generation_registrate(&gc->generations[0], ptr);
}

void GenerationalGC_collect(GarbageCollector* _gc) {
	GenerationalGC* gc = (GenerationalGC*) _gc;
	for (ssize_t i = gc->generation_count-1;i>-1;i--) {
		generation_collect(gc, &gc->generations[i]);
	}
}

void GenerationalGC_free(GarbageCollector* _gc) {
	GenerationalGC* gc = (GenerationalGC*) _gc;
	for (size_t i=0;i<gc->generation_count;i++)
		generation_free(&gc->generations[i]);
	free(gc->generations);
	free(gc);
}

GarbageCollector* newGenerationalGC(size_t genCount, size_t gap) {
	GenerationalGC* gc = malloc(sizeof(GenerationalGC));

	gc->generation_count = genCount;
	gc->gap = gap;
	gc->generations = calloc(genCount, sizeof(ObjectGeneration));
	for (ssize_t i=genCount-1;i>-1;i--) {
		gc->generations[i].objects = NULL;
		gc->generations[i].temporary = NULL;
		gc->generations[i].size = 0;
		gc->generations[i].temporary = 0;
		gc->generations[i].collecting = false;
		NEW_MUTEX(&gc->generations[i].mutex);
		gc->generations[i].next = NULL;
		if (i+1<genCount)
			gc->generations[i].next = &gc->generations[i+1];
	}

	gc->gc.panic = false;
	gc->gc.block = false;
	gc->gc.registrate = GenerationalGC_registrate;
	gc->gc.collect = GenerationalGC_collect;
	gc->gc.free = GenerationalGC_free;

	return (GarbageCollector*) gc;
}
