#include "yoyo.h"

typedef struct YoyoJitCompiler {
	JitCompiler jit;
} YoyoJitCompiler;

CompiledProcedure* YoyoJit_compile(JitCompiler* jitcmp, ILProcedure* proc, ILBytecode* bc) {
	return NULL;
}

void YoyoJit_free(JitCompiler* jitcmp) {
	YoyoJitCompiler* jit = (YoyoJitCompiler*) jitcmp;
	free(jit);
}

JitCompiler* getYoyoJit() {
	YoyoJitCompiler* jit = malloc(sizeof(YoyoJitCompiler));

	jit->jit.compile = YoyoJit_compile;
	jit->jit.free = YoyoJit_free;

	printf("JIT compiler not supported. Interpretation will be used.\n");

	return (JitCompiler*) jit;
}
