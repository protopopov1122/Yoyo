#include "yoyo-runtime.h"
#include "map.h"

typedef struct HashMapEntry {
	YValue* key;
	YValue* value;
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
			free(e);
			e = prev;	
		}	
	}
	DESTROY_MUTEX(&map->mutex);
	free(map->map);	
	free(map);
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
		map->map[index] = e;
		map->fullSize++;
		if (e->prev!=NULL) {
			map->col_count++;
			// TODO remap
		}
	}
	MUTEX_UNLOCK(&map->mutex);
}


AbstractYoyoMap* newYoyoHashMap(YThread* th) {
	YoyoHashMap* map = calloc(1, sizeof(YoyoHashMap*));
	initYoyoObject((YoyoObject*) map, HashMap_mark, HashMap_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) map);

	NEW_MUTEX(&map->mutex);
	map->factor = 0.25f;
	map->fullSize = 0;
	map->size = 10;
	map->col_count = 0;
	map->map = calloc(map->size, sizeof(HashMapEntry*));

	map->parent.put = HashMap_put;
	map->parent.get = HashMap_get;
	map->parent.has = HashMap_has;
	map->parent.remove = NULL; // TODO

	return (AbstractYoyoMap*) map;
}
