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
 
#include "yoyoc.h"
#include "codegen.h"

typedef struct VMCommand {
	uint8_t opcode;
	int32_t args[3];
	ssize_t old_position;
} VMCommand;

typedef struct CommandStream {
	uint8_t* input;
	size_t length;
	size_t index;
	LabelTable labels;
	CodeTable codeTable;
	ILProcedure* proc;

	VMCommand* stream[4];
 	VMCommand* (*shift)(struct CommandStream*);
	void (*append)(struct CommandStream*, VMCommand*);
	void (*close)(struct CommandStream*);
} CommandStream;

void CommandStream_close(CommandStream* cs) {
	for (size_t i=0;i<4;i++)
		if (cs->stream[i]!=NULL)
			free(cs->stream[i]);
	free(cs->input);
	free(cs->labels.table);
	free(cs->codeTable.table);
	free(cs);
}
VMCommand* CommandStream_shift(CommandStream* cs) {
	if (cs->stream[0]!=NULL) {
		free(cs->stream[0]);
	}
	cs->stream[0] = cs->stream[1];
	cs->stream[1] = cs->stream[2];
	cs->stream[2] = cs->stream[3];
	if (cs->index+13<=cs->length) {
		VMCommand* cmd = malloc(sizeof(VMCommand));
		cmd->old_position = -1;
		for (size_t i=0;i<cs->labels.length;i++) {
			if (cs->labels.table[i].value == cs->index) {
					cmd->old_position = cs->index;
			}
		}
		cmd->old_position = cs->index;
		cmd->opcode = cs->input[cs->index++];
		int32_t* args = (int32_t*) &cs->input[cs->index];
		cs->index += 12;
		cmd->args[0] = args[0];
		cmd->args[1] = args[1];
		cmd->args[2] = args[2];
		cs->stream[3] = cmd;
	} else {
		cs->stream[3] = NULL;
	}
	if (cs->stream[0]!=NULL) {
		for (size_t i=0;i<cs->labels.length;i++) {
			if (cs->labels.table[i].value == cs->stream[0]->old_position) {
				cs->proc->labels.table[i].value = cs->proc->code_length;
			}
		}
		for (size_t i=0;i<cs->codeTable.length;i++) {
			if (cs->codeTable.table[i].offset == cs->stream[0]->old_position) {
				cs->proc->codeTable.table[i].offset = cs->proc->code_length;
			}
			if (cs->codeTable.table[i].end == cs->stream[0]->old_position) {
				cs->proc->codeTable.table[i].end = cs->proc->code_length;
			}
		}
	}

		return cs->stream[0];
}
void CommandStream_append(CommandStream* cs, VMCommand* cmd) {
	cs->proc->appendCode(cs->proc, &cmd->opcode, 1);
	union {
		int32_t args[3];
		uint8_t raw[12];
	} un;
	un.args[0] = cmd->args[0];
	un.args[1] = cmd->args[1];
	un.args[2] = cmd->args[2];
	cs->proc->appendCode(cs->proc, un.raw, 12);
}

CommandStream* newCommandStream(ILProcedure* proc) {
	CommandStream* cs = malloc(sizeof(CommandStream));
	cs->proc = proc;
	cs->index = 0;
	cs->length = proc->code_length;
	cs->input = malloc(sizeof(uint8_t) * proc->code_length);
	memcpy(cs->input, proc->code, sizeof(uint8_t) * proc->code_length);
	free(proc->code);
	cs->labels.length = proc->labels.length;
	cs->labels.table = malloc(sizeof(LabelEntry) * proc->labels.length);
	cs->codeTable.length = proc->codeTable.length;
	cs->codeTable.table = malloc(sizeof(CodeTableEntry) * proc->codeTable.length);
	memcpy(cs->labels.table, proc->labels.table, sizeof(LabelEntry) * proc->labels.length);
	memcpy(cs->codeTable.table, proc->codeTable.table, sizeof(CodeTableEntry) * proc->codeTable.length);
	proc->code = NULL;
	proc->code_length = 0;
	cs->stream[0] = NULL;
	cs->stream[1] = NULL;
	cs->stream[2] = NULL;
	cs->stream[3] = NULL;
	cs->shift = CommandStream_shift;
	cs->close = CommandStream_close;
	cs->append = CommandStream_append;
	cs->shift(cs);
	cs->shift(cs);
	cs->shift(cs);
	cs->shift(cs);
	return cs;
}

void init_command(VMCommand* cmd, uint8_t opcode) {
	cmd->old_position = -1;
	cmd->opcode = opcode;
	cmd->args[0] = -1;
	cmd->args[1] = -1;
	cmd->args[2] = -1;
}

void raw_optimize_procedure(ILProcedure* proc) {
	CommandStream* cs = newCommandStream(proc);
	while(cs->stream[0]!=NULL) {
		if (cs->stream[1]!=NULL) {
			if (cs->stream[2]!=NULL) {
				if (cs->stream[0]->opcode==VM_Copy &&
					cs->stream[1]->opcode==VM_Copy) {
					if (cs->stream[2]->args[1] == cs->stream[0]->args[0] &&
						cs->stream[2]->args[2] == cs->stream[1]->args[0] &&
						(cs->stream[2]->opcode == VM_Add ||
						cs->stream[2]->opcode == VM_Subtract ||
						cs->stream[2]->opcode == VM_Multiply ||
						cs->stream[2]->opcode == VM_Divide ||
						cs->stream[2]->opcode == VM_Modulo ||
						cs->stream[2]->opcode == VM_And ||
						cs->stream[2]->opcode == VM_Or ||
						cs->stream[2]->opcode == VM_Xor ||
						cs->stream[2]->opcode == VM_Compare ||
						cs->stream[2]->opcode == VM_ShiftLeft ||
						cs->stream[2]->opcode == VM_ShiftRight)) {
						VMCommand cmd;
						init_command(&cmd, cs->stream[2]->opcode);
						cmd.args[0] = cs->stream[2]->args[0];
						cmd.args[1] = cs->stream[0]->args[1];
						cmd.args[2] = cs->stream[1]->args[1];
						cs->append(cs, &cmd);
						cs->shift(cs);
						cs->shift(cs);
						cs->shift(cs);
					}
				}
			}
			if (cs->stream[0]->opcode == VM_Swap &&
				cs->stream[1]->opcode == VM_Swap &&
				((cs->stream[0]->args[0] == cs->stream[1]->args[1] &&
				cs->stream[0]->args[1] == cs->stream[1]->args[0]) ||
				(cs->stream[0]->args[0] == cs->stream[1]->args[0] &&
				cs->stream[0]->args[1] == cs->stream[1]->args[1]))) {
				cs->shift(cs);
				cs->shift(cs);
			}
			if (cs->stream[0]->opcode == VM_Copy &&
				cs->stream[1]->opcode == VM_Push &&
				cs->stream[0]->args[0] == cs->stream[1]->args[0]) {
				VMCommand cmd;
				init_command(&cmd, VM_Push);
				cmd.args[0] = cs->stream[0]->args[1];
				cs->append(cs, &cmd);
				cs->shift(cs);
				cs->shift(cs);
			}
			if (cs->stream[0]->opcode == VM_Copy &&
				cs->stream[1]->opcode == VM_Copy &&
				cs->stream[0]->args[0] == cs->stream[1]->args[1] &&
				cs->stream[0]->args[1] == cs->stream[1]->args[0]) {
				cs->append(cs, cs->stream[0]);
				cs->shift(cs);
				cs->shift(cs);
			}
			if (cs->stream[0]->opcode == VM_Copy &&
				cs->stream[1]->opcode == VM_Copy &&
				cs->stream[0]->args[0] == cs->stream[1]->args[0] &&
				cs->stream[0]->args[1] == cs->stream[1]->args[1]) {
				cs->append(cs, cs->stream[0]);
				cs->shift(cs);
				cs->shift(cs);
			}
			if (cs->stream[0]->opcode==VM_FastCompare&&
				cs->stream[1]->opcode==VM_GotoIfFalse&&
				cs->stream[0]->args[0]==cs->stream[1]->args[1]) {
				VMCommand cmd;
				cmd.old_position = -1;
				cmd.args[0] = cs->stream[1]->args[0];
				cmd.args[1] = cs->stream[0]->args[0];
				cmd.args[2] = cs->stream[0]->args[1];
				if (cs->stream[0]->args[2]==COMPARE_EQUALS) {
					cmd.opcode = VM_GotoIfNotEquals;
					cs->append(cs, &cmd);
					cs->shift(cs);
					cs->shift(cs);
					continue;
				}
				if (cs->stream[0]->args[2]==COMPARE_NOT_EQUALS) {
					cmd.opcode = VM_GotoIfEquals;
					cs->append(cs, &cmd);
					cs->shift(cs);
					cs->shift(cs);
					continue;
				}
				if (cs->stream[0]->args[2]==COMPARE_LESSER_OR_EQUALS) {
					cmd.opcode = VM_GotoIfGreater;
					cs->append(cs, &cmd);
					cs->shift(cs);
					cs->shift(cs);
					continue;
				}
				if (cs->stream[0]->args[2]==COMPARE_GREATER_OR_EQUALS) {
					cmd.opcode = VM_GotoIfLesser;
					cs->append(cs, &cmd);
					cs->shift(cs);
					cs->shift(cs);
					continue;
				}
				if (cs->stream[0]->args[2]==COMPARE_LESSER) {
					cmd.opcode = VM_GotoIfNotLesser;
					cs->append(cs, &cmd);
					cs->shift(cs);
					cs->shift(cs);
					continue;
				}
				if (cs->stream[0]->args[2]==COMPARE_GREATER) {
					cmd.opcode = VM_GotoIfNotGreater;
					cs->append(cs, &cmd);
					cs->shift(cs);
					cs->shift(cs);
					continue;
				}
			}
		}
		if (cs->stream[0]->opcode!=VM_Nop)
			cs->append(cs, cs->stream[0]);
		cs->shift(cs);
	}
	cs->close(cs);
}

void optimize_procedure(ILProcedure* proc) {
	raw_optimize_procedure(proc);
	raw_optimize_procedure(proc);
}
