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
	HeapObject** pool;
	size_t size;
	size_t capacity;
	bool need_gc;
	struct ObjectGeneration* next_gen;
} ObjectGeneration;

typedef struct GenerationalGC {
	GarbageCollector gc;

	ObjectGeneration* generations;
	size_t generation_count;
	uint16_t generation_step;
} GenerationalGC;

void GenerationalGC_free(GarbageCollector* _gc) {
	GenerationalGC* gc = (GenerationalGC*) _gc;
	for (size_t i = 0; i < gc->generation_count; i++) {
		for (size_t j = 0; j < gc->generations[i].size; j++)
			gc->generations[i].pool[j]->free(gc->generations[i].pool[j]);
		free(gc->generations[i].pool);
	}
	free(gc->generations);
	DESTROY_MUTEX(&_gc->access_mutex);
	free(gc);
}

void generation_registrate(ObjectGeneration* gen, HeapObject* ptr) {
	ptr->cycle = 0;
	if (gen->size + 2 >= gen->capacity) {
		gen->capacity = gen->capacity * 1.1 + 100;
		gen->pool = realloc(gen->pool, sizeof(HeapObject*) * gen->capacity);
		gen->need_gc = true;
	}
	gen->pool[gen->size++] = ptr;
}

void GenerationalGC_registrate(GarbageCollector* _gc, HeapObject* ptr) {
	MUTEX_LOCK(&_gc->access_mutex);
	GenerationalGC* gc = (GenerationalGC*) _gc;
	generation_registrate(&gc->generations[0], ptr);
	MUTEX_UNLOCK(&_gc->access_mutex);
}

void generation_collect(GenerationalGC* gc, ObjectGeneration* gen) {
	if (!gen->need_gc)
		return;
	const clock_t MAX_AGE = CLOCKS_PER_SEC;
	gen->need_gc = false;
	HeapObject** newPool = malloc(sizeof(HeapObject*) * gen->capacity);
	size_t newSize = 0;
	for (size_t i = 0; i < gen->size; i++) {
		HeapObject* ptr = gen->pool[i];
		if ((!ptr->marked) && ptr->linkc == 0
				&& clock() - ptr->age >= MAX_AGE) {
			ptr->free(ptr);
		} else {
			ptr->marked = false;
			if (gen->next_gen != NULL && ptr->cycle >= gc->generation_step) {
				generation_registrate(gen->next_gen, ptr);
			} else {
				ptr->cycle++;
				newPool[newSize++] = ptr;
			}
		}
	}
	free(gen->pool);
	gen->pool = newPool;
	gen->size = newSize;
	if (gen->size * 2 < gen->capacity && gen->capacity > 1000) {
		gen->capacity = gen->size * 2;
		gen->pool = realloc(gen->pool, sizeof(HeapObject*) * gen->capacity);
	}
}

void GenerationalGC_collect(GarbageCollector* _gc) {
	GenerationalGC* gc = (GenerationalGC*) _gc;
	for (ssize_t i = gc->generation_count - 1; i > -1; i--)
		generation_collect(gc, &gc->generations[i]);
}

GarbageCollector* newGenerationalGC(size_t gen_cap, uint16_t step) {
	size_t generation_count = 3;

	GenerationalGC* gc = malloc(sizeof(GenerationalGC));
	gc->generation_count = generation_count;
	gc->generation_step = step;
	gc->generations = malloc(sizeof(ObjectGeneration) * gc->generation_count);
	for (size_t i = 0; i < gc->generation_count; i++) {
		gc->generations[i].capacity = gen_cap;
		gc->generations[i].size = 0;
		gc->generations[i].need_gc = false;
		gc->generations[i].pool = malloc(sizeof(HeapObject*) * gen_cap);
		gc->generations[i].next_gen =
				i + 1 < gc->generation_count ? &gc->generations[i + 1] : NULL;
	}

	NEW_MUTEX(&gc->gc.access_mutex);

	gc->gc.collect = GenerationalGC_collect;
	gc->gc.registrate = GenerationalGC_registrate;
	gc->gc.free = GenerationalGC_free;
	return (GarbageCollector*) gc;
}
