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
#ifndef YOYO_H
#define YOYO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "yoyo-runtime.h"
#include "interpreter.h"
#include "yoyoc/yoyoc.h"
#include "yoyoc/node.h"
#include "yoyoc/codegen.h"

bool Yoyo_interpret_file(ILBytecode*, YRuntime*, wchar_t*);
void Yoyo_main(char** argv, int argc);

#ifdef __cplusplus
}
#endif

#endif // YOYO_H
