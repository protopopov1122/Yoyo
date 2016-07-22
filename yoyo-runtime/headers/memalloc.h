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

#ifndef YILI_MEMALLOC_H
#define YILI_MEMALLOC_H

#include "core.h"

typedef struct MemoryAllocator {
	void* (*alloc)(struct MemoryAllocator*);
	void (*unuse)(struct MemoryAllocator*, void*);
	void (*free)(struct MemoryAllocator*);

	void** pool;
	size_t pool_size;
	size_t capacity;
	size_t rate;
	size_t size;

	MUTEX mutex;
} MemoryAllocator;

MemoryAllocator* newMemoryAllocator(size_t, size_t);

#endif
