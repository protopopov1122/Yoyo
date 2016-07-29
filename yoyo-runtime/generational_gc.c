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

/*File contains main implementation of Yoyo garbage collector.
 * This is generational Garbage Collector:
 * 	exist 3 generations of objects - young, middle and old.
 * 	Each object when registred to Garbage Collector
 * 	being added to young generation. Object can move
 * 	from young to middle generation and from middle
 * 	to old generation if in current generation it
 * 	wasn't collected some number of times(defined in GC).
 * 	Garbage in each generation is collected only if
 * 	it has flag gc_required enabled. If generation
 * 	has no enought space to add object then generations
 * 	object pool is reallocated and flag gc_required
 * 	is enabled. If after garbage collection pool
 * 	size is two times lesser than pool capacity then
 * 	pool is reallocated*/

typedef struct ObjectGeneration {
	YoyoObject** pool;
	size_t size;
	size_t capacity;
	bool needGC;
	MUTEX mutex;
	struct ObjectGeneration* next_gen;
} ObjectGeneration;

typedef struct GenerationalGC {
	GarbageCollector gc;
	ObjectGeneration* generations;
	size_t generation_count;
} GenerationalGC;

void generation_register(ObjectGeneration* gen, YoyoObject* ptr) {
	MUTEX_LOCK(&gen->mutex);
	if (gen->size+2>=gen->capacity) {
		gen->capacity = gen->capacity * 2;
		gen->pool = realloc(gen->pool, sizeof(YoyoObject*) * gen->capacity);
		gen->needGC = true;
	}
	gen->pool[gen->size++] = ptr;
	ptr->marked = true;
	ptr->cycle = 0;
	MUTEX_UNLOCK(&gen->mutex);
}

void generation_collect(ObjectGeneration* gen) {
	if (!gen->needGC)
		return;
	gen->needGC = false;
	MUTEX_LOCK(&gen->mutex);

	YoyoObject** newPool = malloc(sizeof(YoyoObject*) * gen->capacity);
	size_t newSize = 0;
	const clock_t MAX_AGE = CLOCKS_PER_SEC;
	for (size_t i=0;i<gen->size;i++) {
		YoyoObject* ptr = gen->pool[i];
		if ((!ptr->marked) &&
			ptr->linkc == 0 &&
			clock()-ptr->age>=MAX_AGE) {
			ptr->free(ptr);
		} else if (gen->next_gen==NULL) {
				newPool[newSize++] = ptr;
				ptr->marked = false;
				ptr->cycle++;
		} else {
				if (ptr->cycle<5) {
					newPool[newSize++] = ptr;
					ptr->marked = false;
					ptr->cycle++;
				}	else
					generation_register(gen->next_gen, ptr);
		}
	}

	free(gen->pool);
	gen->pool = newPool;
	gen->size = newSize;
	if (gen->size * 2 < gen->capacity) {
		gen->capacity = gen->size * 1.1 + 1000;
		gen->pool = realloc(gen->pool, sizeof(YoyoObject*) * gen->capacity);
	}

	MUTEX_UNLOCK(&gen->mutex);
}

void GenerationalGC_collect(GarbageCollector* _gc) {
	GenerationalGC* gc = (GenerationalGC*) _gc;
	for (size_t i=gc->generation_count-1;i<gc->generation_count;i--) {
		generation_collect(&gc->generations[i]);
	}
}
void GenerationalGC_register(GarbageCollector* _gc, YoyoObject* ptr) {
	GenerationalGC* gc = (GenerationalGC*) _gc;
	generation_register(&gc->generations[0], ptr);
}
void GenerationalGC_free(GarbageCollector* _gc) {
	GenerationalGC* gc = (GenerationalGC*) _gc;
	for (size_t i=0;i<gc->generation_count;i++) {
		for (size_t j=0;j<gc->generations[i].size;j++)
			gc->generations[i].pool[j]->free(gc->generations[i].pool[j]);
		DESTROY_MUTEX(&gc->generations[i].mutex);
		free(gc->generations[i].pool);
	}
	free(gc->generations);
	free(gc);
}

GarbageCollector* newGenerationalGC(size_t initCap, uint16_t gen_count) {
	GenerationalGC* gc = malloc(sizeof(GenerationalGC));
	gc->generation_count = gen_count;
	gc->generations = malloc(sizeof(ObjectGeneration) * gen_count);
	for (size_t i=0;i<gen_count;i++) {
		gc->generations[i].size = 0;
		gc->generations[i].capacity = initCap;
		gc->generations[i].needGC = false;
		gc->generations[i].pool = malloc(sizeof(YoyoObject*) * initCap);
		NEW_MUTEX(&gc->generations[i].mutex);
	}
	for (size_t i=0;i<gen_count;i++)
		gc->generations[i].next_gen = i+1<gen_count ?
			&gc->generations[i+1] : NULL;

	gc->gc.collect = GenerationalGC_collect;
	gc->gc.registrate = GenerationalGC_register;
	gc->gc.free = GenerationalGC_free;
	return (GarbageCollector*) gc;
}
