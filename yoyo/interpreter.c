/*
 * Copyright (C) 2016  Jevgenijs Protopopovs <protopopov1122@yandex.ru>
 */
/*This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "yoyo/interpreter.h"
#include "yoyo/opcodes.h"

/*This file contains procedures to interpret bytecode.*/

/*Some useful procedures for interpreter*/
/*Assign certain register value. If value is NULL then
 * assigned is Null.*/
void setRegister(YValue* v, size_t reg, YThread* th) {
	if (reg < th->frame->regc)
		th->frame->regs[reg] = v!=NULL ? v : getNull(th);
}
/*Push value to frames stack. If stack is full it's being reallocated.*/
void push(YValue* v, YThread* th) {
	ExecutionFrame* frame = th->frame;
	if (frame->stack_offset + 1 >= frame->stack_size) {
		frame->stack_size += 10;
		frame->stack = realloc(frame->stack,
				sizeof(YValue*) * (frame->stack_size));
	}
	frame->stack[frame->stack_offset++] = v;
}
/*Return value popped from stack or Null if stack is empty.*/
YValue* pop(YThread* th) {
	ExecutionFrame* frame = th->frame;
	if (frame->stack_offset != 0)
		return frame->stack[--frame->stack_offset];
	else
		return getNull(th);
}
/*Pop value from stack. If it's integer return it, else return 0*/
int64_t popInt(YThread* th) {
	YValue* v = pop(th);
	if (v->type->type == IntegerT)
		return ((YInteger*) v)->value;
	else
		return 0;
}

/*Initialize execution frame, assign it to thread and call execute method on it*/
YValue* invoke(int32_t procid, YObject* scope, YoyoType* retType, YThread* th) {
	ExecutionFrame frame;

	// Init execution frame
	ILProcedure* proc = th->runtime->bytecode->procedures[procid];
	frame.proc = proc;
	frame.retType = retType;
	frame.regc = proc->regc;
	frame.regs = malloc(sizeof(YValue*) * frame.regc);
	for (size_t i = 1; i < frame.regc; i++)
		frame.regs[i] = getNull(th);
	frame.regs[0] = (YValue*) scope;
	frame.pc = 0;
	frame.stack_size = 10;
	frame.stack_offset = 0;
	frame.stack = malloc(sizeof(YValue*) * frame.stack_size);
	frame.catchBlock = NULL;
	frame.debug_ptr = NULL;
	frame.debug_field = -1;
	frame.debug_flags = 0;
	YBreakpoint bp;
	frame.breakpoint = &bp;

	// Assign execution frame to thread
	frame.prev = th->frame;
	th->frame = &frame;
	// Call debugger if nescesarry
	if (frame.prev == NULL && th->type == Normal)
		DEBUG(th->runtime->debugger, interpret_start, &procid, th);
	DEBUG(th->runtime->debugger, enter_function, frame.prev, th);

	YValue* out = execute(th);	// Finally execute
	frame.pc = 0;	// For debug

	// Call debugger if nescesarry
	if (frame.prev == NULL && th->type == Normal)
		DEBUG(th->runtime->debugger, interpret_end, &procid, th);

	// Free resources and remove frame from stack
	free(frame.regs);
	free(frame.stack);
	th->frame = frame.prev;

	return out;
}

/*Procedure that interprets current frame bytecode
 * Uses loop with switch statement.*/
YValue* execute(YThread* th) {
	YRuntime* runtime = th->runtime;
	ILBytecode* bc = runtime->bytecode;
	ExecutionFrame* frame = th->frame;

	while (frame->pc + 13 <= frame->proc->code_length) {
		// If runtime is paused then execution should be paused too.
		if (runtime->state == RuntimePaused) {
			th->state = Paused;
			while (runtime->state == RuntimePaused)
				YIELD();
			th->state = RuntimeRunning;
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
				setRegister(cpool->get(cpool, iarg1, th), iarg0, th);
				break;
			}
			/*Else it is created, added to pool and returned*/
			Constant* cnst = bc->getConstant(bc, iarg1);
			YValue* val = getNull(th);
			if (cnst != NULL) {
				switch (cnst->type) {
				case IntegerC:
					val = newInteger(((IntConstant*) cnst)->value, th);
					break;
				case FloatC:
					val = newFloat(((FloatConstant*) cnst)->value, th);
					break;
				case StringC:
					val = newString(
							bc->getSymbolById(bc,
									((StringConstant*) cnst)->value), th);
					break;
				case BooleanC:
					val = newBoolean(((BooleanConstant*) cnst)->value, th);
					break;
				default:
					break;
				}
			}
			cpool->put(cpool, iarg1, val, true, th);
			setRegister(val, iarg0, th);
		}
			break;
		case VM_LoadInteger: {
			/*Load integer from argument directly to register*/
			YValue* val = newInteger(iarg1, th);
			setRegister(val, iarg0, th);
		}
			break;
		case VM_Copy: {
			/*Copy register value to another*/
			setRegister(getRegister(iarg1, th), iarg0, th);
		}
			break;
		case VM_Push: {
			/*Push register value to stack*/
			push(getRegister(iarg0, th), th);
		}
			break;

			/*Next instructions load values from two registers,
			 * perform polymorph binary operation and
			 * save result to third register*/
		case VM_Add: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			setRegister(v1->type->oper.add_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_Subtract: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			setRegister(v1->type->oper.subtract_operation(v1, v2, th), iarg0,
					th);
		}
			break;
		case VM_Multiply: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			setRegister(v1->type->oper.multiply_operation(v1, v2, th), iarg0,
					th);
		}
			break;
		case VM_Divide: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			setRegister(v1->type->oper.divide_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_Modulo: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			setRegister(v1->type->oper.modulo_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_Power: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			setRegister(v1->type->oper.power_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_ShiftRight: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			setRegister(v1->type->oper.shr_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_ShiftLeft: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			setRegister(v1->type->oper.shl_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_And: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			setRegister(v1->type->oper.and_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_Or: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			setRegister(v1->type->oper.or_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_Xor: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);
			setRegister(v1->type->oper.xor_operation(v1, v2, th), iarg0, th);
		}
			break;
		case VM_Compare: {
			YValue* v1 = getRegister(iarg1, th);
			YValue* v2 = getRegister(iarg2, th);

			setRegister(newInteger(v1->type->oper.compare(v1, v2, th), th),
					iarg0, th);
		}
			break;

		case VM_Test: {
			/*Take an integer from register and
			 * check if certain bit is 1 or 0*/
			YValue* arg = getRegister(iarg1, th);
			if (arg->type->type == IntegerT) {
				int64_t i = ((YInteger*) arg)->value;
				YValue* res = newBoolean((i & iarg2) != 0, th);
				setRegister(res, iarg0, th);
			} else
				setRegister(getNull(th), iarg0, th);
		}
			break;
			/*These instruction perform polymorph unary operations*/
		case VM_Negate: {
			YValue* v1 = getRegister(iarg1, th);
			setRegister(v1->type->oper.negate_operation(v1, th), iarg0, th);
		}
			break;
		case VM_Not: {
			YValue* v1 = getRegister(iarg1, th);
			setRegister(v1->type->oper.not_operation(v1, th), iarg0, th);
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
			if (val->type->type == LambdaT) {

				YLambda* l = (YLambda*) val;
				setRegister(invokeLambda(l, args, argc, th), iarg0, th);
			} else {
				throwException(L"CallingNotALambda", NULL, 0, th);
				setRegister(getNull(th), iarg0, th);
			}
			free(args);
		}
			break;
		case VM_Return: {
			/*Verify register value to be some type(if defined)
			 * and return it. Execution has been ended*/
			YValue* ret = getRegister(iarg0, th);
			if (th->frame->retType != NULL
					&& !th->frame->retType->verify(th->frame->retType, ret,
							th)) {
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
			if (iarg1 != -1 && p->type->type == ObjectT) {
				YObject* obj = th->runtime->newObject((YObject*) p, th);
				setRegister((YValue*) obj, iarg0, th);
			} else
				setRegister((YValue*) th->runtime->newObject(NULL, th), iarg0,
						th);
		}
			break;
		case VM_NewArray: {
			/*Create empty array*/
			setRegister((YValue*) newArray(th), iarg0, th);
		}
			break;
		case VM_NewLambda: {
			/*Create lambda. Lambda signature is stored in stack.
			 * It is popped and formed as signature*/
			// Check if lambda is vararg
			YValue* vvararg = pop(th);
			bool vararg =
					(vvararg->type->type == BooleanT) ?
							((YBoolean*) vvararg)->value : false;
			// Get argument count and types
			size_t argc = (size_t) popInt(th);
			int32_t* argids = calloc(1, sizeof(int32_t) * argc);
			YoyoType** argTypes = calloc(1, sizeof(YoyoType*) * argc);
			for (size_t i = argc - 1; i < argc; i--) {
				argids[i] = (int32_t) popInt(th);
				YValue* val = pop(th);
				if (val->type->type == DeclarationT)
					argTypes[i] = (YoyoType*) val;
				else
					argTypes[i] = val->type->TypeConstant;
			}
			// Get lambda return type
			YValue* retV = pop(th);
			YoyoType* retType = NULL;
			if (retV->type->type == DeclarationT)
				retType = (YoyoType*) retV;
			else
				retType = retV->type->TypeConstant;
			// Get lambda scope from argument and create
			// lambda signature and lambda
			YValue* sp = getRegister(iarg2, th);
			if (sp->type->type == ObjectT) {
				YObject* scope = (YObject*) sp;
				YLambda* lmbd = newProcedureLambda(iarg1, scope, argids,
						newLambdaSignature(argc, vararg, argTypes, retType, th),
						th);
				setRegister((YValue*) lmbd, iarg0, th);
			} else
				setRegister(getNull(th), iarg0, th);
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
				if (val->type->type == LambdaT)
					lambdas[i] = (YLambda*) val;
				else
					lambdas[i] = NULL;
			}
			// If default lambda is defined then get it
			YLambda* defLmbd = NULL;
			if (iarg2 != -1) {
				YValue* val = getRegister(iarg2, th);
				if (val->type->type == LambdaT)
					defLmbd = (YLambda*) val;
			}
			// Create overloaded lambda
			setRegister(
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
				if (val->type->type == ObjectT)
					mixins[i] = (YObject*) val;
				else
					mixins[i] = NULL;
			}
			// Get base object
			YValue* basev = getRegister(iarg1, th);
			YObject* base = NULL;
			if (basev->type->type == ObjectT)
				base = (YObject*) basev;
			else
				base = th->runtime->newObject(NULL, th);
			// Create complex object and free allocated resources
			setRegister((YValue*) newComplexObject(base, mixins, count, th),
					iarg0, th);
			free(mixins);
		}
			break;

		case VM_GetField: {
			/*Read value property and store it in register*/
			YValue* val = getRegister(iarg1, th);
			if (val->type->oper.readProperty != NULL) {
				setRegister(val->type->oper.readProperty(iarg2, val, th), iarg0,
						th);
			} else
				setRegister(getNull(th), iarg0, th);
		}
			break;
		case VM_SetField: {
			/*Set objects field*/
			YValue* val = getRegister(iarg0, th);
			if (val->type->type == ObjectT) {
				YObject* obj = (YObject*) val;
				obj->put(obj, iarg1, getRegister(iarg2, th), false, th);
			}
		}
			break;
		case VM_NewField: {
			/*Create new field in object*/
			YValue* val = getRegister(iarg0, th);
			if (val->type->type == ObjectT) {
				YObject* obj = (YObject*) val;
				obj->put(obj, iarg1, getRegister(iarg2, th), true, th);
			}
		}
			break;
		case VM_DeleteField: {
			/*Delete field from object*/
			YValue* val = getRegister(iarg0, th);
			if (val->type->type == ObjectT) {
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
			if (val->type->type == ArrayT && val2->type->type == IntegerT) {
				YArray* arr = (YArray*) val;
				size_t index = (size_t) ((YInteger*) val2)->value;
				setRegister(arr->get(arr, index, th), iarg0, th);
			} else if (val->type->oper.readIndex != NULL) {
				// Else calls readIndex on type(if readIndex is defined)
				setRegister(val->type->oper.readIndex(val, val2, th), iarg0,
						th);
			} else {
				throwException(L"AccesingNotAnArray", NULL, 0, th);
				setRegister(getNull(th), iarg0, th);
			}
		}
			break;
		case VM_ArraySet: {
			/*Set value to other value on index. If can't throw exception*/
			YValue* val = getRegister(iarg0, th);
			YValue* val2 = getRegister(iarg1, th);
			// If value if array, but index is integer
			// then assigns value to an array
			if (val->type->type == ArrayT && val2->type->type == IntegerT) {
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
			if (val->type->type == ArrayT && val2->type->type == IntegerT) {
				YArray* arr = (YArray*) val;
				size_t index = (size_t) ((YInteger*) val2)->value;
				arr->remove(arr, index, th);
			} else {
				throwException(L"ModifyingNotAnArray", NULL, 0, th);
			}

		}
			break;

		case VM_Jump: {
			/*Get label id from argument, get label address and jump*/
			uint32_t addr = frame->proc->getLabel(frame->proc, iarg0)->value;
			frame->pc = addr;
			continue;
		}
			break;
		case VM_JumpIfTrue: {
			/*Get label id from argument, get label address and jump
			 * if condition is true*/
			YValue* bln = getRegister(iarg1, th);
			if (bln->type->type == BooleanT && ((YBoolean*) bln)->value) {
				uint32_t addr = frame->proc->getLabel(frame->proc, iarg0)->value;
				frame->pc = addr;
				continue;
			}
		}
			break;
		case VM_JumpIfFalse: {
			/*Get label id from argument, get label address and jump
			 * if condition is false*/
			YValue* bln = getRegister(iarg1, th);
			if (bln->type->type == BooleanT && !((YBoolean*) bln)->value) {
				uint32_t addr = frame->proc->getLabel(frame->proc, iarg0)->value;
				frame->pc = addr;
				continue;
			}
		}
			break;
		case VM_DirectJump: {
			/*Jump to an address*/
			frame->pc = iarg0;
			continue;
		}
			break;
		case VM_DirectJumpIfTrue: {
			/*Jump to an address if condition is true*/
			YValue* bln = getRegister(iarg1, th);
			if (bln->type->type == BooleanT && ((YBoolean*) bln)->value) {
				frame->pc = iarg0;
				continue;
			}
		}
			break;
		case VM_DirectJumpIfFalse: {
			/*Jump to an address if condition is false*/
			YValue* bln = getRegister(iarg1, th);
			if (bln->type->type == BooleanT && !((YBoolean*) bln)->value) {
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
			setRegister(th->exception, iarg0, th);
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
			setRegister(r1, iarg1, th);
			setRegister(r2, iarg0, th);
		}
			break;
		case VM_Subsequence: {
			/*Get subsequence from value if subseq method
			 * is defined*/
			YValue* reg = getRegister(iarg0, th);
			YValue* tfrom = getRegister(iarg1, th);
			YValue* tto = getRegister(iarg2, th);
			if (tfrom->type->type == IntegerT&&
			tto->type->type==IntegerT&&
			reg->type->oper.subseq!=NULL) {
				size_t from = (size_t) ((YInteger*) tfrom)->value;
				size_t to = (size_t) ((YInteger*) tto)->value;
				setRegister(reg->type->oper.subseq(reg, from, to, th), iarg0,
						th);
			} else
				setRegister(getNull(th), iarg0, th);
		}
			break;
		case VM_Iterator: {
			/*Get iterator from value if it is iterable*/
			YValue* v = getRegister(iarg1, th);
			if (v->type->oper.iterator != NULL)
				setRegister((YValue*) v->type->oper.iterator(v, th), iarg0, th);
			else
				setRegister(getNull(th), iarg0, th);
		}
			break;
		case VM_Iterate: {
			/*If iterator has next value than get it and save
			 * to register. If iterator doesn't has next value
			 * then jump to a label*/
			YValue* v = getRegister(iarg1, th);
			YValue* value = NULL;
			if (v->type->type == ObjectT && ((YObject*) v)->iterator) {
				YoyoIterator* iter = (YoyoIterator*) v;
				if (iter->hasNext(iter, th))
					value = iter->next(iter, th);
			}
			if (value == NULL) {
				uint32_t addr = frame->proc->getLabel(frame->proc, iarg2)->value;
				frame->pc = addr;
			} else
				setRegister(value, iarg0, th);
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
				if (val->type->type == DeclarationT)
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
				if (val->type->type == DeclarationT)
					type = (YoyoType*) val;
				else
					type = val->type->TypeConstant;
				attrs[i].type = type;
			}
			// Build interface and free allocated resources
			setRegister(
					(YValue*) newInterface(parents, (size_t) iarg1, attrs,
							(size_t) iarg2, th), iarg0, th);
			free(attrs);
			free(parents);
		}
			break;
		case VM_ChType: {
			/*Change field type*/
			YValue* val = getRegister(iarg2, th);
			YoyoType* type = NULL;
			if (val->type->type == DeclarationT)
				type = (YoyoType*) val;
			else
				type = val->type->TypeConstant;
			YValue* o = getRegister(iarg0, th);
			if (o->type->type == ObjectT) {
				YObject* obj = (YObject*) o;
				obj->setType(obj, iarg1, type, th);
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
