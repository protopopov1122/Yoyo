#include "analyze.h"
#include "opcodes.h"

struct opcode_arg_info {
	bool returns_result;
	bool mod_args[3];
};

struct opcode_arg_info get_opcode_arg_info(uint8_t opcode) {
		bool returnsResult = true;
		bool modify_arg0 = false;
		bool modify_arg1 = true;
		bool modify_arg2 = true;

		switch (opcode) {
			case VM_Halt: case VM_Nop:
				returnsResult = false;
				modify_arg1 = false;
				modify_arg2 = false;
			break;
			case VM_LoadConstant: case VM_LoadInteger:
				modify_arg1 = false;
				modify_arg2 = false;
			break;
			case VM_Push: case VM_Return: case VM_Throw:
				returnsResult = false;
				modify_arg0 = true;
				modify_arg1 = false;
				modify_arg2 = false;
			break;
			case VM_Pop: case VM_Catch: case VM_NewInterface: case VM_PushInteger:
				modify_arg1 = false;
				modify_arg2 = false;
			break;
			case VM_Not: case VM_LogicalNot: case VM_Negate: case VM_Test: case VM_FastCompare:
			case VM_NewObject: case VM_GetField: case VM_Copy: case VM_Swap: case VM_Iterator:
			case VM_Iterate: case VM_CheckType:
			case VM_Increment: case VM_Decrement:
				modify_arg2 = false;
			break;
			case VM_SetField: case VM_NewField: case VM_DeleteField:
			case VM_ChangeType:
				returnsResult = false;
				modify_arg0 = true;
				modify_arg1 = false;
			break;
			case VM_ArraySet: case VM_ArrayDelete:
				returnsResult = false;
				modify_arg0 = true;
			break;
			case VM_Jump: case VM_Goto:
			case VM_OpenCatch: case VM_CloseCatch:
				returnsResult = false;
				modify_arg1 = false;
				modify_arg2 = false;
			break;
			case VM_JumpIfTrue: case VM_JumpIfFalse:
			case VM_GotoIfTrue: case VM_GotoIfFalse:
				returnsResult = false;
				modify_arg2 = false;
			break;
			case VM_JumpIfEquals: case VM_JumpIfLesser: case VM_JumpIfGreater:
			case VM_JumpIfNotEquals: case VM_JumpIfNotLesser: case VM_JumpIfNotGreater:
			case VM_GotoIfEquals: case VM_GotoIfLesser: case VM_GotoIfGreater:
			case VM_GotoIfNotEquals: case VM_GotoIfNotLesser: case VM_GotoIfNotGreater:
				returnsResult = false;
			break;
		}
		struct opcode_arg_info out;
		out.returns_result = returnsResult;
		out.mod_args[0] = modify_arg0;
		out.mod_args[1] = modify_arg1;
		out.mod_args[2] = modify_arg2;
		return out;
}

void analyzer_convert_code(ProcedureStats* stats) {
	SSARegister** ssa_map = calloc(stats->proc->regc, sizeof(SSARegister*));

	stats->ssa_regs = realloc(stats->ssa_regs, sizeof(SSARegister) * (++stats->ssa_reg_count));
	SSARegister* ssa_scope_reg = malloc(sizeof(SSARegister));
	ssa_scope_reg->real_reg = 0;
	ssa_scope_reg->id = stats->ssa_reg_count -1;
	ssa_map[0] = ssa_scope_reg;
	ssa_scope_reg->first_use = 0;
	ssa_scope_reg->last_use = 0;
	stats->ssa_regs[stats->ssa_reg_count - 1] = ssa_scope_reg;

	for (size_t pc = 0; pc + 13 <= stats->proc->code_length; pc += 13) {
		uint8_t opcode = stats->proc->code[pc];
		int32_t* direct_args = (int32_t*) &stats->proc->code[pc + 1];
		int32_t args[] = {direct_args[0], direct_args[1], direct_args[2]};

		struct opcode_arg_info arg_info = get_opcode_arg_info(opcode);

		bool returnsResult = arg_info.returns_result;
		bool modify_arg0 = arg_info.mod_args[0];
		bool modify_arg1 = arg_info.mod_args[1];
		bool modify_arg2 = arg_info.mod_args[2];
		

		if (modify_arg1 && args[1] != -1 && args[1] < stats->proc->regc) {
			args[1] = ssa_map[args[1]]->id;
		}
		if (modify_arg2 && args[2] != -1 && args[2] < stats->proc->regc) {
			args[2] = ssa_map[args[2]]->id;
		}
		if (returnsResult) {
			stats->ssa_regs = realloc(stats->ssa_regs, sizeof(SSARegister*) * (++stats->ssa_reg_count));
			SSARegister* ssa_reg = malloc(sizeof(SSARegister));
			ssa_reg->id = stats->ssa_reg_count - 1;
			ssa_reg->real_reg = args[0];
			ssa_reg->first_use = -1;
			ssa_reg->last_use = -1;
			stats->ssa_regs[ssa_reg->id] = ssa_reg;

			ssa_map[args[0]] = ssa_reg;
			args[0] = ssa_reg->id;
		} else if (modify_arg0 && args[0] != -1 && args[0] < stats->proc->regc) {
			args[0] = ssa_map[args[0]]->id;
		}

		stats->code = realloc(stats->code, sizeof(ProcInstr) * (++stats->code_length));
		stats->code[stats->code_length - 1].opcode = opcode;
		stats->code[stats->code_length - 1].args[0] = args[0];
		stats->code[stats->code_length - 1].args[1] = args[1];
		stats->code[stats->code_length - 1].args[2] = args[2];
	}
}

void analyze_register_area(ProcedureStats* stats) {
	for (size_t i = 0; i < stats->code_length; i++) {
		ProcInstr* instr = &stats->code[i];
		struct opcode_arg_info arg_info = get_opcode_arg_info(instr->opcode);

		if (arg_info.mod_args[1] && instr->args[1] != -1) {
			stats->ssa_regs[instr->args[1]]->last_use = i;
		}

		if (arg_info.mod_args[2] && instr->args[2] != -1) {
			stats->ssa_regs[instr->args[2]]->last_use = i;
		}

		if (arg_info.returns_result) {
			stats->ssa_regs[instr->args[0]]->first_use = i;
		}

		if (arg_info.mod_args[0] && instr->args[0] != -1) {
			stats->ssa_regs[instr->args[0]]->last_use = i;
		}
	}

	uint8_t* real_regs = NULL;
	size_t real_regs_size = 0;
	size_t real_regc = 0;
	for (size_t pc = 0; pc < stats->code_length; pc++) {
		for (size_t i = 0; i < stats->ssa_reg_count; i++) {
			if (stats->ssa_regs[i]->first_use == pc) {
				if (real_regs_size * 8 == real_regc) {
					real_regs = realloc(real_regs, ++real_regs_size * sizeof(uint8_t));
				}
				stats->ssa_regs[i]->real_reg = -1;
				if (stats->ssa_regs[i]->last_use == -1)
					continue;
				for (size_t j = 0; j < real_regs_size; j++) {
					if (stats->ssa_regs[i]->real_reg != -1)
						break;
					for (size_t k = 0; k < 8; k++) {
						uint8_t bit = real_regs[j] & (1 << k);
						if (bit == 0) {
							stats->ssa_regs[i]->real_reg = j * 8 + k;
							real_regs[j] |= 1 << k;
							real_regc++;
							break;
						}
					}
				}
			}
			else if (stats->ssa_regs[i]->last_use == pc) {
				real_regs[stats->ssa_regs[i]->real_reg / 8] =
					real_regs[stats->ssa_regs[i]->real_reg / 8] &
						~(1 << stats->ssa_regs[i]->real_reg % 8);
			}
		}
	}
	free(real_regs);
}

void analyzer_convert_block(ProcedureStats* stats) {
	stats->blocks = calloc(++stats->block_count, sizeof(InstrBlock));
	InstrBlock* current = &stats->blocks[stats->block_count - 1];
	current->block = &stats->code[0];
	for (size_t i = 0; i < stats->code_length; i++) {
		size_t pc = i * 13;
		for (size_t j = 0; j < stats->proc->labels.length; j++)
			if (stats->proc->labels.table[j].value == pc) {
				if (current->block_length > 0) {
					stats->blocks = realloc(stats->blocks, ++stats->block_count * sizeof(InstrBlock));
					current = &stats->blocks[stats->block_count - 1];
					current->block = &stats->code[i];
					current->block_length = 0;
				}
				break;
			}
		current->block_length++;
		uint8_t opcode = stats->code[i].opcode;
		if ((opcode == VM_Jump || opcode == VM_Goto ||
			opcode == VM_JumpIfTrue || opcode == VM_GotoIfTrue ||
			opcode == VM_JumpIfFalse || opcode == VM_GotoIfFalse ||
			opcode == VM_JumpIfEquals || opcode == VM_GotoIfEquals ||
			opcode == VM_JumpIfNotEquals || opcode == VM_GotoIfNotEquals ||
			opcode == VM_JumpIfGreater || opcode == VM_GotoIfGreater ||
			opcode == VM_JumpIfNotGreater || opcode == VM_GotoIfNotGreater ||
			opcode == VM_JumpIfLesser || opcode == VM_GotoIfLesser ||
			opcode == VM_JumpIfNotLesser || opcode == VM_GotoIfNotLesser ||
			opcode == VM_Iterate || opcode == VM_Return || opcode == VM_Throw)&&
			i + 1 < stats->code_length) {
					stats->blocks = realloc(stats->blocks, ++stats->block_count * sizeof(InstrBlock));
					current = &stats->blocks[stats->block_count - 1];
					current->block = &stats->code[i+1];
					current->block_length = 0;

		}
	}
}

void analyzer_raw_call_transform(ProcedureStats* stats) {
	int32_t* push_regs = NULL;
	size_t push_regc = 0;

	ProcInstr* new_code = NULL;
	size_t new_code_len = 0;
	for (size_t i = 0; i < stats->code_length; i++) {
		ProcInstr* instr = &stats->code[i];

		if (instr->opcode == VM_Push) {
			push_regs = realloc(push_regs, ++push_regc * sizeof(int32_t));
			push_regs[push_regc - 1] = instr->args[0];
			continue;
		}

		if (instr->opcode == VM_Call ||
			instr->opcode == VM_NewOverload ||
			instr->opcode == VM_NewComplexObject) {
			for (size_t j = 0; j < push_regc; j++) {
				new_code = realloc(new_code, ++new_code_len * sizeof(ProcInstr));
				ProcInstr* ninstr = &new_code[new_code_len - 1];
				ninstr->opcode = VM_Push;
				ninstr->args[0] = push_regs[j];
				ninstr->args[1] = -1;
				ninstr->args[2] = -1;
			}
			free(push_regs);
			push_regc = 0;
			push_regs = NULL;
		}
		
		new_code = realloc(new_code, ++new_code_len * sizeof(ProcInstr));
		ProcInstr* ninstr = &new_code[new_code_len - 1];
		ninstr->opcode = instr->opcode;
		ninstr->args[0] = instr->args[0];
		ninstr->args[1] = instr->args[1];
		ninstr->args[2] = instr->args[2];
	}

	free(stats->code);
	stats->code = new_code;
	stats->code_length = new_code_len;
}

ProcedureStats* analyze(ILProcedure* proc, ILBytecode* bc) {
	ProcedureStats* stats = calloc(1, sizeof(ProcedureStats));
	stats->proc = proc;
	analyzer_convert_code(stats);
	analyzer_raw_call_transform(stats);
	analyze_register_area(stats);
	analyzer_convert_block(stats);

	printf("Procedure %"PRId32":\n", proc->id);
	size_t pc = 0;
	for (size_t i = 0; i < stats->block_count; i++) {
		printf("\tblock%zu:\n", i);
		InstrBlock* block = &stats->blocks[i];
		for (size_t j = 0; j < block->block_length; j++) {
			wchar_t* mnem = NULL;
			for (size_t k = 0; k < OPCODE_COUNT; k++)
				if (Mnemonics[k].opcode == block->block[j].opcode) {
					mnem = Mnemonics[k].mnemonic;
					break;
				}
			if (mnem != NULL)
				printf("\t%zu:\t%ls %"PRId32", %"PRId32", %"PRId32"\n", pc, mnem, block->block[j].args[0],
					block->block[j].args[1], block->block[j].args[2]);
			pc++;
		}
	}
	printf("\n\tRegisters:\n");
	for (size_t i = 0; i < stats->ssa_reg_count; i++) {
		SSARegister* reg = stats->ssa_regs[i];
		printf("\t\t%zu used from %zi to %zi mapped to %zi\n", reg->id, reg->first_use, reg->last_use, reg->real_reg);
	}

	return stats;
}
