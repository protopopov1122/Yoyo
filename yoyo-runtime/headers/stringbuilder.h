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

#ifndef YOYO_RUNTIME_STRING_BUILDER_H
#define YOYO_RUNTIME_STRING_BUILDER_H

#include "core.h"

typedef struct StringBuilder {
	wchar_t* string;

	void (*append)(struct StringBuilder*, wchar_t*);
	void (*free)(struct StringBuilder*);
} StringBuilder;

StringBuilder* newStringBuilder(wchar_t*);

#endif
