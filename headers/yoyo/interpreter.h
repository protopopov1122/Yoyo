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

#ifndef YILI_INTERPRETER_H
#define YILI_INTERPRETER_H

#include "../yoyo/yoyo-runtime.h"
#include "yoyo/opcodes.h"
#include "yoyo/bytecode.h"
#include "yoyo/io.h"

/*Procedures used to interpret bytecode.
 * See virtual machine description and 'interpreter.c'.*/

YLambda* newProcedureLambda(int32_t, YObject*, int32_t*, YoyoLambdaSignature*,
		YThread*);
YValue* invoke(int32_t, YObject*, YoyoType*, YThread*);
YValue* execute(YThread*);

void setRegister(YValue*, size_t, YThread*);
#define getRegister(reg, th) ((reg>-1&&reg<th->frame->regc) ? th->frame->regs[reg] : getNull(th))
void push(YValue*, YThread*);
YValue* pop(YThread*);
int64_t popInt(YThread*);

#endif
