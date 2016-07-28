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
	if (cs->stream[0]!=NULL) for (size_t i=0;i<cs->labels.length;i++) {
		if (cs->labels.table[i].value == cs->stream[0]->old_position) {
			cs->proc->labels.table[i].value = cs->proc->code_length;
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
	memcpy(cs->labels.table, proc->labels.table, sizeof(LabelEntry) * proc->labels.length);
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

void optimize_procedure(ILProcedure* proc) {
	CommandStream* cs = newCommandStream(proc);
	while(cs->stream[0]!=NULL) {
		/*if (cs->stream[0]->opcode==VM_LoadInteger&&
			cs->stream[1]->opcode==VM_Push) {
			VMCommand cmd;
			cmd.old_position = -1;
			cmd.opcode = VM_PushInteger;
			cmd.args[0] = cs->stream[0]->args[1];
			cmd.args[1] = -1;
			cmd.args[2] = -1;
			cs->shift(cs);
			cs->shift(cs);
			cs->append(cs, &cmd);
		} else {*/
			if (cs->stream[0]->opcode!=VM_Nop)
				cs->append(cs, cs->stream[0]);
			cs->shift(cs);
		//}
	}
	cs->close(cs);
}
