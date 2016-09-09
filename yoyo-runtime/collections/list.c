#include "yoyo-runtime.h"

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
	if (index >= list->length) {
		YValue* yint = newInteger(index, th);
		wchar_t* wstr = toString(yint, th);
		throwException(L"WrongArrayIndex", &wstr, 1, th);
		free(wstr);
		return getNull(th);
	}
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
	if (index >= list->length) {
		YValue* yint = newInteger(index, th);
		wchar_t* wstr = toString(yint, th);
		throwException(L"WrongArrayIndex", &wstr, 1, th);
		free(wstr);
		return;
	}
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
	if (index > list->length) {
		YValue* yint = newInteger(index, th);
		wchar_t* wstr = toString(yint, th);
		throwException(L"WrongArrayIndex", &wstr, 1, th);
		free(wstr);
		return;
	}
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
	if (index >= list->length) {
		YValue* yint = newInteger(index, th);
		wchar_t* wstr = toString(yint, th);
		throwException(L"WrongArrayIndex", &wstr, 1, th);
		free(wstr);
		return;
	}
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

void List_clear(YArray* a, YThread* th) {
	YList* list = (YList*) a;
	MUTEX_LOCK(&list->mutex);
	list->length = 0;

	for (ListEntry* e = list->list; e != NULL;) {
		ListEntry* ne = e->next;
		free(e);
		e = ne;
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
	list->array.clear = List_clear;
	list->array.toString = NULL;

	return (YArray*) list;
}

