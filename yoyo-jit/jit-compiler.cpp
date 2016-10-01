#include "headers/jit-compiler.hpp"

#ifdef __x86_64__
extern "C" JitCompiler* getYoyoJit() {
	return getX64Jit();
}
#elif defined(__i386__)
extern "C" JitCompiler* getYoyoJit() {
	return getX86Jit();
}
#else
extern "C" JitCompiler* getYoyoJit() {
	return NULL;
}
#endif
