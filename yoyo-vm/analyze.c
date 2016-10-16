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
			case VM_NewOverload: case VM_NewLambda:
				modify_arg1 = false;
			break;
			case VM_FastCompare:
				returnsResult = false;
				modify_arg0 = true;
				modify_arg2 = false;
			break;
			case VM_Not: case VM_LogicalNot: case VM_Negate: case VM_Test:
			case VM_NewObject: case VM_GetField: case VM_Copy: case VM_Swap: case VM_Iterator:
			case VM_Iterate: case VM_CheckType: case VM_NewComplexObject:
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

void traslate_opcode(uint8_t opcode, int32_t* args, size_t pc, ProcedureStats* stats, SSARegister** ssa_map) {
		struct opcode_arg_info arg_info = get_opcode_arg_info(opcode);

		bool returnsResult = arg_info.returns_result;
		bool modify_arg0 = arg_info.mod_args[0];
		bool modify_arg1 = arg_info.mod_args[1];
		bool modify_arg2 = arg_info.mod_args[2];
		
		SSARegister* ssa_reg = NULL;
		if (modify_arg1 && args[1] != -1 && args[1] < stats->proc->regc) {
			args[1] = ssa_map[args[1]]->id;
		}
		if (modify_arg2 && args[2] != -1 && args[2] < stats->proc->regc) {
			args[2] = ssa_map[args[2]]->id;
		}
		if (returnsResult) {
			stats->ssa_regs = realloc(stats->ssa_regs, sizeof(SSARegister*) * (++stats->ssa_reg_count));
			ssa_reg = calloc(1, sizeof(SSARegister));
			ssa_reg->id = stats->ssa_reg_count - 1;
			ssa_reg->real_reg = args[0];
			ssa_reg->first_use = -1;
			ssa_reg->last_use = -1;
			ssa_reg->first_read = -1;
			ssa_reg->use_count = 0;
			ssa_reg->cmd = NULL;
			ssa_reg->dead = false;
			ssa_reg->type = DynamicRegister;
			stats->ssa_regs[ssa_reg->id] = ssa_reg;

			ssa_map[args[0]] = ssa_reg;
			args[0] = ssa_reg->id;
		} else if (modify_arg0 && args[0] != -1 && args[0] < stats->proc->regc) {
			args[0] = ssa_map[args[0]]->id;
		}

		stats->code = realloc(stats->code, sizeof(ProcInstr*) * (++stats->code_length));
		ProcInstr* instr = calloc(1, sizeof(ProcInstr));
		stats->code[stats->code_length - 1] = instr;
		instr->opcode = opcode;
		instr->args[0] = args[0];
		instr->args[1] = args[1];
		instr->args[2] = args[2];
		instr->real_offset = pc;
		instr->dead = false;
		instr->affects = returnsResult ? stats->ssa_regs[args[0]] : NULL;
		instr->max_delay = -1;

}

void analyzer_convert_code(ProcedureStats* stats) {
	SSARegister** ssa_map = calloc(stats->proc->regc, sizeof(SSARegister*));

	stats->ssa_regs = realloc(stats->ssa_regs, sizeof(SSARegister) * (++stats->ssa_reg_count));
	SSARegister* ssa_scope_reg = calloc(1, sizeof(SSARegister));
	ssa_scope_reg->real_reg = 0;
	ssa_scope_reg->id = stats->ssa_reg_count -1;
	ssa_map[0] = ssa_scope_reg;
	ssa_scope_reg->first_use = 0;
	ssa_scope_reg->last_use = 0;
	ssa_scope_reg->first_read = -1;
	ssa_scope_reg->cmd = NULL;
	ssa_scope_reg->use_count = 0;
	ssa_scope_reg->dead = false;
	ssa_scope_reg->type = DynamicRegister;
	ssa_scope_reg->runtime.type[ObjectRT]++;
	stats->ssa_regs[stats->ssa_reg_count - 1] = ssa_scope_reg;

	for (size_t pc = 0; pc + 13 <= stats->proc->code_length; pc += 13) {
		uint8_t opcode = stats->proc->code[pc];
		int32_t* direct_args = (int32_t*) &stats->proc->code[pc + 1];
		int32_t args[] = {direct_args[0], direct_args[1], direct_args[2]};
		
		if (opcode == VM_FastCompare) {
			int32_t cmp_args[] = {args[0], args[0], args[1]};
			int32_t test_args[] = {args[0], args[0], args[2]};
			traslate_opcode(VM_Compare, cmp_args, pc, stats, ssa_map);
			traslate_opcode(VM_Test, test_args, pc, stats, ssa_map);
			continue;
		}
		traslate_opcode(opcode, args, pc, stats, ssa_map);

	}
}

void analyze_register_area(ProcedureStats* stats) {
	for (size_t i = 0; i < stats->code_length; i++) {
		ProcInstr* instr = stats->code[i];
		if (instr->opcode == VM_Copy && instr->args[1] > -1) {
			stats->ssa_regs[instr->args[0]]->link = stats->ssa_regs[instr->args[1]];
		}
		struct opcode_arg_info arg_info = get_opcode_arg_info(instr->opcode);

		if (arg_info.mod_args[1] && instr->args[1] != -1) {
			if (stats->ssa_regs[instr->args[1]]->link != NULL) {
				instr->args[1] = stats->ssa_regs[instr->args[1]]->link->id;
			}
			stats->ssa_regs[instr->args[1]]->last_use = i;
			if (stats->ssa_regs[instr->args[1]]->first_read == -1)
				stats->ssa_regs[instr->args[1]]->first_read = i;
			stats->ssa_regs[instr->args[1]]->use_count++;
		}

		if (arg_info.mod_args[2] && instr->args[2] != -1) {
			if (stats->ssa_regs[instr->args[2]]->link != NULL) {
				instr->args[2] = stats->ssa_regs[instr->args[2]]->link->id;
			}
			stats->ssa_regs[instr->args[2]]->last_use = i;
			if (stats->ssa_regs[instr->args[2]]->first_read == -1)
				stats->ssa_regs[instr->args[2]]->first_read = i;
			stats->ssa_regs[instr->args[2]]->use_count++;
		}

		if (arg_info.returns_result) {
			stats->ssa_regs[instr->args[0]]->first_use = i;
		}

		if (arg_info.mod_args[0] && instr->args[0] != -1) {
			if (stats->ssa_regs[instr->args[0]]->link != NULL) {
				instr->args[0] = stats->ssa_regs[instr->args[0]]->link->id;
			}
			stats->ssa_regs[instr->args[0]]->last_use = i;
			if (stats->ssa_regs[instr->args[0]]->first_read == -1)
				stats->ssa_regs[instr->args[0]]->first_read = i;
			stats->ssa_regs[instr->args[0]]->use_count++;
		}
	}

	uint8_t* real_regs = malloc(sizeof(uint8_t));
	real_regs[0] = 1;
	size_t real_regs_size = 1;
	size_t real_regc = 1;
	for (size_t pc = 0; pc < stats->code_length; pc++) {
		for (size_t i = 1; i < stats->ssa_reg_count; i++) {
			if (stats->ssa_regs[i]->first_use == pc) {
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
							break;
						}
					}
				}
				if (stats->ssa_regs[i]->real_reg == -1) {
					real_regs = realloc(real_regs, ++real_regs_size * sizeof(uint8_t));
					real_regs[real_regs_size - 1] = 1;
					stats->ssa_regs[i]->real_reg = (real_regs_size - 1) * 8;
				}
				if (stats->ssa_regs[i]->real_reg >= real_regc)
					real_regc = stats->ssa_regs[i]->real_reg + 1;
			}
			else if (stats->ssa_regs[i]->last_use == pc) {
				real_regs[stats->ssa_regs[i]->real_reg / 8] =
					real_regs[stats->ssa_regs[i]->real_reg / 8] &
						~(1 << stats->ssa_regs[i]->real_reg % 8);
			}
		}
	}
	stats->real_regc = real_regc;
	free(real_regs);
}

void analyzer_convert_block(ProcedureStats* stats) {
	stats->blocks = calloc(++stats->block_count, sizeof(InstrBlock));
	InstrBlock* current = &stats->blocks[stats->block_count - 1];
	memset(current, 0, sizeof(InstrBlock));
	current->block = malloc(sizeof(ProcInstr*));
	current->block[0] = NULL;
	current->block_length = 0;
	for (size_t i = 0; i < stats->code_length; i++) {
		stats->code[i]->offset = i;
		size_t pc = stats->code[i]->real_offset;
		for (size_t j = 0; j < stats->proc->labels.length; j++)
			if (stats->proc->labels.table[j].value == pc) {
				if (current->block_length > 0) {
					stats->blocks = realloc(stats->blocks, ++stats->block_count * sizeof(InstrBlock));
					current = &stats->blocks[stats->block_count - 1];
					current->block = NULL;
					current->block_length = 0;
				}
				break;
			}
		current->block = realloc(current->block, ++current->block_length * sizeof(ProcInstr*));
		current->block[current->block_length - 1] = stats->code[i];
		uint8_t opcode = stats->code[i]->opcode;
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
					memset(current, 0, sizeof(InstrBlock));
					current->block = NULL;
					current->block_length = 0;

		}
	}
}

void analyze_links(ProcedureStats* stats) {
	for (size_t i = 0; i < stats->ssa_reg_count; i++) {
		SSARegister* reg = stats->ssa_regs[i];
		if (reg->type == DynamicRegister)
			continue;
		for (size_t j = 0; j < i; j++) {
			SSARegister* reg2 = stats->ssa_regs[j];
	  	if (reg2->type == reg->type) {
				if (reg2->value.i64 == reg->value.i64 ||
					reg2->value.fp64 == reg->value.fp64 ||
					reg2->value.boolean == reg->value.boolean) {
						reg->link = reg2;
						break;
				}
			}
		}
	}
}

void analyzer_raw_call_transform(ProcedureStats* stats) {
	ProcInstr** push_regs = NULL;
	size_t push_regc = 0;

	ProcInstr** new_code = NULL;
	size_t new_code_len = 0;
	for (size_t i = 0; i < stats->code_length; i++) {
		ProcInstr* instr = stats->code[i];

		if (instr->opcode == VM_Push) {
			push_regs = realloc(push_regs, ++push_regc * sizeof(ProcInstr*));
			push_regs[push_regc - 1] = instr;
			continue;
		}

		if (instr->opcode == VM_Call ||
			instr->opcode == VM_NewOverload ||
			instr->opcode == VM_NewComplexObject ||
			instr->opcode == VM_NewLambda ||
			instr->opcode == VM_NewInterface) {
			for (size_t j = 0; j < push_regc; j++) {
				new_code = realloc(new_code, ++new_code_len * sizeof(ProcInstr*));
				new_code[new_code_len - 1] = push_regs[j];
			}
			free(push_regs);
			push_regc = 0;
			push_regs = NULL;
		}
				
		new_code = realloc(new_code, ++new_code_len * sizeof(ProcInstr*));
		new_code[new_code_len - 1] = instr;
	}

	free(stats->code);
	stats->code = new_code;
	stats->code_length = new_code_len;
}

void analyze_mark_dead(ProcedureStats* stats) {
	for (size_t i = 0; i < stats->ssa_reg_count; i++)
		if (stats->ssa_regs[i]->last_use == -1)
			stats->ssa_regs[i]->dead = true;
	for (ssize_t i = stats->code_length - 1; i >= 0; i--) {
		struct opcode_arg_info info = get_opcode_arg_info(stats->code[i]->opcode);
		if (stats->code[i]->opcode == VM_Call)
			continue;
		if (info.returns_result && stats->code[i]->args[0] != -1) {
			if (stats->ssa_regs[stats->code[i]->args[0]]->dead) {
				stats->code[i]->dead = true;
				if (info.mod_args[1] && stats->code[i]->args[1] != -1)
					stats->ssa_regs[stats->code[i]->args[1]]->dead = true;
				if (info.mod_args[2] && stats->code[i]->args[2] != -1)
					stats->ssa_regs[stats->code[i]->args[2]]->dead = true;
			}
		}
	}
}

void analyze_calculate_static(ProcedureStats* stats) {
	for (size_t pc = 0; pc < stats->code_length; pc++) {
		ProcInstr* instr = stats->code[pc];

		if (instr->opcode == VM_LoadInteger) {
			stats->ssa_regs[instr->args[0]]->type = StaticI64;
			stats->ssa_regs[instr->args[0]]->value.i64 = instr->args[1];
		}
		if (instr->opcode == VM_LoadConstant) {
			Constant* cnst = stats->bytecode->getConstant(stats->bytecode, instr->args[1]);
			if (cnst->type == IntegerC) {
				stats->ssa_regs[instr->args[0]]->type = StaticI64;
				stats->ssa_regs[instr->args[0]]->value.i64 = cnst->value.i64;
			} else if (cnst->type == FloatC) {
				stats->ssa_regs[instr->args[0]]->type = StaticFp64;
				stats->ssa_regs[instr->args[0]]->value.i64 = cnst->value.fp64;
			} else if (cnst->type == BooleanC) {
				stats->ssa_regs[instr->args[0]]->type = StaticBool;
				stats->ssa_regs[instr->args[0]]->value.i64 = cnst->value.boolean;
			}
		}
	}

}

void analyze_calculate_jumps(ProcedureStats* stats) {
	for (size_t pc = 0; pc < stats->code_length; pc++) {
		uint8_t opcode = stats->code[pc]->opcode;
		if (opcode == VM_Jump || opcode == VM_JumpIfTrue || opcode == VM_JumpIfFalse ||
				opcode == VM_JumpIfEquals || opcode == VM_JumpIfLesser || opcode == VM_JumpIfGreater ||
				opcode == VM_JumpIfNotEquals || opcode == VM_JumpIfNotLesser || opcode == VM_JumpIfNotGreater) {
			size_t addr = (size_t) stats->code[pc]->args[0];
			for (size_t bl = 0; bl < stats->block_count; bl++) {
				if (stats->blocks[bl].block[0]->real_offset == addr) {
					stats->code[pc]->args[0] = stats->blocks[bl].block[0]->offset;
						break;
				}
			}
		}
	}
}

void analyze_delays(ProcedureStats* stats) {
	for (size_t pc = 0; pc < stats->code_length; pc++) {
		ProcInstr* i = stats->code[pc];
		if (i->affects != NULL &&
			i->opcode != VM_Call &&
			i->affects->first_read != pc + 1)
			i->max_delay = i->affects->first_read;

	}
}

void analyze_dependencies(ProcedureStats* stats) {
	for (size_t pc = 0; pc < stats->code_length; pc++) {
		ProcInstr* i = stats->code[pc];
		if (i->affects != NULL)
			i->affects->cmd = i;
		struct opcode_arg_info arg_info = get_opcode_arg_info(i->opcode);
		if (arg_info.mod_args[0] && i->args[0] > -1) {
			i->dependencies.instr1 = stats->ssa_regs[i->args[0]]->cmd;	
		}
		if (arg_info.mod_args[1] && i->args[1] > -1) {
			i->dependencies.instr2 = stats->ssa_regs[i->args[1]]->cmd;	
		}
		if (arg_info.mod_args[2] && i->args[2] > -1) {
			i->dependencies.instr3 = stats->ssa_regs[i->args[2]]->cmd;	
		}
	}
}


ProcedureStats* analyze(ILProcedure* proc, ILBytecode* bc) {
	ProcedureStats* stats = calloc(1, sizeof(ProcedureStats));
	stats->proc = proc;
	stats->bytecode = bc;
	analyzer_convert_code(stats);
	analyzer_raw_call_transform(stats);
	analyze_calculate_static(stats);
	analyze_links(stats);
	analyze_register_area(stats);
	analyze_delays(stats);
	analyze_mark_dead(stats);
	analyze_dependencies(stats);
	analyzer_convert_block(stats);
	analyze_calculate_jumps(stats);


	return stats;
}

void print_stats(ProcedureStats* stats) {
	printf("Procedure %"PRId32":\n", stats->proc->id);
	size_t pc = 0;
	for (size_t i = 0; i < stats->block_count; i++) {
		printf("\tblock%zu:\n", i);
		InstrBlock* block = &stats->blocks[i];
		for (size_t j = 0; j < block->block_length; j++) {
			wchar_t* mnem = NULL;
			for (size_t k = 0; k < OPCODE_COUNT; k++)
				if (Mnemonics[k].opcode == block->block[j]->opcode) {
					mnem = Mnemonics[k].mnemonic;
					break;
				}
			if (mnem != NULL) {
				printf("\t%zu:\t%ls %"PRId32", %"PRId32", %"PRId32"\t", pc, mnem, block->block[j]->args[0],
					block->block[j]->args[1], block->block[j]->args[2]);
				ProcInstr* instr = block->block[j];
				printf("; real offset: %zu", instr->real_offset);
				if (instr->max_delay > -1)
					printf("; may be delayed until %zu", instr->max_delay);
				if (instr->dependencies.instr1 != NULL ||
					instr->dependencies.instr2 != NULL ||
					instr->dependencies.instr3 != NULL) {
					printf("; depends: ");
					if (instr->dependencies.instr1 != NULL)
						printf("%zu ", instr->dependencies.instr1->offset);
					if (instr->dependencies.instr2 != NULL)
						printf("%zu ", instr->dependencies.instr2->offset);
					if (instr->dependencies.instr3 != NULL)
						printf("%zu ", instr->dependencies.instr3->offset);
				}
				printf("\n");
			}
			pc++;
		}
	}
	printf("\n\tRegisters(%zu):\n", stats->ssa_reg_count);
	for (size_t i = 0; i < stats->ssa_reg_count; i++) {
		SSARegister* reg = stats->ssa_regs[i];
		printf("\t\t%zu used from %zi to %zi mapped to %zi", reg->id, reg->first_use, reg->last_use, reg->real_reg);
		printf("; used %zu times", reg->use_count);
		if (reg->cmd != NULL)
			printf("; command: %zu", reg->cmd->offset);
		if (reg->link != NULL)
			printf("; linked to %zu", reg->link->id);
		if (reg->type == DynamicRegister) {
			char* types[] = {"int", "float", "bool", "string", "array", "object", "lambda", "null"};
			int regt = Int64RT;
			uint32_t max = reg->runtime.type[0];
			for (size_t i = 0; i <= sizeof(reg->runtime.type) / sizeof(uint32_t); i++) {
				if (reg->runtime.type[i] > max) {
					regt = i;
					max = reg->runtime.type[i];
				}
			}
			printf("; type = %s\n", types[regt]);			
		}
		else switch (reg->type) {
			case StaticI64:
				printf("; value(i64) = %"PRId64"\n", reg->value.i64);
			break;
			case StaticFp64:
				printf("; value(double) = %lf\n", reg->value.fp64);
			break;
			case StaticBool:
			printf("; value(bool) = %s\n", reg->value.i64 ? "true" : " false");
			break;
			case StaticNullPtr:
				printf("; value = null\n");
			break;
			default:
			break;
		}
	}
}

void procedure_stats_free(ProcedureStats* proc) {
	if (proc == NULL)
		return;
	for (size_t reg = 0; reg < proc->ssa_reg_count; reg++)
		free(proc->ssa_regs[reg]);
	for (size_t pc = 0; pc < proc->code_length; pc++)
		free(proc->code[pc]);
	for (size_t i = 0; i < proc->block_count; i++)
		free(proc->blocks[i].block);
	free(proc->ssa_regs);
	free(proc->code);
	free(proc->blocks);
	free(proc);
}
