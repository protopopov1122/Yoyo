/*
 * Copyright (C) 2016  Jevgenijs Protopopovs <protopopov1122@yandex.ru>
 */
/*This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 3 as published by
 the Free Software Foundation.


 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "memalloc.h"

/* Memory allocator is used to hold pool of
 * memory regions of certain size. When MemoryAllocator->alloc
 * method is called allocator returns next region
 * from pool(if pool is empty then it allocates some memory to fill it in pool).
 * When Memory->unuse method is called allocator
 * adds memory region back to pool to reuse it.
 * Memory allocator speeds up some allocation code
 * by reusing space that allocated before */
void* MemoryAllocator_alloc(MemoryAllocator* mem) {
	MUTEX_LOCK(&mem->mutex);
	if (mem->pool_size == 0) {
		size_t newSize = mem->pool_size + mem->rate;
		if (mem->capacity <= newSize) {
			mem->capacity = newSize + 2;
			mem->pool = realloc(mem->pool, sizeof(void*) * mem->capacity);
		}
		size_t len = newSize - mem->pool_size;
		for (size_t i = 0; i < len; i++) {
			mem->pool[i + mem->pool_size] = malloc(mem->size);
		}
		mem->pool_size = newSize;
	}
	void *out = mem->pool[--mem->pool_size];
	memset(out, 0, mem->size);
	MUTEX_UNLOCK(&mem->mutex);
	return out;
}

void MemoryAllocator_unuse(MemoryAllocator* mem, void* ptr) {
	MUTEX_LOCK(&mem->mutex);
	if (mem->pool_size + mem->rate / 2 >= mem->capacity) {
		mem->capacity += mem->rate;
		mem->pool = realloc(mem->pool, sizeof(void*) * mem->capacity);
	}
	mem->pool[mem->pool_size++] = ptr;
	MUTEX_UNLOCK(&mem->mutex);
}

MemoryAllocator* newMemoryAllocator(size_t size, size_t rate) {
	MemoryAllocator* mem = malloc(sizeof(MemoryAllocator));

	mem->size = size;
	mem->rate = rate;
	mem->capacity = rate;
	mem->pool_size = 0;
	mem->pool = malloc(sizeof(void*) * mem->capacity);

	mem->alloc = MemoryAllocator_alloc;
	mem->unuse = MemoryAllocator_unuse;

	NEW_MUTEX(&mem->mutex);

	return mem;
}
