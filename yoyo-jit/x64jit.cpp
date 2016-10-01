
#include "headers/jit-compiler.hpp"

#define ASMJIT_BUILD_X64

#include <cstddef>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <map>
#include <asmjit/asmjit.h>

using namespace std;
using namespace asmjit;

typedef struct X64JitCompiler {
	JitCompiler jit;

	JitRuntime* runtime;
} X64JitCompiler;

void X64Jit_free(JitCompiler* jc) {
	X64JitCompiler* jit = (X64JitCompiler*) jc;

	delete jit->runtime;

	free(jc);
}

typedef struct x64CompiledProcedure {
    CompiledProcedure cproc;

    JitRuntime* runtime;
} x64CompiledProcedure;

void x64CompiledProcedure_free(CompiledProcedure* cproc)
{
    x64CompiledProcedure* proc = (x64CompiledProcedure*) cproc;
    proc->runtime->release((void*) cproc->call);
    free(proc);
}

typedef struct ProcedureFrame {
	X86GpVar* regs;
	size_t regc;
	X86Mem stack;
	size_t stack_size;
	X86GpVar stack_index;
	X86GpVar scope;
	X86GpVar null_ptr;
	X86GpVar ObjectType;
	X86GpVar th;
	X86GpVar runtime;
} ProcedureFrame;

void x64_set_register(X86GpVar var, int32_t reg, ProcedureFrame* frame, X86Compiler* c) {
	if (reg>-1&&reg<frame->regc) {
		// TODO NULL check
		c->dec(x86::ptr(frame->regs[reg], offsetof(YValue, o)+offsetof(YoyoObject, linkc)));
		c->mov(frame->regs[reg], var);
		c->inc(x86::ptr(frame->regs[reg], offsetof(YValue, o)+offsetof(YoyoObject, linkc)));
	}
}

void x64_push(X86GpVar var, ProcedureFrame* frame, X86Compiler* c) {
	// TODO NULL check
	c->mov(frame->stack.clone().setIndex(frame->stack_index), var);
	c->add(frame->stack_index, imm(sizeof(void*)));
	c->inc(x86::ptr(var, offsetof(YValue, o)+offsetof(YoyoObject, linkc)));
}

void x64_pop(X86GpVar var, ProcedureFrame* frame, X86Compiler* c) {
	Label end_l = c->newLabel();
	c->mov(var, frame->null_ptr);
	c->cmp(frame->stack_index, 0);
	c->je(end_l);

	c->sub(frame->stack_index, imm(sizeof(void*)));
	c->mov(var, frame->stack.clone().setIndex(frame->stack_index));
	c->dec(x86::ptr(var, offsetof(YValue, o)+offsetof(YoyoObject, linkc)));

	c->bind(end_l);
}

void x64_popInt(X86GpVar var, ProcedureFrame* frame, X86Compiler* c) {
	X86GpVar ptr = c->newIntPtr();
	x64_pop(ptr, frame, c);
	c->mov(var, x86::ptr(ptr, offsetof(YInteger, value)));
	c->unuse(ptr);
}

void test_fun(YValue* v, YThread* th) {
	wchar_t* wcs = toString(v, th);
	printf("\t%ls\n", wcs);
	free(wcs);
}

#define x64_get_register(reg, frame) (reg>-1&&reg<(frame)->regc ? (frame)->regs[reg] : (frame)->null_ptr)
#define x64_assert_type(var, tp, c) {X86GpVar _var = c.newIntPtr();\
																		c.mov(_var, x86::ptr(var, offsetof(YValue, type)));\
																		c.cmp(_var, tp);\
																		c.unuse(_var);}


CompiledProcedure* X64Jit_compile(JitCompiler* jc, ILProcedure* proc, ILBytecode* bc) {
	if (proc==NULL)
		return NULL;
	X64JitCompiler* jit = (X64JitCompiler*) jc;
	StringLogger logger;
	X86Assembler a(jit->runtime);  
	a.setLogger(&logger);
    X86Compiler c(&a);

    x64CompiledProcedure* cproc = (x64CompiledProcedure*) malloc(sizeof(x64CompiledProcedure));
    cproc->runtime = jit->runtime;
    cproc->cproc.free = x64CompiledProcedure_free;

		map<int32_t, Label> label_map;
		for (size_t i=0;i<proc->labels.length;i++)
			label_map[proc->labels.table[i].id] = c.newLabel();
    c.addFunc(FuncBuilder2<YValue*, YObject*, YThread*>(kCallConvHost));
    // Frame initialization
    ProcedureFrame frame;
    frame.scope = c.newIntPtr("scope");
    frame.th = c.newIntPtr("thread");
    c.setArg(0, frame.scope);
    c.setArg(1, frame.th);
    frame.regs = new X86GpVar[proc->regc];
    frame.regs[0] = frame.scope;
	c.inc(x86::ptr(frame.regs[0], offsetof(YValue, o)+offsetof(YoyoObject, linkc)));
    frame.null_ptr = c.newIntPtr("null_ptr");
    X86CallNode* getNull_call = c.call(imm_ptr(getNull),
    		FuncBuilder1<YValue*, YThread*>(kCallConvHost));
    getNull_call->setArg(0, frame.th);
    getNull_call->setRet(0, frame.null_ptr);
    for (size_t i=1;i<proc->regc;i++) {
    	frame.regs[i] = c.newIntPtr("reg_%zu", i);
    	c.mov(frame.regs[i], frame.null_ptr);
    	c.inc(x86::ptr(frame.regs[i], offsetof(YValue, o)+offsetof(YoyoObject, linkc)));
    }
    frame.regc = proc->regc;
    frame.stack = c.newStack(256, sizeof(void*), "yoyo_stack");
    frame.stack_size = 256;
    frame.stack_index = c.newIntPtr("stack_index");
		c.xor_(frame.stack_index, frame.stack_index);
    c.mov(frame.stack_index, imm(0));
		frame.runtime = c.newIntPtr("runtime");
		c.mov(frame.runtime, x86::ptr(frame.th, offsetof(YThread, runtime)));
		frame.ObjectType = c.newIntPtr("runtime_ObjectType");
		c.mov(frame.ObjectType, frame.runtime);
		c.add(frame.ObjectType, offsetof(YRuntime, ObjectType));


    size_t pc = 0;
    while (pc+13<=proc->code_length) {
			for (size_t i=0;i<proc->labels.length;i++) {
				if (proc->labels.table[i].value==pc) {
					int32_t id = proc->labels.table[i].id;
					c.bind(label_map[id]);
				}
			}
			
    	uint8_t opcode = proc->code[pc];
    	int32_t* args = (int32_t*) &proc->code[pc+1];
    	switch (opcode) {
    	case VM_LoadConstant: {
    		Constant* cnst = bc->getConstant(bc, args[1]);
    		switch (cnst->type) {
    		case Constant::IntegerC: {
    			int64_t i = cnst->value.i64; 
    			X86GpVar var_c = c.newInt64("constant_%" PRId32, args[1]);
    			c.mov(var_c, imm(i));
    			X86GpVar val_ptr = c.newIntPtr();

    			X86CallNode* call = c.call(imm_ptr(newInteger),
    					FuncBuilder2<YValue*, int64_t, YThread*>(kCallConvHost));
    			call->setArg(0, var_c);
    			call->setArg(1, frame.th);
    			call->setRet(0, val_ptr);

    			c.unuse(var_c);
					x64_set_register(val_ptr, args[0], &frame, &c);
    			c.unuse(val_ptr);
    		}
				break;
				case Constant::BooleanC: {
    			X86GpVar var_c = c.newInt32("constant_%" PRId32, args[1]);
    			c.mov(var_c, imm(cnst->value.boolean));
    			X86GpVar val_ptr = c.newIntPtr();

    			X86CallNode* call = c.call(imm_ptr(newBoolean),
    					FuncBuilder2<YValue*, int32_t, YThread*>(kCallConvHost));
    			call->setArg(0, var_c);
    			call->setArg(1, frame.th);
    			call->setRet(0, val_ptr);

    			c.unuse(var_c);
					x64_set_register(val_ptr, args[0], &frame, &c);
    			c.unuse(val_ptr);

				}
    		break;
				default:
				break;
    		}
    	}
    	break;
		
    	case VM_LoadInteger: {
    			X86GpVar var_c = c.newInt64("integer_%" PRId32, args[1]);
    			c.mov(var_c, imm(args[1]));
    			X86GpVar val_ptr = c.newIntPtr();

    			X86CallNode* call = c.call(imm_ptr(newInteger),
    					FuncBuilder2<YValue*, int64_t, YThread*>(kCallConvHost));
    			call->setArg(0, var_c);
    			call->setArg(1, frame.th);
    			call->setRet(0, val_ptr);

    			c.unuse(var_c);
					x64_set_register(val_ptr, args[0], &frame, &c);
    			c.unuse(val_ptr);

    	}
    	break;
			case VM_Push: {
				x64_push(x64_get_register(args[0], &frame), &frame, &c);
			}
			break;

			case VM_GetField: {
				X86GpVar obj = x64_get_register(args[1], &frame);

				X86GpVar ptr = c.newIntPtr("GetField_ptr");
				c.mov(ptr, x86::ptr(obj, offsetof(YValue, type)));
				c.mov(ptr, x86::ptr(ptr, offsetof(YType, oper)+offsetof(Operations, readProperty)));
			
				X86CallNode* call = c.call(ptr,
					FuncBuilder3<YValue*, int32_t, YValue*, YThread*>(kCallConvHost));
				call->setArg(0, imm(args[2]));
				call->setArg(1, obj);
				call->setArg(2, frame.th);
				X86GpVar val = c.newIntPtr("GetField_value");
				call->setRet(0, val);
				x64_set_register(val, args[0], &frame, &c);
				c.unuse(ptr);
				c.unuse(val);
			}
			break;

			case VM_SetField: {
				X86GpVar obj = x64_get_register(args[0], &frame);
				X86GpVar val = x64_get_register(args[2], &frame);

				Label not_object = c.newLabel();			
				x64_assert_type(obj, frame.ObjectType, c);
				c.jne(not_object);
				
				
				X86GpVar ptr = c.newIntPtr("Object_put");
				c.mov(ptr, x86::ptr(obj, offsetof(YObject, put)));

				X86CallNode* call = c.call(ptr,
					FuncBuilder5<void, YObject*, int32_t, YValue*, int, YThread*>(kCallConvHost));
				call->setArg(0, obj);
				call->setArg(1, imm(args[1]));
				call->setArg(2, val);
				call->setArg(3, imm(false));
				call->setArg(4, frame.th); 
				c.unuse(ptr);

				c.bind(not_object);
			}
			break;

			case VM_NewField: {
				X86GpVar obj = x64_get_register(args[0], &frame);
				X86GpVar val = x64_get_register(args[2], &frame);

				Label not_object = c.newLabel();			
				x64_assert_type(obj, frame.ObjectType, c);
				c.jne(not_object);
				
				
				X86GpVar ptr = c.newIntPtr("Object_put");
				c.mov(ptr, x86::ptr(obj, offsetof(YObject, put)));

				X86CallNode* call = c.call(ptr,
					FuncBuilder5<void, YObject*, int32_t, YValue*, int, YThread*>(kCallConvHost));
				call->setArg(0, obj);
				call->setArg(1, imm(args[1]));
				call->setArg(2, val);
				call->setArg(3, imm(true));
				call->setArg(4, frame.th); 
				c.unuse(ptr);

				c.bind(not_object);
			}
			break;

			case VM_DeleteField: {
				X86GpVar obj = x64_get_register(args[0], &frame);

				Label not_object = c.newLabel();			
				x64_assert_type(obj, frame.ObjectType, c);
				c.jne(not_object);
				
				
				X86GpVar ptr = c.newIntPtr("Object_remove");
				c.mov(ptr, x86::ptr(obj, offsetof(YObject, remove)));

				X86CallNode* call = c.call(ptr,
					FuncBuilder3<void, YObject*, int32_t, YThread*>(kCallConvHost));
				call->setArg(0, obj);
				call->setArg(1, imm(args[1]));
				call->setArg(2, frame.th); 
				c.unuse(ptr);

				c.bind(not_object);
			}
			break;

			case VM_Call: {
				X86GpVar argc = c.newInt32("Call_argc");
				X86GpVar rargs = c.newIntPtr("Call_args");
				x64_popInt(argc, &frame, &c);

				X86GpVar size_reg = c.newUInt32("Call_args_size");
				c.mov(size_reg, argc);
				c.imul(size_reg, imm(sizeof(YValue*)));
				X86CallNode* malloc_call = c.call(imm_ptr(malloc),
					FuncBuilder1<void*, size_t>(kCallConvHost));
				malloc_call->setArg(0, size_reg);
				malloc_call->setRet(0, rargs);
				
				c.mov(size_reg, argc);
				X86GpVar index = c.newUInt32("Call_index");
				X86GpVar value = c.newIntPtr("Call_value");
				Label loop1_start = c.newLabel();
				Label loop1_end = c.newLabel();
				c.bind(loop1_start);
				c.cmp(size_reg, imm(0));
				c.je(loop1_end);
				c.dec(size_reg);
				c.mov(index, size_reg);
				c.imul(index, sizeof(YValue*));
				x64_pop(value, &frame, &c);
				c.mov(x86::ptr(rargs, index), value);
				c.jmp(loop1_start);
				c.bind(loop1_end);
				c.unuse(index);


				X86GpVar lmbd = x64_get_register(args[1], &frame);
				X86CallNode* invokeLambda_call = c.call(imm_ptr(invokeLambda),
					FuncBuilder5<YValue*, YLambda*, YObject*, YValue**, int32_t, YThread*>(kCallConvHost));
				invokeLambda_call->setArg(0, lmbd);
				invokeLambda_call->setArg(1, imm_ptr(NULL));
				invokeLambda_call->setArg(2, rargs);
				invokeLambda_call->setArg(3, argc);
				invokeLambda_call->setArg(4, frame.th);
				invokeLambda_call->setRet(0, value);

				x64_set_register(value, args[0], &frame, &c);


				X86CallNode* free_call = c.call(imm_ptr(free),
					FuncBuilder1<void, void*>(kCallConvHost));
				free_call->setArg(0, rargs);

				c.unuse(argc);	
				c.unuse(rargs);
				c.unuse(value);
				c.unuse(size_reg);
			}
			break;

			#define GenBin(op) {\
				X86GpVar op1 = x64_get_register(args[1], &frame);\
				X86GpVar op2 = x64_get_register(args[2], &frame);\
				X86GpVar val = c.newIntPtr();\
				c.mov(val, x86::ptr(op1, offsetof(YValue, type)));\
				c.mov(val, x86::ptr(val, offsetof(YType, oper)+offsetof(Operations, op)));\
				X86CallNode* call = c.call(val, FuncBuilder3<YValue*, YValue*, YValue*, YThread*>(kCallConvHost));\
				call->setArg(0, op1);\
				call->setArg(1, op2);\
				call->setArg(2, frame.th);\
				call->setRet(0, val);\
				x64_set_register(val, args[0], &frame, &c);\
				c.unuse(val);\
			}

			case VM_Add: GenBin(add_operation); 	break;
			case VM_Subtract: GenBin(subtract_operation); 	break;
			case VM_Multiply: GenBin(multiply_operation); 	break;
			case VM_Divide: GenBin(divide_operation); 	break;
			case VM_Modulo: GenBin(modulo_operation); 	break;
			case VM_Power: GenBin(power_operation); 	break;
			case VM_ShiftLeft: GenBin(shl_operation); 	break;
			case VM_ShiftRight: GenBin(shr_operation); 	break;
			case VM_And: GenBin(and_operation); 	break;
			case VM_Or: GenBin(or_operation); 	break;
			case VM_Xor: GenBin(xor_operation); 	break;

			case VM_Compare: {
				X86GpVar op1 = x64_get_register(args[1], &frame);
				X86GpVar op2 = x64_get_register(args[2], &frame);
				X86GpVar ptr = c.newIntPtr();
				X86GpVar val = c.newInt64();
				c.mov(ptr, x86::ptr(op1, offsetof(YValue, type)));
				c.mov(ptr, x86::ptr(ptr, offsetof(YType, oper)+offsetof(Operations, compare)));
				c.xor_(val, val);
				X86CallNode* call = c.call(ptr, FuncBuilder3<int, YValue*, YValue*, YThread*>(kCallConvHost));
				call->setArg(0, op1);
				call->setArg(1, op2);
				call->setArg(2, frame.th);
				call->setRet(0, val);

				X86CallNode* newInt_call = c.call(imm_ptr(newInteger),
					FuncBuilder2<YValue*, int64_t, YThread*>(kCallConvHost));
				newInt_call->setArg(0, val);
				newInt_call->setArg(1, frame.th);
				newInt_call->setRet(0, ptr);
				x64_set_register(ptr, args[0], &frame, &c);
				c.unuse(val);
				c.unuse(ptr);

			}
			break;
			case VM_Test: {
				X86GpVar op1 = x64_get_register(args[1], &frame);
				X86GpVar op2 = x64_get_register(args[2], &frame);
				Label falseL = c.newLabel();
				Label trueL = c.newLabel();
				/*x64_assert_type(op1, r, c);
				c.jne(falseL);
				x64_assert_type(op2, IntegerT, c);
				c.jne(falseL);*/

				X86GpVar i_var = c.newInt64();
				X86GpVar i2_var = c.newInt64();
				c.mov(i_var, x86::ptr(op1, offsetof(YInteger, value)));
				c.mov(i2_var, x86::ptr(op2, offsetof(YInteger, value)));
				c.and_(i_var, i2_var);
				c.unuse(i2_var);
				c.cmp(i_var, imm(0));
				c.jne(falseL);
				c.unuse(i_var);

    		X86GpVar val_ptr = c.newIntPtr();
    		X86CallNode* call = c.call(imm_ptr(newBoolean),
    			FuncBuilder2<YValue*, int32_t, YThread*>(kCallConvHost));
    		call->setArg(0, imm(true));
    		call->setArg(1, frame.th);
    		call->setRet(0, val_ptr);
				x64_set_register(val_ptr, args[0], &frame, &c);
	   		c.unuse(val_ptr);

				c.bind(falseL);

    		val_ptr = c.newIntPtr();
    		call = c.call(imm_ptr(newBoolean),
    			FuncBuilder2<YValue*, int32_t, YThread*>(kCallConvHost));
    		call->setArg(0, imm(false));
    		call->setArg(1, frame.th);
    		call->setRet(0, val_ptr);
				x64_set_register(val_ptr, args[0], &frame, &c);
	   		c.unuse(val_ptr);

				c.bind(trueL);

			}
			break;

			case VM_Jump: {
				c.cmp(frame.th, frame.th);
				c.je(label_map[args[0]]);	
			}
			break;
			case VM_JumpIfTrue: {
				X86GpVar reg = x64_get_register(args[1], &frame);
				Label notBool = c.newLabel();
				/*x64_assert_type(reg, BooleanT, c);
				c.jne(notBool);*/
				X86GpVar tmp = c.newInt32();

				c.mov(tmp, x86::ptr(reg, offsetof(YBoolean, value)));
				c.and_(tmp, imm(1));

				c.cmp(tmp, imm(true));
				c.je(label_map[args[0]]);
				c.unuse(tmp);
				c.bind(notBool);
			}
			break;
			case VM_JumpIfFalse: {
				X86GpVar reg = x64_get_register(args[1], &frame);
				Label notBool = c.newLabel();
				/*x64_assert_type(reg, BooleanT, c);
				c.jne(notBool);*/
				X86GpVar tmp = c.newInt32();

				c.mov(tmp, x86::ptr(reg, offsetof(YBoolean, value)));
				c.and_(tmp, imm(1));

				c.cmp(tmp, imm(false));
				c.je(label_map[args[0]]);
				c.unuse(tmp);
				c.bind(notBool);
			}
			break;
    	}
    	pc+=13;
    }
		for (size_t i=0;i<proc->labels.length;i++) {
				if (proc->labels.table[i].value==pc) {
					int32_t id = proc->labels.table[i].id;
					c.bind(label_map[id]);
				}
			}


		for (size_t i=0;i<proc->regc;i++) {
    		c.dec(x86::ptr(frame.regs[i], offsetof(YValue, o)+offsetof(YoyoObject, linkc)));
    		c.unuse(frame.regs[i]);
    }

		X86GpVar stack_el = c.newIntPtr();
		Label stack_free_start = c.newLabel();
		Label stack_free_end = c.newLabel();
		c.bind(stack_free_start);
		c.cmp(frame.stack_index, imm(0));
		c.je(stack_free_end);
		x64_pop(stack_el, &frame, &c);
		c.jmp(stack_free_start);
		c.bind(stack_free_end);
		c.unuse(stack_el);


    X86GpVar ret = c.newIntPtr("return");
    c.mov(ret, imm_ptr(NULL));
    c.ret(ret);

    c.unuse(frame.th);
    c.unuse(frame.stack_index);
		c.unuse(frame.null_ptr);
    delete[] frame.regs;

    c.endFunc();
    c.finalize();
    void* funcPtr = a.make();
    cproc->cproc.call = asmjit_cast<NativeProcedure>(funcPtr);

		cout << logger.getString() << endl;

    return (CompiledProcedure*) cproc;
}

JitCompiler* getX64Jit() {
	X64JitCompiler* jit = (X64JitCompiler*) malloc(sizeof(X64JitCompiler));

	jit->runtime = new JitRuntime();

	jit->jit.free = X64Jit_free;
	jit->jit.compile = X64Jit_compile;
    
    jit->jit.compile((JitCompiler*) jit, NULL, NULL);

	return (JitCompiler*) jit;
}
