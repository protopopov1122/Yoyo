#ifndef YOYOVM_HEADERS_ANALYZE_H
#define YOYOVM_HEADERS_ANALYZE_H

#include "bytecode.h"


typedef enum RegisterRuntimeType {
	Int64RT = 0, Fp64RT = 1,
	BoolRT = 2, StringRT = 3,
	ArrayRT = 4, ObjectRT = 5,
	LambdaRT =  6, NullRT = 7
} RegisterRuntimeType;

typedef struct SSARegister {
	size_t id;
	ssize_t real_reg;

	ssize_t first_use;
	ssize_t last_use;
	ssize_t first_read;
	struct ProcInstr* cmd;
	size_t use_count;
	bool dead;

	enum {
		DynamicRegister, StaticI64,
		StaticFp64, StaticBool,
		StaticNullPtr
	} type;
	union {
		int64_t i64;
		double fp64;
		bool boolean;
	} value;

	struct {
		uint32_t type[7];
	} runtime;
} SSARegister;

typedef struct ProcInstr {
	uint8_t opcode;

	int32_t args[3];

	size_t real_offset;
	size_t offset;
	bool dead;
	SSARegister* affects;
	ssize_t max_delay;

	struct {
		struct ProcInstr* instr1;
		struct ProcInstr* instr2;
		struct ProcInstr* instr3;
	} dependencies;
} ProcInstr;

typedef struct InstrBlock {
	ProcInstr* block;
	size_t block_length;
} InstrBlock;

typedef struct ProcedureStats {
	ILProcedure* proc;
	ILBytecode* bytecode;

	size_t ssa_reg_count;
	SSARegister** ssa_regs;

	size_t code_length;
	ProcInstr* code;

	InstrBlock* blocks;
	size_t block_count;

	size_t real_regc;
} ProcedureStats;

ProcedureStats* analyze(ILProcedure*, ILBytecode*);
void procedure_stats_free(ProcedureStats*);
void print_stats(ProcedureStats*);

#endif
