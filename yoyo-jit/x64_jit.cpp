#include "headers/jit-compiler.hpp"

#include <stdio.h>
#include <iostream>
#include <asmjit/asmjit.h>

using namespace asmjit;

typedef struct X64JitCompiler {
	JitCompiler jit;

	JitRuntime* runtime;
	X86Assembler* as;
	X86Compiler* comp;
} X64JitCompiler;

void X64Jit_free(JitCompiler* jc) {
	X64JitCompiler* jit = (X64JitCompiler*) jc;

	delete jit->comp;
	delete jit->as;
	delete jit->runtime;

	free(jc);
}

JitCompiler* getX64Jit() {
	X64JitCompiler* jit = (X64JitCompiler*) malloc(sizeof(X64JitCompiler));

	jit->runtime = new JitRuntime();
	jit->as = new X86Assembler(jit->runtime);
	jit->comp = new X86Compiler(jit->as);

	jit->jit.free = X64Jit_free;

	return (JitCompiler*) jit;
}
