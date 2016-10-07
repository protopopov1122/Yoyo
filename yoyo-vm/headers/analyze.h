#ifndef YOYOVM_HEADERS_ANALYZE_H
#define YOYOVM_HEADERS_ANALYZE_H

#include "bytecode.h"

typedef struct SSARegister {
	size_t id;
	ssize_t real_reg;

	ssize_t first_use;
	ssize_t last_use;
} SSARegister;

typedef struct ProcInstr {
	uint8_t opcode;

	int32_t args[3];
} ProcInstr;

typedef struct InstrBlock {
	ProcInstr* block;
	size_t block_length;
} InstrBlock;

typedef struct ProcedureStats {
	ILProcedure* proc;

	size_t ssa_reg_count;
	SSARegister** ssa_regs;

	size_t code_length;
	ProcInstr* code;

	InstrBlock* blocks;
	size_t block_count;

	size_t real_regc;
} ProcedureStats;

ProcedureStats* analyze(ILProcedure*, ILBytecode*);

#endif
