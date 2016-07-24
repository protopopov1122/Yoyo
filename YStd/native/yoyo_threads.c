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

#include  "yoyo-runtime.h"

typedef struct NewThread {
	YLambda* lambda;
	YRuntime* runtime;
} NewThread;

void* launch_new_thread(void* ptr) {
	NewThread* nth = (NewThread*) ptr;
	YThread* th = newThread(nth->runtime);
	invokeLambda(nth->lambda, NULL, NULL, 0, th);
	th->free(th);
	free(nth);
	THREAD_EXIT(NULL);
	return NULL;
}

YOYO_FUNCTION(YSTD_THREADS_NEW_THREAD) {
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

YOYO_FUNCTION(YSTD_THREADS_YIELD) {
	YIELD();
	return getNull(th);
}

void YoyoMutex_free(void* ptr) {
	MUTEX* mutex = (MUTEX*) ptr;
	DESTROY_MUTEX(mutex);
	free(mutex);
}

YOYO_FUNCTION(YSTD_THREADS_MUTEX_LOCK) {
	MUTEX* mutex = (MUTEX*) ((YRawPointer*) ((NativeLambda*) lambda)->object)->ptr;
	MUTEX_LOCK(mutex);
	return getNull(th);
}

YOYO_FUNCTION(YSTD_THREADS_MUTEX_UNLOCK) {
	MUTEX* mutex = (MUTEX*) ((YRawPointer*) ((NativeLambda*) lambda)->object)->ptr;
	MUTEX_UNLOCK(mutex);
	return getNull(th);
}

YOYO_FUNCTION(YSTD_THREADS_MUTEX_TRYLOCK) {
	MUTEX* mutex = (MUTEX*) ((YRawPointer*) ((NativeLambda*) lambda)->object)->ptr;
	return newBoolean(!MUTEX_TRYLOCK(mutex), th);
}


YOYO_FUNCTION(YSTD_THREADS_NEW_MUTEX) {
	MUTEX* mutex = malloc(sizeof(MUTEX));
	NEW_MUTEX(mutex);
	YValue* ptr = newRawPointer(mutex, YoyoMutex_free, th);
	return ptr;
}
