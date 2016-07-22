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

#include "core.h"

/* Contain procedures to search files and
 * implement InputStream interfaces on files. */

FILE* search_file(wchar_t* wname, wchar_t** wpaths, size_t sz) {
	char* name = malloc(sizeof(wchar_t) / sizeof(char) * (wcslen(wname) + 1));
	wcstombs(name, wname, wcslen(wname));
	name[wcslen(wname)] = '\0';
	FILE* fd = NULL;
	if (access(name, F_OK) != -1) {
		fd = fopen(name, "r");
	} else
		for (size_t i = 0; i < sz; i++) {
			if (fd != NULL)
				break;
			char* path = malloc(
					sizeof(wchar_t) / sizeof(char)
							* (wcslen(wpaths[i]) + strlen(name) + 2));
			wcstombs(path, wpaths[i], wcslen(wpaths[i]));
			path[wcslen(wpaths[i])] = '/';
			path[wcslen(wpaths[i]) + 1] = '\0';
			strcat(path, name);
			if (access(path, F_OK) != -1) {
				fd = fopen(path, "r");
			}
			free(path);
		}
	free(name);
	return fd;
}

typedef struct FileInputStream {
	InputStream is;
	FILE* file;
} FileInputStream;

wint_t FIS_get(InputStream* is) {
	FileInputStream* fis = (FileInputStream*) is;
	wint_t out = fgetwc(fis->file);
	return out;
}
void FIS_unget(InputStream* is, wint_t ch) {
	FileInputStream* fis = (FileInputStream*) is;
	ungetwc(ch, fis->file);
}
void FIS_close(InputStream* is) {
	FileInputStream* fis = (FileInputStream*) is;
	fclose(fis->file);
	free(fis);
}

InputStream* yfileinput(FILE* file) {
	if (file == NULL)
		return NULL;
	FileInputStream* is = malloc(sizeof(FileInputStream));
	is->file = file;
	is->is.get = FIS_get;
	is->is.unget = FIS_unget;
	is->is.close = FIS_close;
	return (InputStream*) is;
}

wchar_t* readLine(FILE* stream) {
	wchar_t* wstr = NULL;
	size_t size = 0;
	wint_t wch;
	while ((wch = getwc(stream)) != L'\n') {
		wstr = realloc(wstr, sizeof(wchar_t) * (++size));
		wstr[size - 1] = (wchar_t) wch;
	}
	wstr = realloc(wstr, sizeof(wchar_t) * (++size));
	wstr[size - 1] = L'\0';
	return wstr;
}

#ifdef OS_WIN

typedef struct ThreadInfo {
	void* (*fun)(void*);
	void* ptr;
}ThreadInfo;

DWORD WINAPI start_win_thread(CONST LPVOID param) {
	ThreadInfo* info = (ThreadInfo*) param;
	info->fun(info->ptr);
	free(info);
	ExitThread(0);
}

void win_new_thread(void* none, void* (*fun)(void*), void* ptr) {
	ThreadInfo* info = malloc(sizeof(ThreadInfo));
	info->fun = fun;
	info->ptr = ptr;
	CreateThread(NULL, 0, start_win_thread, info, 0, NULL);
}
#endif
