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

wchar_t* readLine(FILE* stream) {
	wchar_t* wstr = NULL;
	size_t size = 0;
	wint_t wch;
	while ((wch = fgetwc(stream)) != L'\n') {
		wstr = realloc(wstr, sizeof(wchar_t) * (++size));
		wstr[size - 1] = (wchar_t) wch;
	}
	wstr = realloc(wstr, sizeof(wchar_t) * (++size));
	wstr[size - 1] = L'\0';
	return wstr;
}

