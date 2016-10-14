#include "yoyo.h"




#ifdef EXECUTE_PROC

#ifdef SET_REGISTER
#undef SET_REGISTER
#endif

#ifndef COLLECT_STATS
#define SET_REGISTER(value, reg, th) setRegister(value, reg, th)
#else
void track_register_assignment(YValue* value, int32_t reg, YThread* th) {
	setRegister(value, reg, th);
	ExecutionFrame* frame = (ExecutionFrame*) th->frame;
	ProcedureStats* stats = frame->proc->stats;
	if (stats == NULL)
		return;
	ProcInstr* instr = NULL;
	for (size_t i = 0; i < stats->code_length; i++) {
		if (stats->code[i].real_offset == frame->pc) {
			instr = &stats->code[i];
			break;
		}
	}

	if (instr == NULL || instr->affects == NULL)
		return;
	SSARegister* ssa_reg = instr->affects;
//	printf("%zu = %ls;\t", ssa_reg->id, toString(value, th));
	if (value->type == &th->runtime->IntType) {
		ssa_reg->runtime.type[Int64RT]++;
	} else if (value->type == &th->runtime->FloatType) {
		ssa_reg->runtime.type[Fp64RT]++;
	} else if (value->type == &th->runtime->BooleanType) {
		ssa_reg->runtime.type[BoolRT]++;
	} else if (value->type == &th->runtime->StringType) {
		ssa_reg->runtime.type[StringRT]++;
	} else if (value->type == &th->runtime->ArrayType) {
		ssa_reg->runtime.type[ArrayRT]++;
	} else if (value->type == &th->runtime->LambdaType) {
		ssa_reg->runtime.type[LambdaRT]++;
	} else if (value->type == &th->runtime->ObjectType) {
		ssa_reg->runtime.type[ObjectRT]++;
	} else if (value->type == &th->runtime->NullType)
		ssa_reg->runtime.type[NullRT]++;
//	for (size_t i = 0; i < sizeof(ssa_reg->runtime.type) / sizeof(uint32_t); i++)
//		printf("%"PRIu32" ", ssa_reg->runtime.type[i]);
//	printf("\n");
}
#define SET_REGISTER(value, reg, th) track_register_assignment(value, reg, th)
#endif

/*Procedure that interprets current frame bytecode
 * Uses loop with switch statement.*/
YValue* EXECUTE_PROC(YObject* scope, YThread* th) {
	ExecutionFrame* frame = (ExecutionFrame*) th->frame;
	frame->regs[0] = (YValue*) scope;
	YRuntime* runtime = th->runtime;
	ILBytecode* bc = frame->bytecode;
	size_t code_len = frame->proc->code_length;

	while (frame->pc + 13 <= code_len) {
		// If runtime is paused then execution should be paused too.
		if (runtime->state == RuntimePaused) {
			th->state = ThreadPaused;
			while (runtime->state == RuntimePaused)
				YIELD();
			th->state = ThreadWorking;
		}
		// Decode opcode and arguments
		uint8_t opcode = frame->proc->code[frame->pc];

		int32_t* args = (int32_t*) &(frame->proc->code[frame->pc + 1]);
		const int32_t iarg0 = args[0];
		const int32_t iarg1 = args[1];
		const int32_t iarg2 = args[2];

		// Call debugger before each instruction if nescesarry
		YBreakpoint breakpoint = { .procid = frame->proc->id, .pc = frame->pc };
		DEBUG(th->runtime->debugger, instruction, &breakpoint, th);

		// Interpret opcode
		// See virtual machine description
		switch (opcode) {
		case VM_Halt:
			break;
		case VM_LoadConstant: {
			/*All constants during execution should be stored in pool.
			 * If pool contains constant id, then constant is returned*/
			YObject* cpool = th->runtime->Constants.pool;
			if (cpool->contains(cpool, iarg1, th)) {
				SET_REGISTER(cpool->get(cpool, iarg1, th), iarg0, th);
				break;
			}
			/*Else it is created, added to pool and returned*/
			Constant* cnst = bc->getConstant(bc, iarg1);
			YValue* val = getNull(th);
			if (cnst != NULL) {
				switch (cnst->type) {
				case IntegerC:
					val = newInteger(cnst->value.i64, th);
					break;
				case FloatC:
					val = newFloat(cnst->value.fp64, th);
					break;
				case StringC:
					val = newString(
							bc->getSymbolById(bc,
									cnst->value.string_id), th);
					break;
				case BooleanC:
					val = newBoolean(cnst->value.boolean, th);
					break;
				default:
					break;
				}
			}
			cpool->put(cpool, iarg1, val, true, th);
			SET_REGISTER(val, iarg0, th);
		}
			break;
		case VM_LoadInteger: {
			/*Load integer from argument directly to register*/
			YValue* val = newInteger(iarg1, th);
			SET_REGISTER(val, iarg0, th);
		}
			break;
		case VM_Copy: {
			/*Copy register value to another*/
			SET_REGISTER(getRegister(iarg1, th), iarg0, th);
		}
			break;
		case VM_Push: {
			/*Push register value to stack*/
			push(getRegister(iarg0, th), th);
		}
			break;
		case VM_PushInteger: {
			push(newInteger(iarg0, th), th);
		}
		break;

			/*Next instructions load values from two registers,
			 * perform polymorph binary operation and
			 * save result to third register*/
		case VM_Add: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			SET_REGISTER(v1->type->oper.add_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_Subtract: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			SET_REGISTER(v1->type->oper.subtract_operation(v1, v2, th), iarg0,
					th);
		}
			break;
		case VM_Multiply: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			SET_REGISTER(v1->type->oper.multiply_operation(v1, v2, th), iarg0,
					th);
		}
			break;
		case VM_Divide: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			SET_REGISTER(v1->type->oper.divide_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_Modulo: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			SET_REGISTER(v1->type->oper.modulo_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_Power: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			SET_REGISTER(v1->type->oper.power_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_ShiftRight: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			SET_REGISTER(v1->type->oper.shr_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_ShiftLeft: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			SET_REGISTER(v1->type->oper.shl_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_And: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			SET_REGISTER(v1->type->oper.and_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_Or: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			SET_REGISTER(v1->type->oper.or_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_Xor: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			SET_REGISTER(v1->type->oper.xor_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_Compare: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);

			SET_REGISTER(newInteger(v1->type->oper.compare(v1, v2, th), th),
					iarg0, th);
		}
			break;

		case VM_Test: {
			/*Take an integer from register and
			 * check if certain bit is 1 or 0*/
			YValue* arg = getRegister(iarg1, th);
			if (arg->type == &th->runtime->IntType) {
				int64_t i = ((YInteger*) arg)->value;
				YValue* res = newBoolean((i & iarg2) != 0, th);
				SET_REGISTER(res, iarg0, th);
			} else
				SET_REGISTER(getNull(th), iarg0, th);
		}
			break;
		case VM_FastCompare: {
			YValue* v1 = getRegister(iarg0, th);
			YValue* v2 = getRegister(iarg1, th);
			int i = v1->type->oper.compare(v1, v2, th);
			YValue* res = newBoolean((i & iarg2) != 0, th);
			SET_REGISTER(res, iarg0, th);
		}
		break;
			/*These instruction perform polymorph unary operations*/
		case VM_Negate: {
			YValue* v1 = getRegister(iarg1, th);
			SET_REGISTER(v1->type->oper.negate_operation(v1, th), iarg0, th);
		}
			break;
		case VM_Not: {
			YValue* v1 = getRegister(iarg1, th);
			SET_REGISTER(v1->type->oper.not_operation(v1, th), iarg0, th);
		}
			break;
		case VM_Increment: {
			YValue* v1 = getRegister(iarg1, th);
			if (v1->type == &th->runtime->IntType) {
				int64_t i = getInteger(v1, th);
				i++;
				SET_REGISTER(newInteger(i, th), iarg0, th);
			} else if (v1->type==&th->runtime->FloatType) {
				double i = getFloat(v1, th);
				i++;
				SET_REGISTER(newFloat(i, th), iarg0, th);
			} else {
				SET_REGISTER(v1, iarg0, th);
			}

		}
		break;
		case VM_Decrement: {
			YValue* v1 = getRegister(iarg1, th);
			if (v1->type == &th->runtime->IntType) {
				int64_t i = getInteger(v1, th);
				i--;
				SET_REGISTER(newInteger(i, th), iarg0, th);
			} else if (v1->type==&th->runtime->FloatType) {
				double i = getFloat(v1, th);
				i--;
				SET_REGISTER(newFloat(i, th), iarg0, th);
			} else {
				SET_REGISTER(v1, iarg0, th);
			}

		}
		break;

		case VM_Call: {
			/*Invoke lambda from register.
			 * Arguments stored in stack.
			 * Argument count passed as argument*/
			size_t argc = (size_t) popInt(th);
			YValue** args = malloc(sizeof(YValue*) * argc);
			for (size_t i = argc - 1; i < argc; i--)
				args[i] = pop(th);
			YValue* val = getRegister(iarg1, th);
			YObject* scope = NULL;
			if (iarg2 != -1) {
				YValue* scl = getRegister(iarg2, th);
				if (scl->type == &th->runtime->ObjectType)
					scope = (YObject*) scl;
				else {
					scope = th->runtime->newObject(NULL, th);
					OBJECT_NEW(scope, L"value", scl, th);
				}
			}
			if (val->type == &th->runtime->LambdaType) {

				YLambda* l = (YLambda*) val;
				SET_REGISTER(invokeLambda(l, scope, args, argc, th), iarg0, th);
			} else {
				throwException(L"CallingNotALambda", NULL, 0, th);
				SET_REGISTER(getNull(th), iarg0, th);
			}
			free(args);
		}
			break;
		case VM_Return: {
			/*Verify register value to be some type(if defined)
			 * and return it. Execution has been ended*/
			YValue* ret = getRegister(iarg0, th);
			if (((ExecutionFrame*) th->frame)->retType != NULL
					&& !((ExecutionFrame*) th->frame)->retType->verify(
							((ExecutionFrame*) th->frame)->retType, ret, th)) {
				wchar_t* wstr = toString(ret, th);
				throwException(L"Wrong return type", &wstr, 1, th);
				free(wstr);
				return getNull(th);
			}
			return ret;
		}
		case VM_NewObject: {
			/*Create object with parent(if defined)*/
			YValue* p = getRegister(iarg1, th);
			if (iarg1 != -1 && p->type == &th->runtime->ObjectType) {
				YObject* obj = th->runtime->newObject((YObject*) p, th);
				SET_REGISTER((YValue*) obj, iarg0, th);
			} else
				SET_REGISTER((YValue*) th->runtime->newObject(NULL, th), iarg0,
						th);
		}
			break;
		case VM_NewArray: {
			/*Create empty array*/
			SET_REGISTER((YValue*) newArray(th), iarg0, th);
		}
			break;
		case VM_NewLambda: {
			/*Create lambda. Lambda signature is stored in stack.
			 * It is popped and formed as signature*/
			// Check if lambda is vararg
			YValue* vmeth = pop(th);
			bool meth =
					(vmeth->type == &th->runtime->BooleanType) ?
							((YBoolean*) vmeth)->value : false;
			YValue* vvararg = pop(th);
			bool vararg =
					(vvararg->type == &th->runtime->BooleanType) ?
							((YBoolean*) vvararg)->value : false;
			// Get argument count and types
			size_t argc = (size_t) popInt(th);
			int32_t* argids = calloc(1, sizeof(int32_t) * argc);
			YoyoType** argTypes = calloc(1, sizeof(YoyoType*) * argc);
			for (size_t i = argc - 1; i < argc; i--) {
				argids[i] = (int32_t) popInt(th);
				YValue* val = pop(th);
				if (val->type == &th->runtime->DeclarationType)
					argTypes[i] = (YoyoType*) val;
				else
					argTypes[i] = val->type->TypeConstant;
			}
			// Get lambda return type
			YValue* retV = pop(th);
			YoyoType* retType = NULL;
			if (retV->type == &th->runtime->DeclarationType)
				retType = (YoyoType*) retV;
			else
				retType = retV->type->TypeConstant;
			// Get lambda scope from argument and create
			// lambda signature and lambda
			YValue* sp = getRegister(iarg2, th);
			if (sp->type == &th->runtime->ObjectType) {
				YObject* scope = (YObject*) sp;
				YLambda* lmbd = newProcedureLambda(iarg1, bc, scope, argids,
						newLambdaSignature(meth, argc, vararg, argTypes,
								retType, th), th);
				SET_REGISTER((YValue*) lmbd, iarg0, th);
			} else
				SET_REGISTER(getNull(th), iarg0, th);
			// Free allocated resources
			free(argids);
			free(argTypes);
		}
			break;
		case VM_NewOverload: {
			/*Pop lambdas from stack and
			 * create overloaded lambda*/
			// Pop lambda count and lambdas
			size_t count = (size_t) iarg1;
			YLambda** lambdas = malloc(sizeof(YLambda*) * count);
			for (size_t i = 0; i < count; i++) {
				YValue* val = pop(th);
				if (val->type == &th->runtime->LambdaType)
					lambdas[i] = (YLambda*) val;
				else
					lambdas[i] = NULL;
			}
			// If default lambda is defined then get it
			YLambda* defLmbd = NULL;
			if (iarg2 != -1) {
				YValue* val = getRegister(iarg2, th);
				if (val->type == &th->runtime->LambdaType)
					defLmbd = (YLambda*) val;
			}
			// Create overloaded lambda
			SET_REGISTER(
					(YValue*) newOverloadedLambda(lambdas, count, defLmbd, th),
					iarg0, th);

			// Free allocated resources
			free(lambdas);
		}
			break;
		case VM_NewComplexObject: {
			/*Pop mixin objects from stack and create complex object*/
			// Get mixin count and pop mixins
			size_t count = (size_t) iarg2;
			YObject** mixins = malloc(sizeof(YObject*) * count);
			for (size_t i = 0; i < count; i++) {
				YValue* val = pop(th);
				if (val->type == &th->runtime->ObjectType)
					mixins[i] = (YObject*) val;
				else
					mixins[i] = NULL;
			}
			// Get base object
			YValue* basev = getRegister(iarg1, th);
			YObject* base = NULL;
			if (basev->type == &th->runtime->ObjectType)
				base = (YObject*) basev;
			else
				base = th->runtime->newObject(NULL, th);
			// Create complex object and free allocated resources
			SET_REGISTER((YValue*) newComplexObject(base, mixins, count, th),
					iarg0, th);
			free(mixins);
		}
			break;

		case VM_GetField: {
			/*Read value property and store it in register*/
			YValue* val = getRegister(iarg1, th);
			if (val->type->oper.readProperty != NULL) {
				SET_REGISTER(val->type->oper.readProperty(iarg2, val, th), iarg0,
						th);
			} else
				SET_REGISTER(getNull(th), iarg0, th);
		}
			break;
		case VM_SetField: {
			/*Set objects field*/
			YValue* val = getRegister(iarg0, th);
			if (val->type == &th->runtime->ObjectType) {
				YObject* obj = (YObject*) val;
				obj->put(obj, iarg1, getRegister(iarg2, th), false, th);
			}
		}
			break;
		case VM_NewField: {
			/*Create new field in object*/
			YValue* val = getRegister(iarg0, th);
			if (val->type == &th->runtime->ObjectType) {
				YObject* obj = (YObject*) val;
				obj->put(obj, iarg1, getRegister(iarg2, th), true, th);
			}
		}
			break;
		case VM_DeleteField: {
			/*Delete field from object*/
			YValue* val = getRegister(iarg0, th);
			if (val->type == &th->runtime->ObjectType) {
				YObject* obj = (YObject*) val;
				obj->remove(obj, iarg1, th);
			}
		}
			break;
		case VM_ArrayGet: {
			/*Get index from value. If can't then throw exception*/
			YValue* val = getRegister(iarg1, th);
			YValue* val2 = getRegister(iarg2, th);
			// If value is array, but index is integer then
			// reads array element at index
			if (val->type == &th->runtime->ArrayType && val2->type == &th->runtime->IntType) {
				YArray* arr = (YArray*) val;
				size_t index = (size_t) ((YInteger*) val2)->value;
				SET_REGISTER(arr->get(arr, index, th), iarg0, th);
			} else if (val->type->oper.readIndex != NULL) {
				// Else calls readIndex on type(if readIndex is defined)
				SET_REGISTER(val->type->oper.readIndex(val, val2, th), iarg0,
						th);
			} else {
				throwException(L"AccesingNotAnArray", NULL, 0, th);
				SET_REGISTER(getNull(th), iarg0, th);
			}
		}
			break;
		case VM_ArraySet: {
			/*Set value to other value on index. If can't throw exception*/
			YValue* val = getRegister(iarg0, th);
			YValue* val2 = getRegister(iarg1, th);
			// If value if array, but index is integer
			// then assigns value to an array
			if (val->type == &th->runtime->ArrayType && val2->type == &th->runtime->IntType) {
				YArray* arr = (YArray*) val;
				size_t index = (size_t) ((YInteger*) val2)->value;
				arr->set(arr, index, getRegister(iarg2, th), th);
			} else if (val->type->oper.readIndex != NULL) {
				// Else calls writeIndex on type(if writeIndex is defined)
				val->type->oper.writeIndex(val, val2, getRegister(iarg2, th),
						th);
			} else {
				throwException(L"ModifyingNotAnArray", NULL, 0, th);
			}
		}
			break;
		case VM_ArrayDelete: {
			/*If value is array but index is integer set array index
			 * else throw an exception*/
			YValue* val = getRegister(iarg0, th);
			YValue* val2 = getRegister(iarg1, th);
			if (val->type == &th->runtime->ArrayType && val2->type == &th->runtime->IntType) {
				YArray* arr = (YArray*) val;
				size_t index = (size_t) ((YInteger*) val2)->value;
				arr->remove(arr, index, th);
			} else if (val->type->oper.removeIndex !=NULL) {
				val->type->oper.removeIndex(val, val2, th);
			} else {
				throwException(L"ModifyingNotAnArray", NULL, 0, th);
			}

		}
			break;

		case VM_Goto: {
			/*Get label id from argument, get label address and jump*/
			uint32_t addr = frame->proc->getLabel(frame->proc, iarg0)->value;
			frame->pc = addr;
			continue;
		}
			break;
		case VM_GotoIfTrue: {
			/*Get label id from argument, get label address and jump
			 * if condition is true*/
			YValue* bln = getRegister(iarg1, th);
			if (bln->type == &th->runtime->BooleanType && ((YBoolean*) bln)->value) {
				uint32_t addr = frame->proc->getLabel(frame->proc, iarg0)->value;
				frame->pc = addr;
				continue;
			}
		}
			break;
		case VM_GotoIfFalse: {
			/*Get label id from argument, get label address and jump
			 * if condition is false*/
			YValue* bln = getRegister(iarg1, th);
			if (bln->type == &th->runtime->BooleanType && !((YBoolean*) bln)->value) {
				uint32_t addr = frame->proc->getLabel(frame->proc, iarg0)->value;
				frame->pc = addr;
				continue;
			}
		}
			break;
		case VM_Jump: {
			/*Goto to an address*/
			frame->pc = iarg0;
			continue;
		}
			break;
		case VM_JumpIfTrue: {
			/*Goto to an address if condition is true*/
			YValue* bln = getRegister(iarg1, th);
			if (bln->type == &th->runtime->BooleanType && ((YBoolean*) bln)->value) {
				frame->pc = iarg0;
				continue;
			}
		}
			break;
		case VM_JumpIfFalse: {
			/*Goto to an address if condition is false*/
			YValue* bln = getRegister(iarg1, th);
			if (bln->type == &th->runtime->BooleanType && !((YBoolean*) bln)->value) {
				frame->pc = iarg0;
				continue;
			}
		}
			break;

		case VM_Throw: {
			/*Throw an exception*/
			th->exception = newException(getRegister(iarg0, th), th);
		}
			break;
		case VM_Catch: {
			/*Save current exception in register and
			 * set exception NULL*/
			SET_REGISTER(th->exception, iarg0, th);
			th->exception = NULL;
		}
			break;
		case VM_OpenCatch: {
			/*Add next catch address to frame catch stack.
			 * If exception is thrown then catch address being
			 * popped from stack and interpreter
			 * jump to it*/
			CatchBlock* cb = malloc(sizeof(CatchBlock));
			cb->prev = frame->catchBlock;
			cb->pc = iarg0;
			frame->catchBlock = cb;
		}
			break;
		case VM_CloseCatch: {
			/*Remove catch address from frame catch stack*/
			CatchBlock* cb = frame->catchBlock;
			frame->catchBlock = cb->prev;
			free(cb);
		}
			break;

		case VM_Nop: {
			/*Does nothing*/
		}
			break;

		case VM_Swap: {
			/*Swap two register values*/
			YValue* r1 = getRegister(iarg0, th);
			YValue* r2 = getRegister(iarg1, th);
			SET_REGISTER(r1, iarg1, th);
			SET_REGISTER(r2, iarg0, th);
		}
			break;
		case VM_Subsequence: {
			/*Get subsequence from value if subseq method
			 * is defined*/
			YValue* reg = getRegister(iarg0, th);
			YValue* tfrom = getRegister(iarg1, th);
			YValue* tto = getRegister(iarg2, th);
			if (tfrom->type == &th->runtime->IntType&&
				tto->type == &th->runtime->IntType&&
				reg->type->oper.subseq!=NULL) {
				size_t from = (size_t) ((YInteger*) tfrom)->value;
				size_t to = (size_t) ((YInteger*) tto)->value;
				SET_REGISTER(reg->type->oper.subseq(reg, from, to, th), iarg0,
						th);
			} else
				SET_REGISTER(getNull(th), iarg0, th);
		}
			break;
		case VM_Iterator: {
			/*Get iterator from value if it is iterable*/
			YValue* v = getRegister(iarg1, th);
			if (v->type->oper.iterator != NULL) {
				SET_REGISTER((YValue*) v->type->oper.iterator(v, th), iarg0, th);\
			}
			else {
				SET_REGISTER(getNull(th), iarg0, th);
			}

		}
			break;
		case VM_Iterate: {
			/*If iterator has next value than get it and save
			 * to register. If iterator doesn't has next value
			 * then jump to a label*/
			YValue* v = getRegister(iarg1, th);
			YValue* value = NULL;
			if (v->type->oper.iterator != NULL) {
				YoyoIterator* iter = v->type->oper.iterator(v, th);
				if (iter->hasNext(iter, th))
					value = iter->next(iter, th);
			}
			if (value == NULL) {
				uint32_t addr = frame->proc->getLabel(frame->proc, iarg2)->value;
				frame->pc = addr;
			} else
				SET_REGISTER(value, iarg0, th);
		}
			break;
		case VM_NewInterface: {
			/*Build new interface*/
			YoyoAttribute* attrs = calloc(1, sizeof(YoyoAttribute) * iarg2);
			// Pop interface parents
			YoyoInterface** parents = calloc(1, sizeof(YoyoInterface*) * iarg1);
			for (int32_t i = 0; i < iarg1; i++) {
				YValue* val = pop(th);
				YoyoType* type = NULL;
				if (val->type == &th->runtime->DeclarationType)
					type = (YoyoType*) val;
				else
					type = val->type->TypeConstant;
				parents[i] =
						type->type == InterfaceDT ?
								(YoyoInterface*) type : NULL;
			}
			// Pop interface fields
			for (int32_t i = 0; i < iarg2; i++) {
				attrs[i].id = popInt(th);
				YValue* val = pop(th);
				YoyoType* type = NULL;
				if (val->type == &th->runtime->DeclarationType)
					type = (YoyoType*) val;
				else
					type = val->type->TypeConstant;
				attrs[i].type = type;
			}
			// Build interface and free allocated resources
			SET_REGISTER(
					(YValue*) newInterface(parents, (size_t) iarg1, attrs,
							(size_t) iarg2, th), iarg0, th);
			free(attrs);
			free(parents);
		}
			break;
		case VM_ChangeType: {
			/*Change field type*/
			YValue* val = getRegister(iarg2, th);
			YoyoType* type = NULL;
			if (val->type == &th->runtime->DeclarationType)
				type = (YoyoType*) val;
			else
				type = val->type->TypeConstant;
			YValue* o = getRegister(iarg0, th);
			if (o->type == &th->runtime->ObjectType) {
				YObject* obj = (YObject*) o;
				obj->setType(obj, iarg1, type, th);
			}
		}
			break;
		case VM_GotoIfEquals: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			int cmp = v1->type->oper.compare(v1, v2, th);
			if ((cmp & COMPARE_EQUALS) != 0) {
				uint32_t addr = frame->proc->getLabel(frame->proc, iarg0)->value;
				frame->pc = addr;
				continue;
			}
		}
		break;
		case VM_JumpIfEquals: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			int cmp = v1->type->oper.compare(v1, v2, th);
			if ((cmp & COMPARE_EQUALS) != 0) {
				frame->pc = iarg0;
				continue;
			}
		}
		break;
		case VM_GotoIfNotEquals: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			int cmp = v1->type->oper.compare(v1, v2, th);
			if ((cmp & COMPARE_NOT_EQUALS) != 0) {
				uint32_t addr = frame->proc->getLabel(frame->proc, iarg0)->value;
				frame->pc = addr;
				continue;
			}
		}
		break;
		case VM_JumpIfNotEquals: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			int cmp = v1->type->oper.compare(v1, v2, th);
			if ((cmp & COMPARE_NOT_EQUALS) != 0) {
				frame->pc = iarg0;
				continue;
			}
		}
		break;
		case VM_GotoIfGreater: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			int cmp = v1->type->oper.compare(v1, v2, th);
			if ((cmp & COMPARE_GREATER) != 0) {
				uint32_t addr = frame->proc->getLabel(frame->proc, iarg0)->value;
				frame->pc = addr;
				continue;
			}
		}
		break;
		case VM_JumpIfGreater: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			int cmp = v1->type->oper.compare(v1, v2, th);
			if ((cmp & COMPARE_GREATER) != 0) {
				frame->pc = iarg0;
				continue;
			}
		}
		break;
		case VM_GotoIfLesser: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			int cmp = v1->type->oper.compare(v1, v2, th);
			if ((cmp & COMPARE_LESSER) != 0) {
				uint32_t addr = frame->proc->getLabel(frame->proc, iarg0)->value;
				frame->pc = addr;
				continue;
			}
		}
		break;
		case VM_JumpIfLesser: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			int cmp = v1->type->oper.compare(v1, v2, th);
			if ((cmp & COMPARE_LESSER) != 0) {
				frame->pc = iarg0;
				continue;
			}
		}
		break;
		case VM_GotoIfNotLesser: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			int cmp = v1->type->oper.compare(v1, v2, th);
			if ((cmp & COMPARE_GREATER_OR_EQUALS) != 0) {
				uint32_t addr = frame->proc->getLabel(frame->proc, iarg0)->value;
				frame->pc = addr;
				continue;
			}
		}
		break;
		case VM_JumpIfNotLesser: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			int cmp = v1->type->oper.compare(v1, v2, th);
			if ((cmp & COMPARE_GREATER_OR_EQUALS) != 0) {
				frame->pc = iarg0;
				continue;
			}
		}
		break;
		case VM_GotoIfNotGreater: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			int cmp = v1->type->oper.compare(v1, v2, th);
			if ((cmp & COMPARE_LESSER_OR_EQUALS) != 0) {
				uint32_t addr = frame->proc->getLabel(frame->proc, iarg0)->value;
				frame->pc = addr;
				continue;
			}
		}
		break;
		case VM_JumpIfNotGreater: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			int cmp = v1->type->oper.compare(v1, v2, th);
			if ((cmp & COMPARE_LESSER_OR_EQUALS) != 0) {
				frame->pc = iarg0;
				continue;
			}
		}
		break;
		case VM_CheckType: {
			YValue* value = getRegister(iarg0, th);
			YValue* tv = getRegister(iarg1, th);
			YoyoType* type = NULL;
			if (tv->type == &th->runtime->DeclarationType)
				type = (YoyoType*) tv;
			else
				type = tv->type->TypeConstant;
			if (!type->verify(type, value, th)) {
				wchar_t* wcs = getSymbolById(&th->runtime->symbols, iarg2);
				throwException(L"WrongFieldType", &wcs, 1, th);
			}
		}
		break;
		}
		/*If there is an exception then get last catch block
		 * and jump to an address. If catch stack is empty
		 * then return from procedure*/
		if (th->exception != NULL) {
			if (frame->catchBlock != NULL) {
				frame->pc = frame->proc->getLabel(frame->proc,
						frame->catchBlock->pc)->value;
				continue;
			} else
				return getNull(th);
		}
		frame->pc += 13;
	}
	return getNull(th);
}
#endif
