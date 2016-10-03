#include "yoyo.h"
#include "jitlib.h"
#include <stddef.h>

typedef struct YoyoJitCompiler {
	JitCompiler cmp;

	struct jit* jit;
} YoyoJitCompiler;

#define ACCUM_COUNT 100

typedef struct Frame {
	struct jit* jit;

	jit_value th;
	jit_value* regs;
	size_t regc;
	jit_value stack_ptr;
	jit_value stack_offset;
	jit_value stack_size;
	jit_value null_ptr;
	jit_value accum[ACCUM_COUNT];
	jit_value runtime;
	jit_value ObjectType;
	jit_value IntType;
	jit_value BooleanType;
	jit_value LambdaType;
} Frame;

void test_value(YValue* v, YThread* th) {
printf("%p\n", (void*) v);
 wchar_t* wcs = toString(v, th);
 printf("%ls\n", wcs);
 free(wcs);
}

void exchange_values(YValue* v1, YValue* v2) {
	v1->o.linkc--;
	v2->o.linkc++;
}

void inc_linkc(YValue* v) {
	v->o.linkc++;
}

void dec_linkc(YValue* v) {
	v->o.linkc--;
}

void lock_thread(YThread* th) {
	MUTEX_LOCK(&th->mutex);
}

void unlock_thread(YThread* th) {
	MUTEX_UNLOCK(&th->mutex);
}

#define print_value(v, frame) jit_prepare((frame)->jit);\
															jit_putargr((frame)->jit, v);\
															jit_putargr((frame)->jit, (frame)->th);\
															jit_call((frame)->jit, test_value);
#define get_reg(r, frame) ((r >= 0 && r < frame.regc) ? frame.regs[r] : frame.null_ptr)
void set_reg(ssize_t r, jit_value v, Frame* frame) {
	if (r >= 0 && r < frame->regc) {
		jit_op*  label = jit_beqi(frame->jit, JIT_FORWARD, v, 0);;
		//print_value(v, frame);
/*		jit_ldxi(frame->jit, frame->accum[ACCUM_COUNT-1], frame->regs[r], offsetof(YValue, o) + offsetof(YoyoObject, linkc), sizeof(uint16_t));
		jit_subi(frame->jit, frame->accum[ACCUM_COUNT-1], frame->accum[ACCUM_COUNT-1], 1);
		jit_stxi(frame->jit, offsetof(YValue, o) + offsetof(YoyoObject, linkc), frame->regs[r], frame->accum[ACCUM_COUNT-1], sizeof(uint16_t));
		*/jit_prepare(frame->jit);
		jit_putargr(frame->jit, frame->regs[r]);
		jit_putargr(frame->jit, v);
		jit_call(frame->jit, exchange_values);
		jit_movr(frame->jit, frame->regs[r], v);
/*		jit_ldxi(frame->jit, frame->accum[ACCUM_COUNT-1], frame->regs[r], offsetof(YValue, o) + offsetof(YoyoObject, linkc), sizeof(uint16_t));
		jit_addi(frame->jit, frame->accum[ACCUM_COUNT-1], frame->accum[ACCUM_COUNT-1], 1);
		jit_stxi(frame->jit, offsetof(YValue, o) + offsetof(YoyoObject, linkc), frame->regs[r], frame->accum[ACCUM_COUNT-1], sizeof(uint16_t));*/
		jit_patch(frame->jit, label);
	}
}

void push_reg(jit_value reg, Frame* frame) {

	struct jit* jit = frame->jit;

	jit_op* overflow_jump = jit_bner(frame->jit, (intptr_t) JIT_FORWARD, frame->stack_size, frame->stack_offset);


	jit_addi(jit, frame->stack_size, frame->stack_size, 10);
	jit_movr(jit, frame->accum[0], frame->stack_size);
	jit_muli(jit, frame->accum[0], frame->accum[0], sizeof(YValue*));
	jit_prepare(jit);
	jit_putargr(jit, frame->stack_ptr);
	jit_putargr(jit, frame->accum[0]);
	jit_call(jit, realloc);
	jit_retval(jit, frame->stack_ptr);

	jit_patch(frame->jit, overflow_jump);

	jit_muli(jit, frame->accum[ACCUM_COUNT-1], frame->stack_offset, sizeof(YValue*));
	jit_stxr(jit, frame->stack_ptr, frame->accum[ACCUM_COUNT-1], reg, sizeof(YValue*));
	jit_addi(jit, frame->stack_offset, frame->stack_offset, 1);

}

jit_value pop_reg(Frame* frame) {
	struct jit* jit = frame->jit;

	jit_movr(jit, frame->accum[0], frame->null_ptr);
	jit_op* underflow_jump = jit_beqi(jit, (intptr_t) JIT_FORWARD, frame->stack_offset, 0);
	jit_subi(jit, frame->stack_offset, frame->stack_offset, 1);
	jit_muli(jit, frame->accum[ACCUM_COUNT-1], frame->stack_offset, sizeof(YValue*));
	jit_ldxr(jit, frame->accum[0], frame->stack_ptr, frame->accum[ACCUM_COUNT-1], sizeof(YValue*));	

	jit_patch(jit, underflow_jump);
	return frame->accum[0];
}
jit_value pop_int(Frame* frame) {
	jit_value reg = pop_reg(frame);
	struct jit* jit = frame->jit;
	
	jit_ldxi(jit, frame->accum[1], reg, offsetof(YValue, type), sizeof(void*));
	jit_movi(jit, frame->accum[2], 0);
	jit_op* not_int_jump = jit_bner(jit, (intptr_t) JIT_FORWARD, frame->accum[1], frame->IntType);

	jit_ldxi(jit, frame->accum[2], reg, offsetof(YInteger, value), sizeof(int64_t));

	jit_patch(jit, not_int_jump);
	jit_movr(jit, frame->accum[0], frame->accum[2]);
	return frame->accum[0];
}
#define JITREG(name, num) jit_value name = R(num);
#define GenBr(id, cmd, cmd2) if (label_list[id] != NULL)\
															cmd2;\
									else {\
										if (label_patch_list[id] == NULL)\
											label_patch_list[id] = calloc(1, sizeof(jit_op*));\
										size_t sz = 0;\
										for (;label_patch_list[id][sz] != NULL; sz++);\
										label_patch_list[id][sz] = cmd;\
										label_patch_list[id] = realloc(label_patch_list[id], sizeof(jit_op*) * (sz + 2));\
										label_patch_list[id][sz + 1] = NULL;\
									}
CompiledProcedure* YoyoJit_compile(JitCompiler* jitcmp, ILProcedure* proc, ILBytecode* bc) {
	struct jit* jit = ((YoyoJitCompiler*) jitcmp)->jit;
	CompiledProcedure* cproc = malloc(sizeof(CompiledProcedure));
	cproc->free = (void (*)(CompiledProcedure*)) free;
	jit_prolog(jit, &cproc->call);
	jit_declare_arg(jit, JIT_PTR, sizeof(YObject*));
	jit_declare_arg(jit, JIT_PTR, sizeof(YThread*));
	jit_value proc_scope_reg = R(201);
	jit_getarg(jit, proc_scope_reg, 0);
	Frame frame;
	frame.jit = jit;
	frame.th = R(0);
	frame.regs = malloc(sizeof(jit_value) * proc->regc);
	frame.regc = proc->regc;
	jit_getarg(jit, frame.th, 1);

	jit_prepare(jit);
	jit_putargr(jit, frame.th);
	jit_call(jit, lock_thread);

	frame.stack_ptr = R(frame.regc + 2);
	frame.stack_size = R(frame.regc + 3);
	frame.stack_offset = R(frame.regc + 4);

	jit_movi(jit, frame.stack_offset, 0);
	jit_movi(jit, frame.stack_size, 100);
	jit_prepare(jit);
	jit_putargr(jit, frame.stack_size);
	jit_call(jit, malloc);
	jit_retval(jit, frame.stack_ptr);
	
	frame.null_ptr = R(frame.regc + 1 + 4);
	jit_prepare(jit);
	jit_putargr(jit, frame.th);
	jit_call(jit, getNull);
	jit_retval(jit, frame.null_ptr);
	for (size_t i = 0; i < ACCUM_COUNT; i++) {
		frame.accum[i] = R(frame.regc + 2 + 4 + i);
	}

	frame.runtime = R(frame.regc + 2 + 4 + ACCUM_COUNT);
	frame.ObjectType = R(frame.regc + 2 + 4 + ACCUM_COUNT + 1);
	frame.IntType = R(frame.regc + 2 + 4 + ACCUM_COUNT + 2);
	frame.BooleanType = R(frame.regc + 2 + 4 + ACCUM_COUNT + 3);
	frame.LambdaType = R(frame.regc + 2 + 4 + ACCUM_COUNT + 4);
	for (size_t i = 0; i < frame.regc; i++) {
		frame.regs[i] = R(i + 1);
		jit_movr(jit, frame.regs[i], frame.null_ptr);
		jit_ldxi(jit, frame.accum[0], frame.regs[i], offsetof(YValue, o) + offsetof(YoyoObject, linkc), JIT_UNSIGNED_NUM);
		jit_addi(jit, frame.accum[0], frame.accum[0], 1);
		jit_stxi(jit, offsetof(YValue, o) + offsetof(YoyoObject, linkc), frame.regs[i], frame.accum[0], JIT_UNSIGNED_NUM);
	}
	jit_ldxi(jit, frame.runtime, frame.th, offsetof(YThread, runtime), sizeof(YRuntime*));
	jit_addi(jit, frame.IntType, frame.runtime, offsetof(YRuntime, IntType));
	jit_addi(jit, frame.ObjectType, frame.runtime, offsetof(YRuntime, ObjectType));
	jit_addi(jit, frame.BooleanType, frame.runtime, offsetof(YRuntime, BooleanType));
	jit_addi(jit, frame.LambdaType, frame.runtime, offsetof(YRuntime, LambdaType));
	set_reg(0, proc_scope_reg, &frame);
	//print_value(frame.regs[0], &frame);

	jit_label** label_list = calloc(proc->labels.length, sizeof(jit_label));
	jit_op*** label_patch_list = calloc(proc->labels.length, sizeof(jit_op**));

	jit_prepare(jit);
	jit_putargr(jit, frame.th);
	jit_call(jit, unlock_thread);

	size_t pc = 0;
  while (pc+13<=proc->code_length) {
		for (size_t i=0;i<proc->labels.length;i++) {
			if (proc->labels.table[i].value==pc) {
				label_list[i] = jit_get_label(jit);
				if (label_patch_list[i] != NULL) {
					for (size_t j = 0; label_patch_list[i][j] != NULL; j++)
						jit_patch(jit, label_patch_list[i][j]);
				}
			}
		}

		jit_value thread_state = frame.accum[1];
		jit_value paused_flag = frame.accum[0];
		jit_movi(jit, thread_state, ThreadPaused);
		jit_label* paused_label = jit_get_label(jit);
		jit_stxi(jit, offsetof(YThread, state), frame.th, thread_state, sizeof(int));
		jit_ldxi(jit, paused_flag, frame.runtime, offsetof(YRuntime, state), sizeof(int));
		jit_beqi(jit, paused_label, paused_flag, RuntimePaused);
		jit_movi(jit, thread_state, ThreadPaused);
		jit_stxi(jit, offsetof(YThread, state), frame.th, thread_state, sizeof(int));
		
   	uint8_t opcode = proc->code[pc];
   	int32_t* args = (int32_t*) &proc->code[pc+1];
   	switch (opcode) {
			case VM_LoadConstant: {
				Constant* cnst = bc->getConstant(bc, args[1]);
				switch (cnst->type) {
					case IntegerC: {
						jit_prepare(jit);
						jit_putargi(jit, cnst->value.i64);
						jit_putargr(jit, frame.th);
						jit_call(jit, newInteger);
						jit_retval(jit, frame.accum[0]);
						set_reg(args[0], frame.accum[0], &frame);
					}
					break;
					case BooleanC: {
						jit_prepare(jit);
						jit_putargi(jit, cnst->value.boolean);
						jit_putargr(jit, frame.th);
						jit_call(jit, newBoolean);
						jit_retval(jit, frame.accum[0]);
						set_reg(args[0], frame.accum[0], &frame);
					}
					break;
					default:
					break;
				}
			}
			break;
			case VM_LoadInteger: {
				jit_prepare(jit);
				jit_putargi(jit, args[1]);
				jit_putargr(jit, frame.th);
				jit_call(jit, newInteger);
				jit_retval(jit, frame.accum[0]);
				set_reg(args[0], frame.accum[0], &frame);
			}
			break;
			case VM_Copy: {
				jit_value val = get_reg(args[1], frame);
				set_reg(args[0], val, &frame);
			}
			break;
			case VM_Push: {
				push_reg(get_reg(args[0], frame), &frame);
			}
			break;
			case VM_Pop: {
				set_reg(args[0], pop_reg(&frame), &frame);
			}
			break;

			case VM_GetField: {
				jit_value obj = get_reg(args[1], frame);
				jit_ldxi(jit, frame.accum[0], obj, offsetof(YValue, type), sizeof(void*));
				jit_ldxi(jit, frame.accum[0], frame.accum[0], offsetof(YType, oper)+offsetof(Operations, readProperty), sizeof(void*));
				jit_op* rp_null = jit_beqi(jit, JIT_FORWARD, frame.accum[0], 0);
				jit_prepare(jit);
				jit_putargi(jit, args[2]);
				jit_putargr(jit, obj);
				jit_putargr(jit, frame.th);
				jit_callr(jit, frame.accum[0]);
				jit_retval(jit, frame.accum[0]);
				set_reg(args[0], frame.accum[0], &frame);
				jit_patch(jit, rp_null);
			}
			break;

			case VM_SetField: {
				jit_value obj = get_reg(args[0], frame);

				jit_ldxi(jit, frame.accum[0], obj, offsetof(YValue, type), sizeof(void*));
				jit_op* not_obj_label = jit_bner(jit, (intptr_t) JIT_FORWARD, frame.accum[0], frame.ObjectType);
				
				jit_ldxi(jit, frame.accum[0], obj, offsetof(YObject, put), sizeof(void*));
				jit_prepare(jit);
				jit_putargr(jit, obj);
				jit_putargi(jit, args[1]);
				jit_putargr(jit, get_reg(args[2], frame));
				jit_putargi(jit, false);
				jit_putargr(jit, frame.th);
				jit_callr(jit, frame.accum[0]);


				jit_patch(jit, not_obj_label);
			}
			break;

			case VM_NewField: {
				jit_value obj = get_reg(args[0], frame);
				jit_ldxi(jit, frame.accum[0], obj, offsetof(YValue, type), sizeof(void*));
				jit_op* not_obj_label = jit_bner(jit, (intptr_t) JIT_FORWARD, frame.accum[0], frame.ObjectType);
				
				jit_ldxi(jit, frame.accum[0], obj, offsetof(YObject, put), sizeof(void*));
				jit_prepare(jit);
				jit_putargr(jit, obj);
				jit_putargi(jit, args[1]);
				jit_putargr(jit, get_reg(args[2], frame));
				jit_putargi(jit, true);
				jit_putargr(jit, frame.th);
				jit_callr(jit, frame.accum[0]);

				jit_patch(jit, not_obj_label);
			}
			break;

			case VM_DeleteField: {
				jit_value obj = get_reg(args[0], frame);
				jit_ldxi(jit, frame.accum[0], obj, offsetof(YValue, type), sizeof(void*));
				jit_op* not_obj_label = jit_bner(jit, (intptr_t) JIT_FORWARD, frame.accum[0], frame.ObjectType);
				
				jit_ldxi(jit, frame.accum[0], obj, offsetof(YObject, remove), sizeof(void*));
				jit_prepare(jit);
				jit_putargr(jit, obj);
				jit_putargi(jit, args[1]);
				jit_putargr(jit, frame.th);
				jit_callr(jit, frame.accum[0]);

				jit_patch(jit, not_obj_label);
			}
			break;

			#define BinOp(name) {\
				jit_value a1 = get_reg(args[1], frame);\
				jit_value a2 = get_reg(args[2], frame);\
				jit_ldxi(jit, frame.accum[0], a1, offsetof(YValue, type), sizeof(void*));\
				jit_ldxi(jit, frame.accum[0], frame.accum[0], offsetof(YType, oper) + offsetof(Operations, name), sizeof(void*));\
				jit_prepare(jit);\
				jit_putargr(jit, a1);\
				jit_putargr(jit, a2);\
				jit_putargr(jit, frame.th);\
				jit_callr(jit, frame.accum[0]);\
				jit_retval(jit, frame.accum[0]);\
				set_reg(args[0], frame.accum[0], &frame);\
			}

			#define UnOp(name) {\
				jit_value a1 = get_reg(args[1], frame);\
				jit_ldxi(jit, frame.accum[0], a1, offsetof(YValue, type), sizeof(void*));\
				jit_ldxi(jit, frame.accum[0], frame.accum[0],offsetof(YType, oper) + offsetof(Operations, name), sizeof(void*));\
				jit_prepare(jit);\
				jit_putargr(jit, a1);\
				jit_putargr(jit, frame.th);\
				jit_callr(jit, frame.accum[0]);\
				jit_retval(jit, frame.accum[0]);\
				set_reg(args[0], frame.accum[0], &frame);\
			}

			case VM_Add: BinOp(add_operation) break;
			case VM_Subtract: BinOp(subtract_operation) break;
			case VM_Multiply: BinOp(multiply_operation) break;
			case VM_Divide: BinOp(divide_operation) break;
			case VM_Power: BinOp(power_operation) break;
			case VM_Modulo: BinOp(modulo_operation) break;
			case VM_ShiftLeft: BinOp(shl_operation) break;
			case VM_ShiftRight: BinOp(shr_operation) break;
			case VM_Or: BinOp(or_operation) break;
			case VM_And: BinOp(and_operation) break;
			case VM_Xor: BinOp(xor_operation) break;
			case VM_Negate: UnOp(negate_operation) break;
			case VM_Not: UnOp(not_operation) break;

			case VM_Compare: {
				jit_value a1 = get_reg(args[1], frame);
				jit_value a2 = get_reg(args[2], frame);

				jit_ldxi(jit, frame.accum[0], a1, offsetof(YValue, type), sizeof(void*));
				jit_ldxi(jit, frame.accum[0], frame.accum[0], offsetof(YType, oper) + offsetof(Operations, compare), sizeof(void*));
				jit_prepare(jit);
				jit_putargr(jit, a1);
				jit_putargr(jit, a2);
				jit_putargr(jit, frame.th);
				jit_callr(jit, frame.accum[0]);
				jit_retval(jit, frame.accum[0]);

				jit_prepare(jit);
				jit_putargi(jit, frame.accum[0]);
				jit_putargr(jit, frame.th);
				jit_call(jit, newInteger);
				jit_retval(jit, frame.accum[0]);
				set_reg(args[0], frame.accum[0], &frame);
			}
			break;

			case VM_Test: {
				jit_value arg = get_reg(args[1], frame);

				jit_ldxi(jit, frame.accum[0], arg, offsetof(YValue, type), sizeof(void*));
				jit_op* false_jump1 = jit_bner(jit, 0, frame.accum[0], frame.IntType);
				
				jit_ldxi(jit, frame.accum[0], frame.accum[0], offsetof(YInteger, value), sizeof(int64_t));
				jit_andi(jit, frame.accum[0], frame.accum[0], args[2]);
				jit_op* false_jump2 = jit_beqi(jit, JIT_FORWARD, frame.accum[0], 0);

				jit_movi(jit, frame.accum[0], (int) true);
				jit_op* true_jump = jit_jmpi(jit, JIT_FORWARD);

				jit_patch(jit, false_jump1);
				jit_patch(jit, false_jump2);

				jit_movi(jit, frame.accum[0], (int) false);

				jit_patch(jit, true_jump);

				jit_prepare(jit);
				jit_putargr(jit, frame.accum[0]);
				jit_putargr(jit, frame.th);
				jit_call(jit, newBoolean);
				jit_retval(jit, frame.accum[0]);
				set_reg(args[0], frame.accum[0], &frame);
			}
			break;

			case VM_FastCompare: {
				jit_value a1 = get_reg(args[0], frame);
				jit_value a2 = get_reg(args[1], frame);

				jit_ldxi(jit, frame.accum[10], a1, offsetof(YValue, type), sizeof(void*));
				jit_ldxi(jit, frame.accum[10], frame.accum[10], offsetof(YType, oper) + offsetof(Operations, compare), sizeof(void*));
				jit_prepare(jit);
				jit_putargr(jit, a1);
				jit_putargr(jit, a2);
				jit_putargr(jit, frame.th);
				jit_callr(jit, frame.accum[10]);
				jit_retval(jit, frame.accum[11]);

				jit_andi(jit, frame.accum[11], frame.accum[11], args[2]);
				jit_op* false_jump2 = jit_beqi(jit, JIT_FORWARD, frame.accum[11], 0);

				jit_movi(jit, frame.accum[12], (int) true);
				jit_op* true_jump = jit_jmpi(jit, JIT_FORWARD);

				jit_patch(jit, false_jump2);

				jit_movi(jit, frame.accum[12], (int) false);

				jit_patch(jit, true_jump);

				jit_prepare(jit);
				jit_putargr(jit, frame.accum[12]);
				jit_putargr(jit, frame.th);
				jit_call(jit, newBoolean);
				jit_retval(jit, frame.accum[12]);
				set_reg(args[0], frame.accum[12], &frame);

			}
			break;

			case VM_Call: {
				jit_value argc = frame.accum[10];
				jit_value fargs = frame.accum[11];
				jit_value counter = frame.accum[12];
				jit_value offset = frame.accum[13];
				jit_movr(jit, argc, pop_int(&frame));
				jit_muli(jit, frame.accum[0], argc, sizeof(YValue*));
				jit_prepare(jit);
				jit_putargr(jit, frame.accum[0]);
				jit_call(jit, malloc);
				jit_retval(jit, fargs);

				jit_subi(jit, counter, argc, 1);

				jit_op* pop_args_loop = jit_beqi(jit, (intptr_t) JIT_FORWARD, counter, -1);

				
				jit_value arg = pop_reg(&frame);

				jit_muli(jit, offset, counter, sizeof(YValue*));
				jit_stxr(jit, fargs, offset, arg, sizeof(YValue*));
				jit_subi(jit, counter, counter, 1);


				jit_patch(jit, pop_args_loop);

				jit_value scope;
				if (args[2] != -1) {
					scope = get_reg(args[2], frame);
					jit_value scope_type = frame.accum[14];
					jit_ldxi(jit, scope_type, scope, offsetof(YValue, type), sizeof(void*));
					jit_op* object_scope = jit_beqr(jit, (intptr_t) JIT_FORWARD, scope_type, frame.ObjectType);

					jit_value val = frame.accum[14];
					jit_value proc_ptr = frame.accum[15];
					jit_ldxi(jit, proc_ptr, frame.runtime, offsetof(YRuntime, newObject), sizeof(void*));
					jit_movr(jit, val, scope);
					jit_prepare(jit);
					jit_putargi(jit, 0);
					jit_putargr(jit, frame.th);
					jit_call(jit, proc_ptr);
					jit_retval(jit, scope);
					
					jit_ldxi(jit, proc_ptr, scope, offsetof(YObject, put), sizeof(void*));
					jit_prepare(jit);
					jit_putargr(jit, scope);
					jit_putargi(jit, bc->getSymbolId(bc, L"value"));
					jit_putargr(jit, val);
					jit_putargi(jit, (int) true);
					jit_putargr(jit, frame.th);
					jit_call(jit, proc_ptr);

					jit_patch(jit, object_scope);
				}
				else
					scope = frame.null_ptr;
				
				jit_value lmbd = get_reg(args[1], frame);
				jit_value lmbd_type = frame.accum[20];
				jit_ldxi(jit, lmbd_type, lmbd, offsetof(YValue, type), sizeof(void*));
				
				jit_op* not_lambda_jump = jit_bner(jit, (intptr_t) JIT_FORWARD, lmbd_type, frame.LambdaType);

				jit_value res = frame.accum[21];
				jit_prepare(jit);
				jit_putargr(jit, lmbd);
				jit_putargr(jit, scope);
				jit_putargr(jit, fargs);
				jit_putargr(jit, argc);
				jit_putargr(jit, frame.th);
				jit_call(jit, invokeLambda);
				jit_retval(jit, res);
				set_reg(args[0], res, &frame);

				jit_op* lambda_jump = jit_jmpi(jit, (intptr_t) JIT_FORWARD);
				jit_patch(jit, not_lambda_jump);

				// TODO

				jit_patch(jit, lambda_jump);

				jit_prepare(jit);
				jit_putargr(jit, fargs);
				jit_call(jit, free);
			}
			break;
			
			case VM_NewObject: {
				jit_value parent = frame.accum[10];
				jit_movi(jit, parent, 0);
				if (args[1] != -1) {
					jit_value reg = get_reg(args[1], frame);
					jit_value type = frame.accum[11];
					jit_ldxi(jit, type, reg, offsetof(YValue, type), sizeof(void*));
					jit_op* not_obj_jump = jit_bner(jit, (intptr_t) JIT_FORWARD, type, frame.ObjectType);
					jit_movr(jit, parent, reg);
					jit_patch(jit, not_obj_jump);
				}
				jit_value obj = frame.accum[11];
				jit_value proc = frame.accum[12];
				jit_ldxi(jit, proc, frame.runtime, offsetof(YRuntime, newObject), sizeof(void*));
				jit_prepare(jit);
				jit_putargr(jit, parent);
				jit_putargr(jit, frame.th);
				jit_callr(jit, proc);
				jit_retval(jit, obj);
				set_reg(args[0], obj, &frame);
			}
			break;

			case VM_Goto: {
				GenBr(args[0], jit_jmpi(jit, JIT_FORWARD), jit_jmpi(jit, label_list[args[0]]));
			}
			break;

			#define CondGoto(cnd) {\
				jit_value a1 = get_reg(args[1], frame);\
				jit_value a2 = get_reg(args[2], frame);\
				/*print_value(a1, &frame);*/\
				/*print_value(a2, &frame);*/\
\
				jit_ldxi(jit, frame.accum[0], a1, offsetof(YValue, type), sizeof(void*));\
				jit_ldxi(jit, frame.accum[0], frame.accum[0], offsetof(YType, oper) + offsetof(Operations, compare), sizeof(void*));\
				jit_prepare(jit);\
				jit_putargr(jit, a1);\
				jit_putargr(jit, a2);\
				jit_putargr(jit, frame.th);\
				jit_callr(jit, frame.accum[0]);\
				jit_retval(jit, frame.accum[0]);\
\
				jit_andi(jit, frame.accum[0], frame.accum[0], cnd);\
				GenBr(args[0], jit_bnei(jit, (intptr_t) JIT_FORWARD, frame.accum[0], 0),\
												jit_bnei(jit, (intptr_t) label_list[args[0]], frame.accum[0], 0));\
			}
			
			case VM_GotoIfEquals: CondGoto(COMPARE_EQUALS) break;
			case VM_GotoIfNotEquals: CondGoto(COMPARE_NOT_EQUALS) break;
			case VM_GotoIfGreater: CondGoto(COMPARE_GREATER) break;
			case VM_GotoIfLesser: CondGoto(COMPARE_LESSER) break;
			case VM_GotoIfNotGreater: CondGoto(COMPARE_LESSER_OR_EQUALS) break;
			case VM_GotoIfNotLesser: CondGoto(COMPARE_GREATER_OR_EQUALS) break;
			case VM_GotoIfTrue: {
				jit_value arg = get_reg(args[1], frame);

				jit_ldxi(jit, frame.accum[0], arg, offsetof(YValue, type), sizeof(void*));
				jit_op* false_jump1 = jit_bner(jit, (intptr_t) JIT_FORWARD, frame.accum[0], frame.BooleanType);
				
				jit_ldxi(jit, frame.accum[0], frame.accum[0], offsetof(YBoolean, value), sizeof(bool));
				GenBr(args[0], jit_beqi(jit, (intptr_t) JIT_FORWARD, frame.accum[0], (int) true),
											jit_beqi(jit, (intptr_t) label_list[args[0]], frame.accum[0], true));

				jit_patch(jit, false_jump1);
			}
			break;
			case VM_GotoIfFalse: {
				jit_value arg = get_reg(args[1], frame);

				jit_ldxi(jit, frame.accum[0], arg, offsetof(YValue, type), sizeof(void*));
				jit_op* false_jump1 = jit_bner(jit, (intptr_t) JIT_FORWARD, frame.accum[0], frame.BooleanType);
				
				jit_ldxi(jit, frame.accum[0], arg, offsetof(YBoolean, value), sizeof(bool));
				GenBr(args[0], jit_beqi(jit, (intptr_t) JIT_FORWARD, frame.accum[0], (int) false),
											jit_beqi(jit, (intptr_t) label_list[args[0]], frame.accum[0], false));

				jit_patch(jit, false_jump1);
			}
			break;

			default:
				printf("Unknown opcode %x\n", opcode);
			break;
		}

		/*jit_prepare(jit);
		jit_putargi(jit, "pc: %x %x\n");
		jit_putargi(jit, pc);
		jit_putargi(jit, opcode);
		jit_call(jit, printf);*/

		pc += 13;
	}

	jit_prepare(jit);
	jit_putargr(jit, frame.th);
	jit_call(jit, lock_thread);

	for (size_t i = 0; i < frame.regc; i++) {
		jit_ldxi_u(jit, frame.accum[0], frame.regs[i], offsetof(YValue, o) + offsetof(YoyoObject, linkc), sizeof(uint16_t));
		jit_subi(jit, frame.accum[0], frame.accum[0], 1);
		jit_stxi(jit, offsetof(YValue, o) + offsetof(YoyoObject, linkc), frame.regs[i], frame.accum[0], sizeof(uint16_t));
	}
	jit_prepare(jit);
	jit_putargr(jit, frame.stack_ptr);
	jit_call(jit, free);

	jit_prepare(jit);
	jit_putargr(jit, frame.th);
	jit_call(jit, unlock_thread);

	free(label_list);
	for (size_t i = 0; i < proc->labels.length; i++)
		free(label_patch_list[i]);
	free(label_patch_list);
	jit_retr(jit, frame.regs[0]);
	//jit_dump_ops(jit, JIT_DEBUG_OPS);
	//jit_check_code(jit, JIT_WARN_ALL);
	jit_generate_code(jit);
	free(frame.regs);
	return cproc;
}

void YoyoJit_free(JitCompiler* cmp) {
	YoyoJitCompiler* jitcmp = (YoyoJitCompiler*) cmp;
	jit_free(jitcmp->jit);
	free(jitcmp);
}

JitCompiler* getYoyoJit() {
	YoyoJitCompiler* cmp = malloc(sizeof(YoyoJitCompiler));
	cmp->jit = jit_init();
	cmp->cmp.compile = YoyoJit_compile;
	cmp->cmp.free = YoyoJit_free;
	return (JitCompiler*) cmp;
}
