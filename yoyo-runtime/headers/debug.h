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

#ifndef YOYO_RUNTIME_DEBUG_H
#define YOYO_RUNTIME_DEBUG_H

#include "yoyo-runtime.h"

typedef struct YDebug YDebug;
typedef void (*DebugProc)(struct YDebug*, void*, YThread*);

typedef struct YBreakpoint {
	int32_t procid;
	size_t pc;
} YBreakpoint;

typedef struct YDebug {
	enum {
		NoDebug, Debug
	} mode;
	DebugProc onload;
	DebugProc interpret_start;
	DebugProc instruction;
	DebugProc enter_function;
	DebugProc error;
	DebugProc interpret_end;
	DebugProc onunload;
	void (*free)(struct YDebug*);
} YDebug;

#define DEBUG(debug, proc, ptr, th) if (debug!=NULL&&debug->mode==Debug) {\
                                        th->runtime->state = RuntimePaused;\
                                        if (debug->proc!=NULL)\
                                            debug->proc(debug, ptr, th);\
                                        th->runtime->state = RuntimeRunning;\
                                    }

#endif // YOYO_RUNTIME_DEBUG_H
