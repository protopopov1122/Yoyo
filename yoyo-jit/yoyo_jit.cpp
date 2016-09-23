#include "yoyo.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace llvm;

typedef struct YoyoLLVMJit {
	JitCompiler cmp;
} YoyoLLVMJit;

CompiledProcedure* YoyoLLVMJit_compile(JitCompiler* jitcmp, ILProcedure* proc, ILBytecode* bc) {
	return NULL;
}

void YoyoLLVMJit_free(JitCompiler* jitcmp) {
	YoyoLLVMJit* jit = (YoyoLLVMJit*) jitcmp;
	delete jit; 
}

JitCompiler* getYoyoJit() {
	YoyoLLVMJit* jit = new YoyoLLVMJit();

	jit->cmp.compile = YoyoLLVMJit_compile;
	jit->cmp.free = YoyoLLVMJit_free;
	return (JitCompiler*) jit;
}
