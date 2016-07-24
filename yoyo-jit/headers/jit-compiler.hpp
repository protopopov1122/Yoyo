#ifndef YOYO_JIT_HEADERS_JIT_COMPILER_H
#define YOYO_JIT_HEADERS_JIT_COMPILER_H

#include "yoyo.h"

#ifdef __cplusplus

extern "C" {
#endif

#include "jit.h"

#ifdef __cplusplus
}
#endif

JitCompiler* getX86Jit();
JitCompiler* getX64Jit();

#endif
