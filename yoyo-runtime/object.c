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

/*File contains two implementations of YObject:
 * TreeObject - implementation based on self-balanced
 * 	tree
 * HashObject - implementation based on hash map*/

/*Type definitions for both TreeObject and HashObject*/
typedef struct TreeObjectField {
	int32_t key;
	YoyoType* type;
	YValue* value;
	struct TreeObjectField* parent;
	struct TreeObjectField* right;
	struct TreeObjectField* left;
	int32_t height;
} TreeObjectField;

typedef struct TreeObject {
	YObject parent;
	YObject* super;
	TreeObjectField* root;
	MUTEX access_mutex;
} TreeObject;


/*Memory allocators to increase structure allocation speed.
 * These allocators being initialized on the first
 * object construction and never freed*/
MemoryAllocator* TreeObjectAlloc = NULL;
MemoryAllocator* LeaveAlloc = NULL;

/*Methods to balance tree. See self-balanced trees.*/
int32_t Leave_fixheight(TreeObjectField* f) {
	if (f == NULL)
		return 0;
	f->height = Leave_fixheight(f->left) + Leave_fixheight(f->right);
	return f->height + 1;
}
TreeObjectField* Leave_rotateLeft(TreeObjectField* f) {
	if (f == NULL || f->right == NULL)
		return f;
	TreeObjectField* p = f->right;
	f->right = p->left;
	p->left = f;
	Leave_fixheight(p);
	return p;
}
TreeObjectField* Leave_rotateRight(TreeObjectField* f) {
	if (f == NULL || f->left == NULL)
		return f;
	TreeObjectField* p = f->left;
	f->left = p->right;
	p->right = f;
	Leave_fixheight(p);
	return p;
}
TreeObjectField* Leave_balance(TreeObjectField* f) {
	if (f == NULL)
		return NULL;
	Leave_fixheight(f);
#define factor(n) (n==NULL ? 0 : (n->right!=NULL ? n->right->height : 0) - (n->left!=NULL ? n->left->height : 0))

	if (factor(f) >= 2) {
		if (factor(f->right) < 0)
			f->right = Leave_rotateRight(f->right);
		return Leave_rotateLeft(f);
	} else if (factor(f) <= -2) {
		if (factor(f->left) > 0)
			f->left = Leave_rotateLeft(f->left);
		return Leave_rotateRight(f);
	}
	return f;

#undef factor
}

/*Methods that work with trees. These methods are used
 * by appropriate object methods*/
bool Leave_contains(TreeObjectField* f, int32_t key) {
	if (f == NULL)
		return false;
	if (f->key == key)
		return true;
	if (key > f->key)
		return Leave_contains(f->right, key);
	else
		return Leave_contains(f->left, key);
}
YValue* Leave_get(TreeObjectField* f, int32_t key) {
	if (f == NULL)
		return NULL;
	if (f->key == key)
		return f->value;
	else if (key > f->key)
		return Leave_get(f->right, key);
	else
		return Leave_get(f->left, key);
}
TreeObjectField* Leave_search(TreeObjectField* f, int32_t key) {
	if (f == NULL)
		return NULL;
	if (f->key == key)
		return f;
	else if (key > f->key)
		return Leave_search(f->right, key);
	else
		return Leave_search(f->left, key);
}
TreeObjectField* Leave_put(TreeObjectField* parent, TreeObjectField* root,
		int32_t key, YValue* value) {
	if (root == NULL) {
		// Initialize root
		root = LeaveAlloc->alloc(LeaveAlloc);
		root->key = key;
		root->type = NULL;
		root->value = value;
		root->parent = parent;
		root->right = NULL;
		root->left = NULL;
		root->height = 0;
	} else {
		if (root->key == key)
			root->value = value;
		else {
			if (key > root->key) {
				root->right = Leave_balance(
						Leave_put(root, root->right, key, value));
			} else {
				root->left = Leave_balance(
						Leave_put(root, root->left, key, value));
			}
		}
	}
	return Leave_balance(root);
}

void Leave_mark(TreeObjectField* root) {
	if (root == NULL)
		return;
	MARK(root->type);
	MARK(root->value);
	Leave_mark(root->right);
	Leave_mark(root->left);
}
void Leave_free(TreeObjectField* root) {
	if (root == NULL)
		return;
	Leave_free(root->left);
	Leave_free(root->right);
	LeaveAlloc->unuse(LeaveAlloc, root);
}
/*Method adds signle leave to the tree*/
void Leave_add(TreeObjectField* root, TreeObjectField* leave) {
	if (root == NULL || leave == NULL)
		return;
	if (root->key == leave->key)
		root->value = leave->value;
	else {
		if (leave->key > root->key) {
			if (root->right == NULL)
				root->right = leave;
			else
				Leave_add(root->right, leave);
		} else {
			if (root->left == NULL)
				root->left = leave;
			else
				Leave_add(root->left, leave);
		}
	}
}
/*Method removed leave from tree.
 * After removing tree should be updated, because
 * removed leave subtrees are added to main tree*/
TreeObjectField* Leave_remove(TreeObjectField* root, TreeObjectField* leave,
		int32_t key) {
	if (leave == NULL)
		return root;
	if (key > leave->key) {
		Leave_remove(root, leave->right, key);
	} else if (key < leave->key) {
		Leave_remove(root, leave->left, key);
	} else {
		if (leave == root) {
			TreeObjectField* newRoot = NULL;
			if (root->left != NULL) {
				Leave_add(root->left, root->right);
				newRoot = root->left;
			} else if (root->right != NULL)
				newRoot = root->right;
			LeaveAlloc->unuse(LeaveAlloc, root);
			root = newRoot;
		} else {
			if (leave->key > leave->parent->key)
				leave->parent->right = NULL;
			else
				leave->parent->left = NULL;
			Leave_add(root, leave->right);
			Leave_add(root, leave->left);
			free(leave);
		}
	}
	return Leave_balance(root);
}

/*Methods that implement YObject interface.
 * These methods use methods above to work with tree*/
bool TreeObject_contains(YObject* o, int32_t key, YThread* th) {
	TreeObject* obj = (TreeObject*) o;
	return Leave_contains(obj->root, key)
			|| (obj->super != NULL && obj->super->contains(obj->super, key, th));
}
YValue* TreeObject_get(YObject* o, int32_t key, YThread* th) {
	TreeObject* obj = (TreeObject*) o;
	YValue* val = Leave_get(obj->root, key);
	if (val != NULL)
		return val;
	if (obj->super != NULL)
		return obj->super->get(obj->super, key, th);
	else {
		wchar_t* name = getSymbolById(&th->runtime->symbols, key);
		throwException(L"UnknownField", &name, 1, th);
		return getNull(th);
	}
}
void TreeObject_put(YObject* o, int32_t key, YValue* value, bool newF,
		YThread* th) {
	MARK(value);
	TreeObject* obj = (TreeObject*) o;
	if (newF || Leave_contains(obj->root, key)) {
		obj->root = Leave_put(NULL, obj->root, key, value);
		TreeObjectField* fld = Leave_search(obj->root, key);
		if (fld->type != NULL && !fld->type->verify(fld->type, value, th)) {
			wchar_t* name = getSymbolById(&th->runtime->symbols, key);
			throwException(L"WrongFieldType", &name, 1, th);
		}
	} else {
		if (obj->super != NULL && obj->super->contains(obj->super, key, th))
			obj->super->put(obj->super, key, value, false, th);
		else {
			obj->root = Leave_put(NULL, obj->root, key, value);
			TreeObjectField* fld = Leave_search(obj->root, key);
			if (fld->type != NULL && !fld->type->verify(fld->type, value, th)) {
				wchar_t* name = getSymbolById(&th->runtime->symbols, key);
				throwException(L"WrongFieldType", &name, 1, th);
			}
		}
	}
}
void TreeObject_remove(YObject* o, int32_t key, YThread* th) {
	TreeObject* obj = (TreeObject*) o;
	if (Leave_contains(obj->root, key))
		obj->root = Leave_remove(obj->root, obj->root, key);
	else if (obj->super != NULL)
		obj->super->remove(obj->super, key, th);
	else {
		wchar_t* name = getSymbolById(&th->runtime->symbols, key);
		throwException(L"UnknownField", &name, 1, th);
	}
}

void TreeObject_setType(YObject* o, int32_t key, YoyoType* type, YThread* th) {
	TreeObject* obj = (TreeObject*) o;
	MARK(type);
	if (o->contains(o, key, th)) {
		TreeObjectField* f = Leave_search(obj->root, key);
		if (f != NULL) {
			f->type = type;
			if (!type->verify(type, f->value, th)) {
				wchar_t* name = getSymbolById(&th->runtime->symbols, key);
				throwException(L"WrongFieldType", &name, 1, th);
			}
		}
	} else if (obj->super != NULL && obj->super->contains(obj->super, key, th))
		obj->super->setType(obj->super, key, type, th);
}
YoyoType* TreeObject_getType(YObject* o, int32_t key, YThread* th) {
	TreeObject* obj = (TreeObject*) o;
	if (o->contains(o, key, th)) {
		TreeObjectField* f = Leave_search(obj->root, key);
		if (f != NULL && f->type != NULL)
			return f->type;
	} else if (obj->super != NULL && obj->super->contains(obj->super, key, th))
		return obj->super->getType(obj->super, key, th);
	return th->runtime->NullType.TypeConstant;
}

void TreeObject_mark(YoyoObject* ptr) {
	ptr->marked = true;
	TreeObject* obj = (TreeObject*) ptr;
	MUTEX_LOCK(&obj->access_mutex);
	if (obj->super != NULL)
		MARK(obj->super);
	Leave_mark(obj->root);
	MUTEX_UNLOCK(&obj->access_mutex);
}
void TreeObject_free(YoyoObject* ptr) {
	TreeObject* obj = (TreeObject*) ptr;
	DESTROY_MUTEX(&obj->access_mutex);
	Leave_free(obj->root);
	TreeObjectAlloc->unuse(TreeObjectAlloc, obj);
}
/*Procedures that build YObject instances,
 * initialize allocators and nescesarry data structures*/
YObject* newTreeObject(YObject* parent, YThread* th) {
	if (LeaveAlloc == NULL) {
		// It's the first TreeObject allocation
		LeaveAlloc = newMemoryAllocator(sizeof(TreeObjectField), 250);
		TreeObjectAlloc = newMemoryAllocator(sizeof(TreeObject), 250);
	}
	TreeObject* obj = TreeObjectAlloc->alloc(TreeObjectAlloc);
	initYoyoObject(&obj->parent.parent.o, TreeObject_mark, TreeObject_free);
	th->runtime->gc->registrate(th->runtime->gc, &obj->parent.parent.o);
	NEW_MUTEX(&obj->access_mutex);
	obj->parent.parent.type = &th->runtime->ObjectType;

	obj->super = parent;
	obj->root = NULL;

	obj->parent.get = TreeObject_get;
	obj->parent.contains = TreeObject_contains;
	obj->parent.put = TreeObject_put;
	obj->parent.remove = TreeObject_remove;
	obj->parent.getType = TreeObject_getType;
	obj->parent.setType = TreeObject_setType;

	return (YObject*) obj;
}

MemoryAllocator* HashTableAlloc = NULL;
MemoryAllocator* EntryAlloc = NULL;

typedef struct HashTableEntry {
	int32_t id;
	YValue* value;
	YoyoType* type;
	struct HashTableEntry* prev;
} HashTableEntry;

typedef struct HashTableObject {
	YObject parent;

	MUTEX mutex;
	YObject* super;
	HashTableEntry** table;
	size_t size;
	size_t col_count;
	float factor;
} HashTableObject;

HashTableEntry* HashTable_get(HashTableObject* obj, int32_t id) {
	size_t index = (size_t) (id % obj->size);
	HashTableEntry* e = obj->table[index];
	while (e!=NULL) {
		if (e->id == id)
			return e;
		e = e->prev;
	}
	return NULL;
}

void HashTable_remap(HashTableObject* obj) {
	size_t newSize = obj->size * (1+obj->factor);
	HashTableEntry** newTable = calloc(newSize, sizeof(HashTableEntry*));
	obj->col_count = 0;
	for (size_t i=0;i<obj->size;i++) {
		HashTableEntry* e = obj->table[i];
		while (e!=NULL) {
			HashTableEntry* prev = e->prev;
			size_t index = (size_t) (e->id % newSize);
			e->prev = newTable[index];
			newTable[index] = e;
			if (e->prev!=NULL)
				obj->col_count++;
			e = prev;
		}
	}
	obj->size = newSize;
	free(obj->table);
	obj->table = newTable;
}

HashTableEntry* HashTable_put(HashTableObject* obj, int32_t id) {
	size_t index = (size_t) (id % obj->size);
	HashTableEntry* e = obj->table[index];
	while (e!=NULL) {
		if (e->id == id)
			return e;
		e = e->prev;
	}
	e = EntryAlloc->alloc(EntryAlloc);
	e->id = id;
	e->value = NULL;
	e->type = NULL;
	e->prev = obj->table[index];
	if (e->prev!=NULL)
		obj->col_count++;
	obj->table[index] = e;
	if ((float) obj->col_count / (float) obj->size > obj->factor)
		HashTable_remap(obj);
	return e;
}	

bool HashTable_remove(HashTableObject* obj, int32_t id) {
	size_t index = (size_t) (id % obj->size);
	HashTableEntry* e = obj->table[index];
	HashTableEntry* next = NULL;
	while (e!=NULL) {
		if (e->id == id)
			break;
		next = e;
		e = e->prev;
	}
	if (e==NULL)
		return false;
	if (next==NULL) {
		obj->table[index] = e->prev;
	}
	else {
		next->prev = e->prev;
	}
	EntryAlloc->unuse(EntryAlloc, e);
	return true;
}

bool HashObject_contains(YObject* o, int32_t id, YThread* th) {
	HashTableObject* obj = (HashTableObject*) o;
	MUTEX_LOCK(&obj->mutex);
	bool res = false;
	HashTableEntry* e = HashTable_get(obj, id);
	if (e!=NULL)
		res = true;
	else if (obj->super!=NULL)
		res = obj->super->contains(obj->super, id, th);
	MUTEX_UNLOCK(&obj->mutex);
	return res;
}

YValue* HashObject_get(YObject* o, int32_t id, YThread* th) {
	HashTableObject* obj = (HashTableObject*) o;
	MUTEX_LOCK(&obj->mutex);
	YValue* res = getNull(th);
	HashTableEntry* e = HashTable_get(obj, id);
	if (e!=NULL) {
		res = e->value;
	} else if (obj->super!=NULL) {
		res = obj->super->get(obj->super, id, th);
	} else {
		wchar_t* name = getSymbolById(&th->runtime->symbols, id);
		throwException(L"UnknownField", &name, 1, th);
	}
	MUTEX_UNLOCK(&obj->mutex);
	return res;
}

void HashObject_put(YObject* o, int32_t id, YValue* val, bool newF, YThread* th) {
	HashTableObject* obj = (HashTableObject*) o;
	MUTEX_LOCK(&obj->mutex);
	HashTableEntry* e = HashTable_get(obj, id);
	if (newF || e!=NULL) {
		if (e==NULL)
			e = HashTable_put(obj, id);
		if (e->type!=NULL && !e->type->verify(e->type, val, th)) {
				wchar_t* name = getSymbolById(&th->runtime->symbols, id);
				throwException(L"WrongFieldType", &name, 1, th);
		} else {
			e->value = val;
		}
	} else if (obj->super!=NULL&&
							obj->super->contains(obj->super, id, th)) {
		obj->super->put(obj->super, id, val, newF, th);
	} else {
		e = HashTable_put(obj, id);
		if (e->type!=NULL && !e->type->verify(e->type, val, th)) {
				wchar_t* name = getSymbolById(&th->runtime->symbols, id);
				throwException(L"WrongFieldType", &name, 1, th);
		} else {
			e->value = val;
		}
	}
	MUTEX_UNLOCK(&obj->mutex);
}

void HashObject_remove(YObject* o, int32_t id, YThread* th) {
	HashTableObject* obj = (HashTableObject*) o;
	MUTEX_LOCK(&obj->mutex);
	if (HashTable_remove(obj, id)) {}
	else if (obj->super!=NULL) {
		obj->super->remove(obj->super, id, th);
	}	else {
		wchar_t* name = getSymbolById(&th->runtime->symbols, id);
		throwException(L"UnknownField", &name, 1, th);
	}
	MUTEX_UNLOCK(&obj->mutex);
}

void HashObject_setType(YObject* o, int32_t id, YoyoType* type, YThread* th) {
	HashTableObject* obj = (HashTableObject*) o;
	MUTEX_LOCK(&obj->mutex);
	HashTableEntry* e = HashTable_get(obj, id);
	if (e!=NULL) {
		e->type = type;
		if (!type->verify(type, e->value, th)) {
				wchar_t* name = getSymbolById(&th->runtime->symbols, id);
				throwException(L"WrongFieldType", &name, 1, th);
		}
	} else if (obj->super!=NULL) {
		obj->super->setType(obj->super, id, type, th);
	} else {
		wchar_t* name = getSymbolById(&th->runtime->symbols, id);
		throwException(L"UnknownField", &name, 1, th);
	}
	MUTEX_UNLOCK(&obj->mutex);
}

YoyoType* HashObject_getType(YObject* o, int32_t id, YThread* th) {
	HashTableObject* obj = (HashTableObject*) o;
	MUTEX_LOCK(&obj->mutex);
	YoyoType* res = th->runtime->NullType.TypeConstant;
	HashTableEntry* e = HashTable_get(obj, id);
	if (e!=NULL) {
		res = e->type;
	} else if (obj->super!=NULL) {
		res = obj->super->getType(obj->super, id, th);
	} else {
		wchar_t* name = getSymbolById(&th->runtime->symbols, id);
		throwException(L"UnknownField", &name, 1, th);
	}
	MUTEX_UNLOCK(&obj->mutex);
	return res;
}

void HashObject_mark(YoyoObject* ptr) {
	ptr->marked = true;
	HashTableObject* obj = (HashTableObject*) ptr;
	MUTEX_LOCK(&obj->mutex);
	MARK(obj->super);
	for (size_t i=0;i<obj->size;i++) {
		for (HashTableEntry* e = obj->table[i]; e!=NULL; e = e->prev) {
			MARK(e->type);
			MARK(e->value);
		}
	}
	MUTEX_UNLOCK(&obj->mutex);
}

void HashObject_free(YoyoObject* ptr) {
	HashTableObject* obj = (HashTableObject*) ptr;
	for (size_t i=0;i<obj->size;i++) {
		HashTableEntry* e = obj->table[i];
		while (e!=NULL) {
			HashTableEntry* prev = e->prev;
			EntryAlloc->unuse(EntryAlloc, e);
			e = prev;
		}
	}
	free(obj->table);
	DESTROY_MUTEX(&obj->mutex);
	HashTableAlloc->unuse(HashTableAlloc, obj);	
}

YObject* newHashObject(YObject* super, YThread* th) {
	if (HashTableAlloc == NULL) {
		HashTableAlloc = newMemoryAllocator(sizeof(HashTableObject), 250);
		EntryAlloc = newMemoryAllocator(sizeof(HashTableEntry), 250);
	}
	HashTableObject* obj = HashTableAlloc->alloc(HashTableAlloc);
	initYoyoObject((YoyoObject*) obj, HashObject_mark, HashObject_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) obj);
	obj->parent.parent.type = &th->runtime->ObjectType;

	obj->super = super;
	obj->factor = 0.25;
	obj->col_count = 0;
	obj->size = 10;
	obj->table = calloc(obj->size, sizeof(HashTableEntry*));
	NEW_MUTEX(&obj->mutex);

	obj->parent.contains = HashObject_contains;
	obj->parent.get = HashObject_get;
	obj->parent.put = HashObject_put;
	obj->parent.remove = HashObject_remove;
	obj->parent.setType = HashObject_setType;
	obj->parent.getType = HashObject_getType;

	return (YObject*) obj;
}
