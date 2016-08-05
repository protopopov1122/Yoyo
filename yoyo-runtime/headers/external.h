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

#ifndef YILI_THREADS_H
#define YILI_THREADS_H

#include <unistd.h>
#include <dlfcn.h>

#if defined(_WIN32)

#ifdef WINVER
#undef WINVER
#endif
#define WINVER 0x0600
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600

#define interface __INTERFACE
#include <windows.h>
#undef interface

#define PLATFORM L"windows"
#define SIZE_T "%Iu"
#define WCSTOK(str, del, st) wcstok(str, del)
#define WCSTOK_STATE(state)
#define OS_WIN

#define YIELD() Sleep(0)
typedef HANDLE THREAD;
void win_new_thread(void*, void* (*)(void*), void*);
#define NEW_THREAD(th, proc, value) win_new_thread(th, proc, value)
#define THREAD_EXIT(ptr) return NULL;
#define THREAD_SELF() GetCurrentThread()
#define THREAD_EQUAL(t1, t2) (GetThreadId(t1)==GetThreadId(t2))
typedef CRITICAL_SECTION MUTEX;
#define NEW_MUTEX(mutex) InitializeCriticalSectionAndSpinCount(mutex, 0x400)
#define DESTROY_MUTEX(mutex)
#define MUTEX_LOCK(mutex) EnterCriticalSection(mutex)
#define MUTEX_UNLOCK(mutex) LeaveCriticalSection(mutex)
#define MUTEX_TRYLOCK(mutex) TryEnterCriticalSection(mutex)
typedef CONDITION_VARIABLE COND;
#define NEW_COND(cond) InitializeConditionVariable(cond)
#define DESTROY_COND(cond)
#define COND_SIGNAL(cond) WakeConditionVariable(cond)
#define COND_BROADCAST(cond) WakeAllConditionVariable(cond)
#define COND_WAIT(cond, mutex) SleepConditionVariableCS(cond, mutex, INFINITE)

#elif defined(__linux__) || defined(__unix__)
#include <pthread.h>

#ifdef __linux__
#define PLATFORM L"linux"
#define OS_LINUX
#else
#define PLATFORM L"unix"
#define OS_UNIX
#endif

#define SIZE_T "%zu"
#define WCSTOK(str, del, st) wcstok(str, del, st)
#define WCSTOK_STATE(state) wchar_t* state = NULL;

#define YIELD() sched_yield()
typedef pthread_t THREAD;
#define NEW_THREAD(th, proc, value) pthread_create(th, NULL, proc, value)
#define THREAD_EXIT(ptr) pthread_exit(ptr)
#define THREAD_SELF() pthread_self()
#define THREAD_EQUAL(t1, t2) pthread_equal(t1, t2)
typedef pthread_mutex_t MUTEX;
#define NEW_MUTEX(mutex) pthread_mutex_init(mutex, NULL)
#define DESTROY_MUTEX(mutex) pthread_mutex_destroy(mutex)
#define MUTEX_LOCK(mutex) pthread_mutex_lock(mutex)
#define MUTEX_UNLOCK(mutex) pthread_mutex_unlock(mutex)
#define MUTEX_TRYLOCK(mutex) pthread_mutex_trylock(mutex)
typedef pthread_cond_t COND;
#define NEW_COND(cond) pthread_cond_init(cond, NULL)
#define DESTROY_COND(cond) pthread_cond_destroy(cond)
#define COND_SIGNAL(cond) pthread_cond_signal(cond)
#define COND_BROADCAST(cond) pthread_cond_broadcast(cond)
#define COND_WAIT(cond, mutex) pthread_cond_wait(cond, mutex)

#else

#define OS_UNKNOWN
#define PLATFORM L"unknown"

#endif

wchar_t* readLine(FILE* stream);

#endif
