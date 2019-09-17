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

#include "yoyo-runtime.h"
#include "interpreter.h"
#include "opcodes.h"

/*This file contains procedures to interpret bytecode.*/

/*Some useful procedures for interpreter*/
/*Assign certain register value. If value is NULL then
 * assigned is Null.*/

void setRegister(YValue* v, size_t reg, YThread* th) {
	ExecutionFrame* frame = (ExecutionFrame*) ((ExecutionFrame*) th->frame);
	if (reg < frame->regc)
		frame->regs[reg] = v != NULL ? v : getNull(th);
}
/*Push value to frames stack. If stack is full it's being reallocated.*/
void push(YValue* v, YThread* th) {
	ExecutionFrame* frame = ((ExecutionFrame*) th->frame);
	if (frame->stack_offset + 1 >= frame->stack_size) {
		frame->stack_size += 10;
		frame->stack = realloc(frame->stack,
				sizeof(YValue*) * (frame->stack_size));
	}
	frame->stack[frame->stack_offset++] = v;
}
/*Return value popped from stack or Null if stack is empty.*/
YValue* pop(YThread* th) {
	ExecutionFrame* frame = ((ExecutionFrame*) th->frame);
	if (frame->stack_offset != 0)
		return frame->stack[--frame->stack_offset];
	else
		return getNull(th);
}
/*Pop value from stack. If it's integer return it, else return 0*/
int64_t popInt(YThread* th) {
	YValue* v = pop(th);
	if (v->type == &th->runtime->IntType)
		return ((YInteger*) v)->value;
	else
		return 0;
}

void ExecutionFrame_mark(LocalFrame* f) {
	ExecutionFrame* frame = (ExecutionFrame*) f;
	for (size_t i = 0; i < frame->regc; i++)
		MARK(frame->regs[i]);
	MARK(frame->retType);
	for (size_t i = 0; i < frame->stack_offset; i++) {
		MARK(frame->stack[i]);
	}
}

SourceIdentifier ExecutionFrame_get_source_id(LocalFrame* f) {
	ExecutionFrame* frame = (ExecutionFrame*) f;
	CodeTableEntry* ent = frame->proc->getCodeTableEntry(frame->proc,
			frame->pc);
	if (ent == NULL) {
		SourceIdentifier sid = { .file = -1 };
		return sid;
	}
	SourceIdentifier sid = { .file = ent->file, .line = ent->line,
			.charPosition = ent->charPos };
	return sid;
}
#define EXECUTE_PROC execute
#include "vm_execute.c"
#undef EXECUTE_PROC
#define EXECUTE_PROC execute_stats
#define COLLECT_STATS
#include "vm_execute.c"

/*Initialize execution frame, assign it to thread and call execute method on it*/
YValue* invoke(int32_t procid, ILBytecode* bytecode, YObject* scope,
		YoyoType* retType, YThread* th) {
	ILProcedure* proc = bytecode->procedures[procid];
	if (proc->compiled!=NULL) {
		proc->compiled->call(scope, th);
		return getNull(th);
	}

	ExecutionFrame frame;

	// Init execution frame
	frame.frame.mark = ExecutionFrame_mark;
	frame.frame.get_source_id = ExecutionFrame_get_source_id;
	frame.bytecode = bytecode;
	frame.proc = proc;
	frame.retType = retType;
	frame.regc = proc->regc;
	frame.regs = malloc(sizeof(YValue*) * frame.regc);
	for (size_t i = 1; i < frame.regc; i++)
		frame.regs[i] = getNull(th);
	frame.pc = 0;
	frame.stack_size = 10;
	frame.stack_offset = 0;
	frame.stack = malloc(sizeof(YValue*) * frame.stack_size);
	frame.catchBlock = NULL;
	frame.debug_ptr = NULL;
	frame.debug_field = -1;
	frame.debug_flags = 0;
	YBreakpoint bp;
	frame.breakpoint = &bp;

	// Assign execution frame to thread
	MUTEX_LOCK(&th->mutex);
	frame.frame.prev = th->frame;
	th->frame = (LocalFrame*) &frame;
	MUTEX_UNLOCK(&th->mutex);
	// Call debugger if nescesarry
	if (frame.frame.prev == NULL)
		DEBUG(th->runtime->debugger, interpret_start, &procid, th);
	DEBUG(th->runtime->debugger, enter_function, frame.frame.prev, th);

	YValue* out;
	if (frame.proc->stats == NULL)
		out = execute(scope, th);
	else
		out = execute_stats(scope, th);	// Finally execute
	frame.pc = 0;	// For debug

	// Call debugger if nescesarry
	if (frame.frame.prev == NULL)
		DEBUG(th->runtime->debugger, interpret_end, &procid, th);

	// Free resources and remove frame from stack
	MUTEX_LOCK(&th->mutex);
	th->frame = frame.frame.prev;
	free(frame.regs);
	free(frame.stack);
	MUTEX_UNLOCK(&th->mutex);


		return out;
}

