#include "yoyo-runtime.h"
#include "map.h"
#include "set.h"

typedef struct YoyoMapSet {
	AbstractYoyoSet parent;

	AbstractYoyoMap* map;
} YoyoMapSet;

void YoyoMapSet_mark(YoyoObject* ptr) {
	ptr->marked = true;
	YoyoMapSet* set = (YoyoMapSet*) ptr;
	MARK(set->map);
}

size_t YoyoMapSet_size(AbstractYoyoCollection* s, YThread* th) {
	YoyoMapSet* set = (YoyoMapSet*) s;
	return set->map->col.size((AbstractYoyoCollection*) set->map, th);
}

bool YoyoMapSet_add(AbstractYoyoSet* s, YValue* val, YThread* th) {
	YoyoMapSet* set = (YoyoMapSet*) s;
	set->map->put(set->map, val, val, th);
	return true;
}

bool YoyoMapSet_has(AbstractYoyoSet* s, YValue* val, YThread* th) {
	YoyoMapSet* set = (YoyoMapSet*) s;
	return set->map->has(set->map, val, th);
}

bool YoyoMapSet_remove(AbstractYoyoSet* s, YValue* val, YThread* th) {
	YoyoMapSet* set = (YoyoMapSet*) s;
	return set->map->remove(set->map, val, th); 
}

YoyoIterator* YoyoMapSet_iter(AbstractYoyoSet* s, YThread* th) {
	YoyoMapSet* set = (YoyoMapSet*) s;
	AbstractYoyoSet* kset = set->map->keySet(set->map, th);
	return kset->iter(kset, th);
}

AbstractYoyoSet* newMapBasedSet(AbstractYoyoMap* map, YThread* th) {
	YoyoMapSet* set = calloc(1, sizeof(YoyoMapSet));
	initYoyoObject((YoyoObject*) set, YoyoMapSet_mark, (void (*)(YoyoObject*)) free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) set);
	set->map = map;

	set->parent.col.size = YoyoMapSet_size;
	set->parent.add = YoyoMapSet_add;
	set->parent.has = YoyoMapSet_has;
	set->parent.remove = YoyoMapSet_remove;
	set->parent.iter = YoyoMapSet_iter;

	return (AbstractYoyoSet*) set;
}
