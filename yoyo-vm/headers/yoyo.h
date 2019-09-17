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
#ifndef YOYO_H
#define YOYO_H

#define VERSION "0.01-alpha"

#define STR_VALUE(arg)      #arg
#define TOWCS(name) L"" STR_VALUE(name)
// Configuration in compile-time
#ifdef YSTD 
#define YSTD_PATH TOWCS(YSTD)
#else
#define YSTD_PATH L"."
#endif


#ifdef OBJECTS
#define OBJ_TYPE TOWCS(OBJECTS)
#else
#define OBJ_TYPE L"hash"
#endif 

#ifdef __cplusplus
extern "C" {
#endif

#include "yoyo-runtime.h"
#include "interpreter.h"
#include "yoyoc.h"


int32_t Yoyo_interpret_file(ILBytecode*, YRuntime*, wchar_t*);
void Yoyo_main(char** argv, int argc);

#ifdef __cplusplus
}
#endif

#endif // YOYO_H
