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

YObject* newHashObject(YObject* super, YThread* th) {
	return newTreeObject(super, th); // TODO Rewrite hash map implementation
}
