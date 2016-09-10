#include "yoyo-runtime.h"
#include "map.h"

typedef struct HashMapEntry {
	YValue* key;
	YValue* value;
	MUTEX mutex;
	struct HashMapEntry* prev;
} HashMapEntry;

typedef struct YoyoHashMap {
	AbstractYoyoMap parent;

	HashMapEntry** map;
	float factor;
	size_t size;
	size_t fullSize;
	size_t col_count;
	MUTEX mutex;
} YoyoHashMap;

void HashMap_mark(YoyoObject* ptr) {
	ptr->marked = true;
	YoyoHashMap* map = (YoyoHashMap*) ptr;
	MUTEX_LOCK(&map->mutex);
	for (size_t i=0;i<map->size;i++) {
		for (HashMapEntry* e = map->map[i]; e != NULL; e = e->prev) {
			MARK(e->key);
			MARK(e->value);
		}
	}
	MUTEX_UNLOCK(&map->mutex);
}

void HashMap_free(YoyoObject* ptr) {
	YoyoHashMap* map = (YoyoHashMap*) ptr;
	for (size_t i=0;i<map->size;i++) {
		HashMapEntry* e = map->map[i];
		while (e!=NULL) {
			HashMapEntry* prev = e->prev;
			DESTROY_MUTEX(&e->mutex);
			free(e);
			e = prev;	
		}	
	}
	DESTROY_MUTEX(&map->mutex);
	free(map->map);	
	free(map);
}

typedef struct HashMap_KeySet {
	AbstractYoyoSet parent;

	YoyoHashMap* map;
} HashMap_KeySet;

void HashMap_KeySet_mark(YoyoObject* ptr) {
	ptr->marked = true;
	HashMap_KeySet* set = (HashMap_KeySet*) ptr;
	MARK(set->map);
}

size_t HashMap_KeySet_size(AbstractYoyoCollection* col, YThread* th) {
	HashMap_KeySet* set = (HashMap_KeySet*) col;
	return set->map->parent.col.size((AbstractYoyoCollection*) set->map, th);
}

bool HashMap_KeySet_add(AbstractYoyoSet* s, YValue* val, YThread* th) {
	return false;
}

bool HashMap_KeySet_remove(AbstractYoyoSet* s, YValue* val, YThread* th) {
	return false;
}

bool HashMap_KeySet_has(AbstractYoyoSet* s, YValue* val, YThread* th) {
	HashMap_KeySet* set = (HashMap_KeySet*) s;
	return set->map->parent.has((AbstractYoyoMap*) set->map, val, th);
}

typedef struct HashMapKeyIter {
	YoyoIterator iter;

	YoyoHashMap* map;
	size_t index;
	HashMapEntry* entry;
} HashMapKeyIter;

void HashMapKeyIter_mark(YoyoObject* ptr) {
	ptr->marked = true;
	HashMapKeyIter* iter = (HashMapKeyIter*) ptr;
	MARK(iter->map);
}

void HashMapKeyIter_findNext(HashMapKeyIter* iter) {
	MUTEX_LOCK(&iter->map->mutex);
	if (iter->entry != NULL) {
		MUTEX_UNLOCK(&iter->entry->mutex);
		iter->entry = iter->entry->prev;
	}
	while (iter->entry == NULL) {
		iter->index++;
		if (iter->index >= iter->map->size)
			break;
		iter->entry = iter->map->map[iter->index];
	}
	if (iter->entry!=NULL) {
		MUTEX_LOCK(&iter->entry->mutex);
	}
	MUTEX_UNLOCK(&iter->map->mutex);
}

void HashMapKeyIter_reset(YoyoIterator* i, YThread* th) {
	HashMapKeyIter* iter = (HashMapKeyIter*) i;
	if (iter->entry!=NULL)
		MUTEX_UNLOCK(&iter->entry->mutex);
	iter->index = 0;
	iter->entry = iter->map->map[iter->index];
	HashMapKeyIter_findNext(iter);
}

bool HashMapKeyIter_hasNext(YoyoIterator* i, YThread* th) {
	HashMapKeyIter* iter = (HashMapKeyIter*) i;
	return iter->entry != NULL;
}

YValue* HashMapKeyIter_next(YoyoIterator* i, YThread* th) {
	HashMapKeyIter* iter = (HashMapKeyIter*) i;
	if (iter->entry == NULL)
		return getNull(th);
	YValue* out = iter->entry->key;
	HashMapKeyIter_findNext(iter);
	return out;
}

void HashMapKeyIter_free(YoyoObject* ptr) {
	HashMapKeyIter* iter = (HashMapKeyIter*) ptr;
	if (iter->entry != NULL)
		MUTEX_UNLOCK(&iter->entry->mutex);
	free(iter);
}

YoyoIterator* HashMap_KeySet_iter(AbstractYoyoSet* s, YThread* th) {
	HashMapKeyIter* iter = calloc(1, sizeof(HashMapKeyIter));
	initYoyoObject((YoyoObject*) iter, HashMapKeyIter_mark, HashMapKeyIter_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) iter);

	iter->map = ((HashMap_KeySet*) s)->map;
	iter->index = 0;
	iter->entry = iter->map->map[iter->index];
	if (iter->entry == NULL)
		HashMapKeyIter_findNext(iter);
	else
		MUTEX_LOCK(&iter->entry->mutex);

	YoyoIterator_init((YoyoIterator*) iter, th);
	
	iter->iter.reset = HashMapKeyIter_reset;
	iter->iter.hasNext = HashMapKeyIter_hasNext;
	iter->iter.next = HashMapKeyIter_next;

	return (YoyoIterator*) iter;
}

AbstractYoyoSet* HashMap_keySet(AbstractYoyoMap* m, YThread* th) {
	HashMap_KeySet* set = calloc(1, sizeof(HashMap_KeySet));
	initYoyoObject((YoyoObject*) set, HashMap_KeySet_mark, (void (*)(YoyoObject*)) free);	
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) set);
	
	set->map = (YoyoHashMap*) m;
	set->parent.col.size = HashMap_KeySet_size;
	set->parent.add = HashMap_KeySet_add;
	set->parent.has = HashMap_KeySet_has;
	set->parent.remove = HashMap_KeySet_remove;
	set->parent.iter = HashMap_KeySet_iter;	

	return (AbstractYoyoSet*) set;
}

YValue* HashMap_get(struct AbstractYoyoMap* m, YValue* key, YThread* th) {
	YoyoHashMap* map = (YoyoHashMap*) m;
	MUTEX_LOCK(&map->mutex);
	YValue* result = NULL;
	uint64_t hash = key->type->oper.hashCode(key, th);
	size_t index = (size_t) (hash % map->size);
	HashMapEntry* e = map->map[index];
	while (e!=NULL) {
		if (CHECK_EQUALS(e->key, key, th)) {
			break;
		}
		e = e->prev;
	}
	if (e!=NULL)
		result = e->value;
	MUTEX_UNLOCK(&map->mutex);
	return result;
}

bool HashMap_has(struct AbstractYoyoMap* m, YValue* key, YThread* th) {
	return HashMap_get(m, key, th)!=NULL;
}

void HashMap_remap(YoyoHashMap* map, YThread* th) {
	size_t newSize = map->size * 1.1;
	HashMapEntry** newMap = calloc(newSize, sizeof(HashMapEntry*));
	map->col_count = 0;
	for (size_t i = 0;i < map->size;i++) {
		HashMapEntry* e = map->map[i];
		HashMapEntry* prev = NULL;
		while (e!=NULL) {
			prev = e->prev;

			MUTEX_LOCK(&e->mutex);
			uint64_t hash = e->key->type->oper.hashCode(e->key, th);
			size_t index = (size_t) (hash % newSize);
			e->prev = newMap[index];
			newMap[index] = e;
			if (e->prev != NULL)
				map->col_count++;
			MUTEX_UNLOCK(&e->mutex);

			e = prev;
		}
	}
	map->size = newSize;
	free(map->map);
	map->map = newMap;
}

void HashMap_put(struct AbstractYoyoMap* m, YValue* key, YValue* value, YThread* th) {
	YoyoHashMap* map = (YoyoHashMap*) m;
	MUTEX_LOCK(&map->mutex);
	uint64_t hash = key->type->oper.hashCode(key, th);
	size_t index = (size_t) (hash % map->size);
	HashMapEntry* e = map->map[index];
	while (e!=NULL) {
		if (CHECK_EQUALS(e->key, key, th)) {
			break;
		}
		e = e->prev;
	}
	if (e!=NULL) {
		e->value = value;
	} else {
		HashMapEntry* e = malloc(sizeof(HashMapEntry));
		e->key = key;
		e->value = value;
		e->prev = map->map[index];
		NEW_MUTEX(&e->mutex);
		map->map[index] = e;
		map->fullSize++;
		if (e->prev!=NULL) {
			map->col_count++;
			if (map->col_count >= map->factor * map->size)
				HashMap_remap(map, th);
		}
	}
	MUTEX_UNLOCK(&map->mutex);
}

bool HashMap_remove(AbstractYoyoMap* m, YValue* key, YThread* th) {
	YoyoHashMap* map = (YoyoHashMap*) m;
	MUTEX_LOCK(&map->mutex);
	uint64_t hash = key->type->oper.hashCode(key, th);
	size_t index = (size_t) (hash % map->size);
	HashMapEntry* next = NULL;
	HashMapEntry* e = map->map[index];
	while (e != NULL) {
		if (CHECK_EQUALS(e->key, key, th)) {
			break;
		}
		next = e;
		e = e->prev;
	}
	bool res = false;
	if (e!=NULL) {
		MUTEX_LOCK(&e->mutex);
		if (next != NULL)
			next->prev = e->prev;
		else
			map->map[index] = e->prev;
		if (e->prev != NULL ||
			next != NULL) {
			map->col_count--;
		}
		map->fullSize--;
		MUTEX_UNLOCK(&e->mutex);
		DESTROY_MUTEX(&e->mutex);
		free(e);
		res = true;
	}
	MUTEX_UNLOCK(&map->mutex);
	return res;
}

size_t HashMap_size(AbstractYoyoCollection* col, YThread* th) {
	return ((YoyoHashMap*) col)->fullSize;
}


AbstractYoyoMap* newHashMap(YThread* th) {
	YoyoHashMap* map = calloc(1, sizeof(YoyoHashMap));
	initYoyoObject((YoyoObject*) map, HashMap_mark, HashMap_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) map);

	NEW_MUTEX(&map->mutex);
	map->factor = 0.25f;
	map->fullSize = 0;
	map->size = 10;
	map->col_count = 0;
	map->map = calloc(map->size, sizeof(HashMapEntry*));

	map->parent.col.size = HashMap_size;
	map->parent.put = HashMap_put;
	map->parent.get = HashMap_get;
	map->parent.has = HashMap_has;
	map->parent.remove = HashMap_remove;
	map->parent.keySet = HashMap_keySet;

	return (AbstractYoyoMap*) map;
}
