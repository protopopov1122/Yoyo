#ifndef YOYO_VM_HEADERS_JIT_H_
#define YOYO_VM_HEADERS_JIT_H_

#include "yoyo-runtime.h"

typedef struct ILProcedure ILProcedure;
typedef struct ILBytecode ILBytecode;

typedef YValue* (*NativeProcedure)(YObject*, YThread*);

typedef struct CompiledProcedure {
		NativeProcedure call;
			void (*free)(struct CompiledProcedure*);
} CompiledProcedure;

typedef struct JitCompiler {
	CompiledProcedure* (*compile)(ILProcedure*, ILBytecode*);
	void (*free)(struct JitCompiler*);
} JitCompiler;

typedef JitCompiler* (*JitGetter)();

JitCompiler* getYoyoJit();

#endif
