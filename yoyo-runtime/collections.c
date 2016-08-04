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

#include "yoyo-runtime.h"

/* Contains implementation of some collections like maps sets & lists.
 * Interfaces are defined in yoyo/collections.h and yoyo/value.h */

typedef struct YoyoHashMapEntry {
	YValue* key;
	YValue* value;

	struct YoyoHashMapEntry* next;
	struct YoyoHashMapEntry* prev;
} YoyoHashMapEntry;
typedef struct YoyoHashMap {
	YoyoMap map;

	YoyoHashMapEntry** entries;
	size_t size;
	uint32_t factor;

	size_t full_size;

	MUTEX map_mutex;
} YoyoHashMap;

void YoyoHashMap_mark(YoyoObject* ptr) {
	ptr->marked = true;
	YoyoHashMap* map = (YoyoHashMap*) ptr;
	MUTEX_LOCK(&map->map_mutex);
	for (size_t i = 0; i < map->size; i++) {
		YoyoHashMapEntry* entry = map->entries[i];
		while (entry != NULL) {
			YoyoObject* ko = (YoyoObject*) entry->key;
			YoyoObject* vo = (YoyoObject*) entry->value;
			MARK(ko);
			MARK(vo);
			entry = entry->next;
		}
	}
	MUTEX_UNLOCK(&map->map_mutex);
}
void YoyoHashMap_free(YoyoObject* ptr) {
	YoyoHashMap* map = (YoyoHashMap*) ptr;
	for (size_t i = 0; i < map->size; i++) {
		YoyoHashMapEntry* entry = map->entries[i];
		while (entry != NULL) {
			YoyoHashMapEntry* e = entry;
			entry = entry->next;
			free(e);
		}
	}
	free(map->entries);
	DESTROY_MUTEX(&map->map_mutex);
	free(map);
}

size_t YoyoHashMap_size(YoyoMap* m, YThread* th) {
	return ((YoyoHashMap*) m)->full_size;
}
YoyoHashMapEntry* YoyoHashMap_getEntry(YoyoHashMap* map, YValue* key,
		YThread* th) {
	MUTEX_LOCK(&map->map_mutex);
	uint64_t hash = key->type->oper.hashCode(key, th);
	size_t index = hash % map->size;
	YoyoHashMapEntry* e = map->entries[index];
	while (e != NULL) {
		if (CHECK_EQUALS(e->key, key, th)) {
			MUTEX_UNLOCK(&map->map_mutex);
			return e;
		}
		e = e->next;
	}
	MUTEX_UNLOCK(&map->map_mutex);
	return NULL;
}
void YoyoHashMap_remap(YoyoHashMap* map, YThread* th) {
	size_t newSize = map->size + map->factor;
	YoyoHashMapEntry** newEnt = calloc(1, sizeof(YoyoHashMapEntry*) * newSize);
	map->factor = 0;
	for (size_t i = 0; i < map->size; i++) {
		YoyoHashMapEntry* e = map->entries[i];
		while (e != NULL) {
			YoyoHashMapEntry* entry = e;
			e = e->next;
			uint64_t hashCode = entry->key->type->oper.hashCode(entry->key, th);
			size_t index = hashCode % newSize;
			YoyoHashMapEntry* newEntry = malloc(sizeof(YoyoHashMapEntry));
			newEntry->key = entry->key;
			newEntry->value = entry->value;
			newEntry->next = NULL;
			newEntry->prev = NULL;
			free(entry);
			if (newEnt[index] == NULL)
				newEnt[index] = newEntry;
			else {
				entry = newEnt[index];
				while (entry->next != NULL)
					entry = entry->next;
				entry->next = newEntry;
				newEntry->prev = entry;
				map->factor++;
			}
		}
	}
	free(map->entries);
	map->entries = newEnt;
	map->size = newSize;
}
YValue* YoyoHashMap_get(YoyoMap* m, YValue* key, YThread* th) {
	YoyoHashMap* map = (YoyoHashMap*) m;
	YoyoHashMapEntry* e = YoyoHashMap_getEntry(map, key, th);
	return e != NULL ? e->value : getNull(th);
}
bool YoyoHashMap_contains(YoyoMap* m, YValue* key, YThread* th) {
	YoyoHashMap* map = (YoyoHashMap*) m;
	YoyoHashMapEntry* e = YoyoHashMap_getEntry(map, key, th);
	return e != NULL;
}
void YoyoHashMap_put(YoyoMap* m, YValue* key, YValue* value, YThread* th) {
	YoyoHashMap* map = (YoyoHashMap*) m;
	YoyoHashMapEntry* e = YoyoHashMap_getEntry(map, key, th);
	MUTEX_LOCK(&map->map_mutex);
	if (e != NULL) {
		e->value = value;
	} else {
		map->full_size++;
		uint64_t hash = key->type->oper.hashCode(key, th);
		size_t index = hash % map->size;
		e = malloc(sizeof(YoyoHashMapEntry));
		e->key = key;
		e->value = value;
		e->next = NULL;
		e->prev = NULL;
		if (map->entries[index] == NULL)
			map->entries[index] = e;
		else {
			YoyoHashMapEntry* entry = map->entries[index];
			while (entry->next != NULL)
				entry = entry->next;
			e->prev = entry;
			entry->next = e;
			map->factor++;
		}
		if ((double) map->factor / (double) map->size >= 0.1)
			YoyoHashMap_remap(map, th);
	}
	MUTEX_UNLOCK(&map->map_mutex);
}
void YoyoHashMap_remove(YoyoMap* m, YValue* key, YThread* th) {
	YoyoHashMap* map = (YoyoHashMap*) m;
	YoyoHashMapEntry* e = YoyoHashMap_getEntry(map, key, th);
	MUTEX_LOCK(&map->map_mutex);
	if (e != NULL) {
		if (e->prev != NULL)
			e->prev->next = e->next;
		else {
			uint64_t hash = key->type->oper.hashCode(key, th);
			size_t index = hash % map->size;
			map->entries[index] = e->next;
		}
		if (e->next != NULL)
			e->next->prev = e->prev;
		map->full_size--;
	}
	MUTEX_UNLOCK(&map->map_mutex);
}
void YoyoHashMap_clear(YoyoMap* m, YThread* th) {
	YoyoHashMap* map = (YoyoHashMap*) m;
	MUTEX_LOCK(&map->map_mutex);
	for (size_t i = 0; i < map->size; i++) {
		YoyoHashMapEntry* e = map->entries[i];
		while (e != NULL) {
			YoyoHashMapEntry* en = e;
			e = e->next;
			free(en);
		}
		map->entries[i] = NULL;
	}
	map->full_size = 0;
	map->factor = 0;
	MUTEX_UNLOCK(&map->map_mutex);
}
YValue* YoyoHashMap_key(YoyoMap* m, size_t index, YThread* th) {
	YoyoHashMap* map = (YoyoHashMap*) m;
	MUTEX_LOCK(&map->map_mutex);
	YValue* out = NULL;
	size_t ei = 0;
	for (size_t i = 0; i < map->size; i++) {
		YoyoHashMapEntry* e = map->entries[i];
		if (out != NULL)
			break;
		while (e != NULL) {
			if (ei == index) {
				out = e->key;
				break;
			}
			e = e->next;
			ei++;
		}
	}
	MUTEX_UNLOCK(&map->map_mutex);
	return out != NULL ? out : getNull(th);
}

typedef struct YoyoKeySet {
	YoyoSet set;

	YoyoMap* map;
} YoyoKeySet;
typedef struct YoyoSetIter {
	YoyoIterator iter;

	YoyoSet* set;
	size_t index;
} YoyoSetIter;
void YoyoSetIter_reset(YoyoIterator* i, YThread* th) {
	YoyoSetIter* iter = (YoyoSetIter*) i;
	iter->index = 0;
}
bool YoyoSetIter_hasNext(YoyoIterator* i, YThread* th) {
	YoyoSetIter* iter = (YoyoSetIter*) i;
	return iter->index < iter->set->size(iter->set, th);
}
YValue* YoyoSetIter_next(YoyoIterator* i, YThread* th) {
	YoyoSetIter* iter = (YoyoSetIter*) i;
	return iter->set->entry(iter->set, iter->index++, th);
}
void YoyoKeySet_add(YoyoSet* set, YValue* v, YThread* th) {
	return;
}
bool YoyoKeySet_contains(YoyoSet* s, YValue* v, YThread* th) {
	YoyoKeySet* set = (YoyoKeySet*) s;
	return set->map->contains(set->map, v, th);
}
void YoyoSetIter_mark(YoyoObject* ptr) {
	ptr->marked = true;
	YoyoSetIter* iter = (YoyoSetIter*) ptr;
	MARK(iter->set);
}
YoyoIterator* YoyoSet_iter(YoyoSet* s, YThread* th) {
	YoyoSetIter* iter = malloc(sizeof(YoyoSetIter));
	initYoyoObject((YoyoObject*) iter, YoyoSetIter_mark,
			(void (*)(YoyoObject*)) free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) iter);

	iter->set = s;
	iter->index = 0;
	iter->iter.reset = YoyoSetIter_reset;
	iter->iter.next = YoyoSetIter_next;
	iter->iter.hasNext = YoyoSetIter_hasNext;

	YoyoIterator_init((YoyoIterator*) iter, th);

	return (YoyoIterator*) iter;
}
size_t YoyoKeySet_size(YoyoSet* s, YThread* th) {
	YoyoKeySet* set = (YoyoKeySet*) s;
	return set->map->size(set->map, th);
}
YValue* YoyoKeySet_entry(YoyoSet* s, size_t index, YThread* th) {
	YoyoKeySet* set = (YoyoKeySet*) s;
	return set->map->key(set->map, index, th);
}
void YoyoKeySet_mark(YoyoObject* ptr) {
	ptr->marked = true;
	YoyoKeySet* set = (YoyoKeySet*) ptr;
	YoyoObject* ho = (YoyoObject*) set->map;
	MARK(ho);
}
YoyoSet* YoyoMap_keySet(YoyoMap* map, YThread* th) {
	YoyoKeySet* set = malloc(sizeof(YoyoKeySet));
	initYoyoObject((YoyoObject*) set, YoyoKeySet_mark,
			(void (*)(YoyoObject*)) free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) set);

	set->map = map;
	set->set.add = YoyoKeySet_add;
	set->set.contains = YoyoKeySet_contains;
	set->set.iter = YoyoSet_iter;
	set->set.size = YoyoKeySet_size;
	set->set.entry = YoyoKeySet_entry;
	return (YoyoSet*) set;
}
void YoyoHashSet_add(YoyoSet* s, YValue* v, YThread* th) {
	YoyoKeySet* set = (YoyoKeySet*) s;
	set->map->put(set->map, v, v, th);
}

YoyoMap* newHashMap(YThread* th) {
	YoyoHashMap* map = malloc(sizeof(YoyoHashMap));
	initYoyoObject((YoyoObject*) map, YoyoHashMap_mark, YoyoHashMap_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) map);

	map->size = 3;
	map->full_size = 0;
	map->factor = 0;
	map->entries = malloc(sizeof(YoyoHashMapEntry*) * map->size);
	memset(map->entries, 0, sizeof(YoyoHashMapEntry*) * map->size);

	map->map.size = YoyoHashMap_size;
	map->map.get = YoyoHashMap_get;
	map->map.contains = YoyoHashMap_contains;
	map->map.put = YoyoHashMap_put;
	map->map.remove = YoyoHashMap_remove;
	map->map.clear = YoyoHashMap_clear;
	map->map.key = YoyoHashMap_key;
	map->map.keySet = YoyoMap_keySet;

	NEW_MUTEX(&map->map_mutex);

	return (YoyoMap*) map;
}
YoyoSet* newHashSet(YThread* th) {
	YoyoMap* map = newHashMap(th);
	YoyoSet* set = map->keySet(map, th);
	set->add = YoyoHashSet_add;
	return set;
}

typedef struct ListEntry {
	YValue* value;
	struct ListEntry* next;
	struct ListEntry* prev;
} ListEntry;

typedef struct YList {
	YArray array;

	ListEntry* list;
	size_t length;
	MUTEX mutex;
} YList;

void List_mark(YoyoObject* ptr) {
	ptr->marked = true;
	YList* list = (YList*) ptr;
	MUTEX_LOCK(&list->mutex);
	ListEntry* e = list->list;
	while (e != NULL) {
		YoyoObject* ho = (YoyoObject*) e->value;
		MARK(ho);
		e = e->next;
	}
	MUTEX_UNLOCK(&list->mutex);
}
void List_free(YoyoObject* ptr) {
	YList* list = (YList*) ptr;
	ListEntry* e = list->list;
	while (e != NULL) {
		ListEntry* entry = e;
		e = e->next;
		free(entry);
	}
	DESTROY_MUTEX(&list->mutex);
	free(list);
}

size_t List_size(YArray* a, YThread* th) {
	YList* list = (YList*) a;
	return list->length;
}
YValue* List_get(YArray* a, size_t index, YThread* th) {
	YList* list = (YList*) a;
	if (index >= list->length)
		return getNull(th);
	MUTEX_LOCK(&list->mutex);
	size_t i = 0;
	ListEntry* e = list->list;
	while (e != NULL) {
		if (i == index) {
			MUTEX_UNLOCK(&list->mutex);
			return e->value;
		}
		i++;
		e = e->next;
	}
	MUTEX_UNLOCK(&list->mutex);
	return getNull(th);
}
void List_add(YArray* a, YValue* v, YThread* th) {
	YList* list = (YList*) a;
	MUTEX_LOCK(&list->mutex);
	ListEntry* e = malloc(sizeof(ListEntry));
	e->value = v;
	e->next = NULL;
	e->prev = NULL;
	if (list->list == NULL)
		list->list = e;
	else {
		ListEntry* entry = list->list;
		while (entry->next != NULL)
			entry = entry->next;
		entry->next = e;
		e->prev = entry;
	}
	list->length++;
	MUTEX_UNLOCK(&list->mutex);
}
void List_set(YArray* a, size_t index, YValue* value, YThread* th) {
	YList* list = (YList*) a;
	if (index >= list->length)
		return;
	MUTEX_LOCK(&list->mutex);
	size_t i = 0;
	ListEntry* e = list->list;
	while (e != NULL) {
		if (i == index) {
			e->value = value;
			break;
		}
		i++;
		e = e->next;
	}
	MUTEX_UNLOCK(&list->mutex);
}
void List_insert(YArray* a, size_t index, YValue* value, YThread* th) {
	YList* list = (YList*) a;
	MUTEX_LOCK(&list->mutex);
	ListEntry* entry = malloc(sizeof(ListEntry));
	entry->value = value;
	entry->prev = NULL;
	entry->next = NULL;
	if (index == 0) {
		if (list->list != NULL) {
			list->list->prev = entry;
			entry->next = list->list;
		}
		list->list = entry;
		list->length++;
		MUTEX_UNLOCK(&list->mutex);
		return;
	}
	MUTEX_UNLOCK(&list->mutex);
	if (index > list->length)
		return;
	else if (index == list->length) {
		List_add(a, value, th);
		return;
	}
	MUTEX_LOCK(&list->mutex);
	ListEntry* e = list->list;
	size_t i = 0;
	while (e != NULL) {
		if (i == index) {
			entry->prev = e->prev;
			entry->next = e;
			e->prev = entry;
			list->length++;
			if (entry->prev != NULL)
				entry->prev->next = entry;
			break;
		}
		i++;
		e = e->next;
	}
	MUTEX_UNLOCK(&list->mutex);
}

void List_remove(YArray* a, size_t index, YThread* th) {
	YList* list = (YList*) a;
	if (index >= list->length)
		return;
	MUTEX_LOCK(&list->mutex);
	size_t i = 0;
	ListEntry* e = list->list;
	while (e != NULL) {
		if (i == index) {
			if (e->prev != NULL)
				e->prev->next = e->next;
			else
				list->list = e->next;
			if (e->next != NULL)
				e->next->prev = e->prev;
			list->length--;
			free(e);
			break;
		}
		i++;
		e = e->next;
	}
	MUTEX_UNLOCK(&list->mutex);
}

YArray* newList(YThread* th) {
	YList* list = malloc(sizeof(YList));
	initYoyoObject((YoyoObject*) list, List_mark, List_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) list);
	list->array.parent.type = &th->runtime->ArrayType;

	NEW_MUTEX(&list->mutex);
	list->length = 0;
	list->list = NULL;

	list->array.size = List_size;
	list->array.get = List_get;
	list->array.add = List_add;
	list->array.set = List_set;
	list->array.insert = List_insert;
	list->array.remove = List_remove;
	list->array.toString = NULL;

	return (YArray*) list;
}

