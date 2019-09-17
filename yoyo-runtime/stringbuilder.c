/*
 * Copyright (C) 2016  Jevgenijs Protopopovs <protopopov1122@yandex.ru>
 */
/*This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 3 as published by
 the Free Software Foundation.


 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "stringbuilder.h"

/*String builder used concat wide-char strings.
 * It accpets initial string, that is copied to memory.
 * Then append method accepts other strings
 * to concat it to resulting strings.
 * free method frees both string and StringBuilder,
 * so if resulting string was not copied and is
 * nescesarry StringBuilder should be freed
 * simply like this: free(...)
 * */

void StringBuilder_free(StringBuilder* sb) {
	free(sb->string);
	free(sb);
}

void StringBuilder_append(StringBuilder* sb, wchar_t* str) {
	size_t len = wcslen(sb->string) + wcslen(str);
	sb->string = realloc(sb->string, sizeof(wchar_t) * (len + 1));
	wcscat(sb->string, str);
	sb->string[len] = L'\0';
}

StringBuilder* newStringBuilder(wchar_t* init) {
	StringBuilder* sb = malloc(sizeof(StringBuilder));
	sb->string = malloc(sizeof(wchar_t) * (wcslen(init) + 1));
	wcscpy(sb->string, init);
	sb->string[wcslen(init)] = L'\0';
	sb->free = StringBuilder_free;
	sb->append = StringBuilder_append;
	return sb;
}
