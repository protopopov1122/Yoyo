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
#include "map.h"

#define NEW_METHOD(name, proc, argc, obj, ptr, th) obj->put(obj, getSymbolId(\
                                                    &th->runtime->symbols, name), (YValue*) newNativeLambda(argc, proc,\
                                                                                    (YoyoObject*) ptr, th),\
                                                            true, th);
#define INIT_HASHMAP AbstractYoyoMap* map = (AbstractYoyoMap*) ((NativeLambda*) lambda)->object;
YOYO_FUNCTION(Map_size) {
	INIT_HASHMAP
	;
	return newInteger(map->col.size((AbstractYoyoCollection*) map, th), th);
}
YOYO_FUNCTION(Map_get) {
	INIT_HASHMAP
	;
	return map->get(map, args[0], th);
}
YOYO_FUNCTION(Map_put) {
	INIT_HASHMAP
	;
	map->put(map, args[0], args[1], th);
	return getNull(th);
}
YOYO_FUNCTION(Map_has) {
	INIT_HASHMAP
	;
	return newBoolean(map->has(map, args[0], th), th);
}
YOYO_FUNCTION(Map_remove) {
	INIT_HASHMAP
	;
	map->remove(map, args[0], th);
	return getNull(th);
}
YOYO_FUNCTION(Map_clear) {
	// TODO
	return getNull(th);
}
YOYO_FUNCTION(Map_keys) {
	INIT_HASHMAP;
	return (YValue*) newYoyoSet(map->keySet(map, th), th);
}
#undef INIT_HASHMAP
#define INIT_SET AbstractYoyoSet* set = (AbstractYoyoSet*) ((NativeLambda*) lambda)->object;

YOYO_FUNCTION(Set_size) {
	INIT_SET
	;
	return newInteger(set->col.size((AbstractYoyoCollection*) set, th), th);
}
YOYO_FUNCTION(Set_add) {
	INIT_SET
	;
	set->add(set, args[0], th);
	return getNull(th);
}
YOYO_FUNCTION(Set_has) {
	INIT_SET
	;
	return newBoolean(set->has(set, args[0], th), th);
}
YOYO_FUNCTION(Set_iter) {
	INIT_SET
	;
	return (YValue*) set->iter(set, th);
}

#undef INIT_SET

YObject* newYoyoMap(AbstractYoyoMap* map, YThread* th) {
	YObject* out = th->runtime->newObject(NULL, th);
	NEW_METHOD(L"size", Map_size, 0, out, map, th);
	NEW_METHOD(L"get", Map_get, 1, out, map, th);
	NEW_METHOD(L"put", Map_put, 2, out, map, th);
	NEW_METHOD(L"has", Map_has, 1, out, map, th);
	NEW_METHOD(L"remove", Map_remove, 1, out, map, th);
	NEW_METHOD(L"clear", Map_clear, 0, out, map, th);
	NEW_METHOD(L"keys", Map_keys, 0, out, map, th);
	return out;
}

YObject* newYoyoSet(AbstractYoyoSet* set, YThread* th) {
	YObject* out = th->runtime->newObject(NULL, th);
	NEW_METHOD(L"size", Set_size, 0, out, set, th);
	NEW_METHOD(L"add", Set_add, 1, out, set, th);
	NEW_METHOD(L"has", Set_has, 1, out, set, th);
	NEW_METHOD(L"iter", Set_iter, 0, out, set, th);
	return out;
}

YOYO_FUNCTION(YSTD_COLLECTIONS_HASH_MAP_NEW) {
	AbstractYoyoMap* map = newYoyoHashMap(th);
	return (YValue*) newYoyoMap(map, th);
}

YOYO_FUNCTION(YSTD_COLLECTIONS_HASH_SET_NEW) {
	return getNull(th); // TODO
}

YOYO_FUNCTION(YSTD_COLLECTIONS_LIST_NEW) {
	return (YValue*) newList(th);
}
