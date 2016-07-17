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

#include  "yoyo.h"

typedef struct NewThread {
	YLambda* lambda;
	YRuntime* runtime;
} NewThread;

void* launch_new_thread(void* ptr) {
	NewThread* nth = (NewThread*) ptr;
	YThread* th = newThread(nth->runtime);
	invokeLambda(nth->lambda, NULL, 0, th);
	th->free(th);
	free(nth);
	THREAD_EXIT(NULL);
	return NULL;
}

YOYO_FUNCTION(Threads_newThread) {
	if (args[0]->type->type == LambdaT) {
		YLambda* lambda = (YLambda*) args[0];
		NewThread* nth = malloc(sizeof(NewThread));
		nth->lambda = lambda;
		nth->runtime = th->runtime;
		THREAD pthr;
		NEW_THREAD(&pthr, launch_new_thread, nth);
	}
	return getNull(th);
}

YOYO_FUNCTION(Threads_yield) {
	YIELD();
	return getNull(th);
}

typedef struct YoyoMutex {
	YObject object;

	MUTEX mutex;
} YoyoMutex;

void YoyoMutex_mark(HeapObject* ptr) {
	ptr->marked = true;
}
void YoyoMutex_free(HeapObject* ptr) {
	YoyoMutex* mutex = (YoyoMutex*) ptr;
	DESTROY_MUTEX(&mutex->mutex);
	free(mutex);
}

YOYO_FUNCTION(YoyoMutex_lock) {
	YoyoMutex* mutex = (YoyoMutex*) ((NativeLambda*) lambda)->object;
	MUTEX_LOCK(&mutex->mutex);
	return getNull(th);
}

YOYO_FUNCTION(YoyoMutex_unlock) {
	YoyoMutex* mutex = (YoyoMutex*) ((NativeLambda*) lambda)->object;
	MUTEX_UNLOCK(&mutex->mutex);
	return getNull(th);
}

YOYO_FUNCTION(YoyoMutex_tryLock) {
	YoyoMutex* mutex = (YoyoMutex*) ((NativeLambda*) lambda)->object;
	return newBoolean(MUTEX_TRYLOCK(&mutex->mutex), th);
}

YValue* YoyoMutex_get(YObject* o, int32_t key, YThread* th) {
	wchar_t* wstr = th->runtime->bytecode->getSymbolById(th->runtime->bytecode,
			key);
	if (wcscmp(wstr, L"lock") == 0)
		return (YValue*) newNativeLambda(0, YoyoMutex_lock, (HeapObject*) o, th);
	if (wcscmp(wstr, L"unlock") == 0)
		return (YValue*) newNativeLambda(0, YoyoMutex_unlock, (HeapObject*) o,
				th);
	if (wcscmp(wstr, L"tryLock") == 0)
		return (YValue*) newNativeLambda(0, YoyoMutex_tryLock, (HeapObject*) o,
				th);
	return getNull(th);
}
bool YoyoMutex_contains(YObject* o, int32_t key, YThread* th) {
	wchar_t* wstr = th->runtime->bytecode->getSymbolById(th->runtime->bytecode,
			key);
	return wcscmp(wstr, L"lock") == 0 || wcscmp(wstr, L"unlock") == 0
			|| wcscmp(wstr, L"tryLock") == 0;
}
void YoyoMutex_put(YObject* o, int32_t key, YValue* v, bool newF, YThread* th) {

}
void YoyoMutex_remove(YObject* o, int32_t key, YThread* th) {

}

YOYO_FUNCTION(Threads_newMutex) {
	YoyoMutex* mutex = malloc(sizeof(YoyoMutex));
	initHeapObject((HeapObject*) mutex, YoyoMutex_mark, YoyoMutex_free);
	th->runtime->gc->registrate(th->runtime->gc, (HeapObject*) mutex);
	mutex->object.parent.type = &th->runtime->ObjectType;
	NEW_MUTEX(&mutex->mutex);

	mutex->object.get = YoyoMutex_get;
	mutex->object.contains = YoyoMutex_contains;
	mutex->object.put = YoyoMutex_put;
	mutex->object.remove = YoyoMutex_remove;

	return (YValue*) mutex;
}
