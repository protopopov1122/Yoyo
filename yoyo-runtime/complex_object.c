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

/*File contains implementation of YObject interface.
 * ComplexObject includes one base object and a list
 * of mixins. All methods that modify object
 * state invoke base object methods, but
 * methods get() and contains() call
 * base object methods and all mixin
 * methods*/

typedef struct ComplexObject {
	YObject parent;

	YObject* base;
	YObject** mixins;
	size_t mixin_count;
} ComplexObject;

void ComplexObject_mark(YoyoObject* ptr) {
	ptr->marked = true;
	ComplexObject* obj = (ComplexObject*) ptr;

	MARK(obj->base);
	for (size_t i = 0; i < obj->mixin_count; i++) {
		MARK(obj->mixins[i]);
	}
}
void ComplexObject_free(YoyoObject* ptr) {
	ComplexObject* obj = (ComplexObject*) ptr;
	free(obj->mixins);
	free(obj);
}

YValue* ComplexObject_get(YObject* o, int32_t key, YThread* th) {
	ComplexObject* obj = (ComplexObject*) o;
	if (obj->base->contains(obj->base, key, th))
		return obj->base->get(obj->base, key, th);
	for (size_t i = 0; i < obj->mixin_count; i++)
		if (obj->mixins[i]->contains(obj->mixins[i], key, th))
			return obj->mixins[i]->get(obj->mixins[i], key, th);
	wchar_t* sym = getSymbolById(&th->runtime->symbols, key);
	throwException(L"UnknownField", &sym, 1, th);
	return getNull(th);
}

bool ComplexObject_contains(YObject* o, int32_t key, YThread* th) {
	ComplexObject* obj = (ComplexObject*) o;
	if (obj->base->contains(obj->base, key, th))
		return true;
	for (size_t i = 0; i < obj->mixin_count; i++)
		if (obj->mixins[i]->contains(obj->mixins[i], key, th))
			return true;
	return false;
}

void ComplexObject_put(YObject* o, int32_t key, YValue* value, bool newF,
		YThread* th) {
	ComplexObject* obj = (ComplexObject*) o;
	obj->base->put(obj->base, key, value, newF, th);
}

void ComplexObject_remove(YObject* o, int32_t key, YThread* th) {
	ComplexObject* obj = (ComplexObject*) o;
	obj->base->remove(obj->base, key, th);
}

void ComplexObject_setType(YObject* o, int32_t key, YoyoType* type, YThread* th) {
	ComplexObject* obj = (ComplexObject*) o;
	obj->base->setType(obj->base, key, type, th);
}
YoyoType* ComplexObject_getType(YObject* o, int32_t key, YThread* th) {
	ComplexObject* obj = (ComplexObject*) o;
	return obj->base->getType(obj->base, key, th);
}

YObject* newComplexObject(YObject* base, YObject** mixins, size_t mixinc,
		YThread* th) {
	ComplexObject* obj = malloc(sizeof(ComplexObject));
	initYoyoObject(&obj->parent.parent.o, ComplexObject_mark,
			ComplexObject_free);
	th->runtime->gc->registrate(th->runtime->gc, &obj->parent.parent.o);
	obj->parent.parent.type = &th->runtime->ObjectType;

	obj->base = base;
	obj->mixin_count = mixinc;
	obj->mixins = malloc(sizeof(YObject*) * mixinc);
	for (size_t i = 0; i < mixinc; i++)
		obj->mixins[i] = mixins[i];

	obj->parent.get = ComplexObject_get;
	obj->parent.contains = ComplexObject_contains;
	obj->parent.put = ComplexObject_put;
	obj->parent.remove = ComplexObject_remove;
	obj->parent.getType = ComplexObject_getType;
	obj->parent.setType = ComplexObject_setType;

	return (YObject*) obj;
}
