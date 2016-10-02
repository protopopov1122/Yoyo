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

#include "yoyoc.h"
#include "codegen.h"
#include "opcodes.h"

#define GET_BIT(n, offset) ((n>>offset)&1)

void ProcedureBuilder_append(ProcedureBuilder* proc, uint8_t opcode,
		int32_t iarg1, int32_t iarg2, int32_t iarg3) {
	proc->proc->appendCode(proc->proc, &opcode, 1);
	union {
		int32_t i32;
		uint8_t raw[4];
	} un;
	un.i32 = iarg1;
	proc->proc->appendCode(proc->proc, un.raw, 4);
	un.i32 = iarg2;
	proc->proc->appendCode(proc->proc, un.raw, 4);
	un.i32 = iarg3;
	proc->proc->appendCode(proc->proc, un.raw, 4);
}
int32_t ProcedureBuilder_nextLabel(ProcedureBuilder* proc) {
	return proc->nLabel++;
}
int32_t ProcedureBuilder_nextRegister(ProcedureBuilder* proc) {
	int32_t out = -1;
	for (size_t i = 0; i < proc->regs_length; i++) {
		if (out != -1)
			break;
		for (size_t j = 0; j < sizeof(uint8_t) * 8; j++)

			if (GET_BIT(proc->regs[i], j) == 0) {
				proc->regs[i] |= 1 << j;
				out = i * sizeof(uint8_t) * 8 + j;
				break;
			}
	}
	if (out == -1) {
		proc->regs_length++;
		proc->regs = realloc(proc->regs, sizeof(uint8_t) * proc->regs_length);
		proc->regs[proc->regs_length - 1] = 1;
		out = (proc->regs_length - 1) * sizeof(uint8_t) * 8;
	}
	if (out >= proc->proc->regc)
		proc->proc->regc = out + 1;
	return out;
}
void ProcedureBuilder_unuse(ProcedureBuilder* proc, int32_t reg) {
	if (reg < 0)
		return;
	size_t offset = reg % (sizeof(uint8_t) * 8);
	size_t block = (reg - offset) / (sizeof(uint8_t) * 8);
	if (block < proc->regs_length) {
		proc->regs[block] ^= 1 << offset;
	}
}
void ProcedureBuilder_putLabel(ProcedureBuilder* proc, int32_t id) {
	proc->proc->addLabel(proc->proc, id, proc->proc->code_length);
}
ProcdeureBuilderLoop* ProcedureBuilder_startLoop(ProcedureBuilder* proc,
		int32_t id, int32_t startL, int32_t endL) {
	ProcdeureBuilderLoop* loop = malloc(sizeof(ProcdeureBuilderLoop));
	loop->prev = proc->loop;
	loop->start = startL;
	loop->end = endL;
	loop->id = id;
	proc->loop = loop;
	return loop;
}
void ProcedureBuilder_endLoop(ProcedureBuilder* proc) {
	ProcdeureBuilderLoop* loop = proc->loop;
	proc->loop = loop->prev;
	free(loop);
}
ProcdeureBuilderLoop* ProcedureBuilder_getLoop(ProcedureBuilder* proc,
		int32_t id) {
	ProcdeureBuilderLoop* loop = proc->loop;
	while (loop != NULL) {
		if (loop->id == id)
			return loop;
		loop = loop->prev;
	}
	return NULL;
}

LocalVarEntry* Procedure_defineLocalVar(ProcedureBuilder* proc, int32_t id) {
	for (LocalVarEntry* e = proc->local_vars;e!=NULL;e=e->prev)
		if (e->id == id)
			return e;
	LocalVarEntry* e = malloc(sizeof(LocalVarEntry));
	e->id = id;
	e->value_reg = proc->nextRegister(proc);
	e->type_reg = -1;
	e->prev = proc->local_vars;
	proc->local_vars = e;
	return e;
}

LocalVarEntry* Procedure_getLocalVar(ProcedureBuilder* proc, int32_t id) {
	for (LocalVarEntry* e = proc->local_vars;e!=NULL;e=e->prev)
		if (e->id == id)
			return e;
	return NULL;
}

ProcedureBuilder* YCodeGen_newProcedure(YCodeGen* builder) {
	ProcedureBuilder* proc = malloc(sizeof(ProcedureBuilder));
	proc->prev = builder->proc;
	builder->proc = proc;
	proc->proc = builder->bc->newProcedure(builder->bc);

	proc->nLabel = 0;
	proc->regs = malloc(sizeof(uint8_t));
	proc->regs[0] = 1;
	proc->regs_length = 1;
	proc->regc = 0;
	proc->loop = NULL;
	proc->local_vars = NULL;

	proc->nextLabel = ProcedureBuilder_nextLabel;
	proc->nextRegister = ProcedureBuilder_nextRegister;
	proc->unuse = ProcedureBuilder_unuse;
	proc->append = ProcedureBuilder_append;
	proc->bind = ProcedureBuilder_putLabel;
	proc->startLoop = ProcedureBuilder_startLoop;
	proc->endLoop = ProcedureBuilder_endLoop;
	proc->getLoop = ProcedureBuilder_getLoop;
	return proc;
}
void Procedure_preprocess(ProcedureBuilder *proc) {
	size_t offset = 0;
	uint8_t* opcode;
	int32_t* args;
	while (offset + 13 <= proc->proc->code_length) {
		opcode = &proc->proc->code[offset];
		args = (void*) &proc->proc->code[offset + 1];
		switch (*opcode) {
		case VM_Goto:
			*opcode = VM_Jump;
			args[0] = proc->proc->getLabel(proc->proc, args[0])->value;
			break;
		case VM_GotoIfTrue:
			*opcode = VM_JumpIfTrue;
			args[0] = proc->proc->getLabel(proc->proc, args[0])->value;
			break;
		case VM_GotoIfFalse:
			*opcode = VM_JumpIfFalse;
			args[0] = proc->proc->getLabel(proc->proc, args[0])->value;
			break;
		case VM_GotoIfEquals:
			*opcode = VM_JumpIfEquals;
			args[0] = proc->proc->getLabel(proc->proc, args[0])->value;
			break;
		case VM_GotoIfNotEquals:
			*opcode = VM_JumpIfNotEquals;
			args[0] = proc->proc->getLabel(proc->proc, args[0])->value;
			break;
		case VM_GotoIfGreater:
			*opcode = VM_JumpIfGreater;
			args[0] = proc->proc->getLabel(proc->proc, args[0])->value;
			break;
		case VM_GotoIfLesser:
			*opcode = VM_JumpIfLesser;
			args[0] = proc->proc->getLabel(proc->proc, args[0])->value;
			break;
		case VM_GotoIfNotLesser:
			*opcode = VM_JumpIfNotLesser;
			args[0] = proc->proc->getLabel(proc->proc, args[0])->value;
			break;
		case VM_GotoIfNotGreater:
			*opcode = VM_JumpIfNotGreater;
			args[0] = proc->proc->getLabel(proc->proc, args[0])->value;
			break;
		default:
			break;
		}
		offset += 13;
	}
}
void YCodeGen_endProcedure(YCodeGen* builder) {
	if (builder->proc != NULL) {
		optimize_procedure(builder->proc->proc);
			if (builder->preprocess && builder->jit == NULL)
				Procedure_preprocess(builder->proc);
		if (builder->jit != NULL)
  		builder->proc->proc->compiled =
    		builder->jit->compile(builder->jit, builder->proc->proc, builder->bc);
		ProcedureBuilder* proc = builder->proc;
		builder->proc = proc->prev;
		for (LocalVarEntry* e = proc->local_vars;e!=NULL;) {
			LocalVarEntry* prev = e->prev;
			free(e);
			e = prev;
		}
		free(proc->regs);
		free(proc);
	}
}

typedef struct YIdentifierModifier {
	YModifier mod;
	int32_t id;
} YIdentifierModifier;

typedef struct YFieldModifier {
	YModifier mod;
	YNode* object;
	int32_t id;
} YFieldModifier;

typedef struct YArrayModifier {
	YModifier mod;
	YNode* array;
	YNode* index;
} YArrayModifier;

typedef struct YListModifier {
	YModifier mod;
	YNode** list;
	size_t length;
} YListModifier;

typedef struct YConditionalModifier {
	YModifier mod;
	YNode* cond;
	YNode* body;
	YNode* elseBody;
} YConditionalModifier;

int32_t ytranslate(YCodeGen* builder, YoyoCEnvironment* env, YNode* node);

void Identifier_setter(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env,
bool newVar, int32_t val) {
	YIdentifierModifier* mod = (YIdentifierModifier*) m;
	ProcedureBuilder* proc = builder->proc;;
	LocalVarEntry* e = Procedure_getLocalVar(proc, mod->id);
	if (mod->mod.local&&e==NULL) {
		e = Procedure_defineLocalVar(proc, mod->id);
	}
	if (e!=NULL) {
		if (e->type_reg!=-1)
			proc->append(proc, VM_CheckType, val, e->type_reg, mod->id);
		proc->append(proc, VM_Copy, e->value_reg, val, -1);
		return;
	}
	if (newVar)
		proc->append(proc, VM_NewField, 0, mod->id, val);
	else
		proc->append(proc, VM_SetField, 0, mod->id, val);
}
void Identifier_remover(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env) {
	YIdentifierModifier* mod = (YIdentifierModifier*) m;
	ProcedureBuilder* proc = builder->proc;
	proc->append(proc, VM_DeleteField, 0, mod->id, -1);
}
void Identifier_setType(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env,
		int32_t reg) {
	YIdentifierModifier* mod = (YIdentifierModifier*) m;
	ProcedureBuilder* proc = builder->proc;
	LocalVarEntry* e = Procedure_getLocalVar(proc, mod->id);
	if (e!=NULL) {
		if (e->type_reg == -1)
			e->type_reg = proc->nextRegister(proc);
		proc->append(proc, VM_Copy, e->type_reg, reg, -1);
	}
	else {
		proc->append(proc, VM_ChangeType, 0, mod->id, reg);
	}
}

void Field_setter(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env,
bool newVar, int32_t val) {
	YFieldModifier* mod = (YFieldModifier*) m;
	ProcedureBuilder* proc = builder->proc;
	int32_t obj = ytranslate(builder, env, mod->object);
	if (!newVar)
		proc->append(proc, VM_SetField, obj, mod->id, val);
	else
		proc->append(proc, VM_NewField, obj, mod->id, val);
	proc->unuse(proc, obj);
}
void Field_remover(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env) {
	YFieldModifier* mod = (YFieldModifier*) m;
	int32_t obj = ytranslate(builder, env, mod->object);
	ProcedureBuilder* proc = builder->proc;
	proc->append(proc, VM_DeleteField, obj, mod->id, -1);
	proc->unuse(proc, obj);
}
void Field_setType(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env,
		int32_t reg) {
	YFieldModifier* mod = (YFieldModifier*) m;
	int32_t obj = ytranslate(builder, env, mod->object);
	ProcedureBuilder* proc = builder->proc;
	proc->append(proc, VM_ChangeType, obj, mod->id, reg);
	proc->unuse(proc, obj);
}

void Array_setter(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env,
bool newVar, int32_t val) {
	YArrayModifier* mod = (YArrayModifier*) m;
	ProcedureBuilder* proc = builder->proc;
	int32_t arr = ytranslate(builder, env, mod->array);
	int32_t ind = ytranslate(builder, env, mod->index);
	proc->append(proc, VM_ArraySet, arr, ind, val);
	proc->unuse(proc, arr);
	proc->unuse(proc, ind);
}
void Array_remover(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env) {
	YArrayModifier* mod = (YArrayModifier*) m;
	ProcedureBuilder* proc = builder->proc;
	int32_t arr = ytranslate(builder, env, mod->array);
	int32_t ind = ytranslate(builder, env, mod->index);
	proc->append(proc, VM_ArrayDelete, arr, ind, -1);
	proc->unuse(proc, arr);
	proc->unuse(proc, ind);
	return;
}
void Array_setType(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env,
		int32_t reg) {
	return;
}

void List_setter(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env,
bool newVar, int32_t val) {
	YListModifier* mod = (YListModifier*) m;
	ProcedureBuilder* proc = builder->proc;
	int32_t reg = proc->nextRegister(proc);
	for (size_t i = 0; i < mod->length; i++) {
		YModifier* imod = ymodifier(builder, env, mod->list[i], m->local);
		if (imod == NULL) {
			fprintf(env->env.out_stream,
					"Expected modifieable expression at %"PRId32":%"PRId32"\n",
					mod->list[i]->line, mod->list[i]->charPos);
			break;
		}
		proc->append(proc, VM_LoadInteger, reg, i, -1);
		proc->append(proc, VM_ArrayGet, reg, val, reg);
		imod->setter(imod, builder, env, newVar, reg);
		imod->free(imod);
	}
	proc->unuse(proc, reg);
}
void List_remover(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env) {
	return;
}
void List_setType(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env,
		int32_t reg) {
	return;
}

void Conditional_setter(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env,
bool newVar, int32_t val) {
	YConditionalModifier* mod = (YConditionalModifier*) m;
	ProcedureBuilder* proc = builder->proc;

	int32_t cond = ytranslate(builder, env, mod->cond);

	int32_t trueL = proc->nextLabel(proc);
	int32_t falseL = proc->nextLabel(proc);
	proc->append(proc, VM_GotoIfFalse, falseL, cond, -1);
	proc->unuse(proc, cond);
	YModifier* tmod = ymodifier(builder, env, mod->body, m->local);
	if (tmod == NULL) {
		fprintf(env->env.out_stream,
				"Expected modifieable expression at %"PRId32":%"PRId32"\n",
				mod->body->line, mod->body->charPos);
		return;
	}
	tmod->setter(tmod, builder, env, newVar, val);
	tmod->free(tmod);
	proc->append(proc, VM_Goto, trueL, -1, -1);
	proc->bind(proc, falseL);
	tmod = ymodifier(builder, env, mod->elseBody, m->local);
	if (tmod == NULL) {
		fprintf(env->env.out_stream,
				"Expected modifieable expression at %"PRId32":%"PRId32"\n",
				mod->elseBody->line, mod->elseBody->charPos);
		return;
	}
	tmod->setter(tmod, builder, env, newVar, val);
	tmod->free(tmod);
	proc->bind(proc, trueL);
}
void Conditinal_remover(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env) {
	YConditionalModifier* mod = (YConditionalModifier*) m;
	ProcedureBuilder* proc = builder->proc;

	int32_t cond = ytranslate(builder, env, mod->cond);

	int32_t trueL = proc->nextLabel(proc);
	int32_t falseL = proc->nextLabel(proc);
	proc->append(proc, VM_GotoIfFalse, falseL, cond, -1);
	proc->unuse(proc, cond);
	YModifier* tmod = ymodifier(builder, env, mod->body, false);
	if (tmod == NULL) {
		fprintf(env->env.out_stream,
				"Expected modifieable expression at %"PRId32":%"PRId32"\n",
				mod->elseBody->line, mod->elseBody->charPos);
		return;
	}
	tmod->remover(tmod, builder, env);
	tmod->free(tmod);
	proc->append(proc, VM_Goto, trueL, -1, -1);
	proc->bind(proc, falseL);
	tmod = ymodifier(builder, env, mod->elseBody, false);
	if (tmod == NULL) {
		fprintf(env->env.out_stream,
				"Expected modifieable expression at %"PRId32":%"PRId32"\n",
				mod->elseBody->line, mod->elseBody->charPos);
		return;
	}
	tmod->remover(tmod, builder, env);
	tmod->free(tmod);
	proc->bind(proc, trueL);
}
void Conditional_setType(YModifier* m, YCodeGen* builder, YoyoCEnvironment* env,
		int32_t reg) {
	YConditionalModifier* mod = (YConditionalModifier*) m;
	ProcedureBuilder* proc = builder->proc;

	int32_t cond = ytranslate(builder, env, mod->cond);

	int32_t trueL = proc->nextLabel(proc);
	int32_t falseL = proc->nextLabel(proc);
	proc->append(proc, VM_GotoIfFalse, falseL, cond, -1);
	proc->unuse(proc, cond);
	YModifier* tmod = ymodifier(builder, env, mod->body, m->local);
	if (tmod == NULL) {
		fprintf(env->env.out_stream,
				"Expected modifieable expression at %"PRId32":%"PRId32"\n",
				mod->elseBody->line, mod->elseBody->charPos);
		return;
	}
	tmod->typeSetter(tmod, builder, env, reg);
	tmod->free(tmod);
	proc->append(proc, VM_Goto, trueL, -1, -1);
	proc->bind(proc, falseL);
	tmod = ymodifier(builder, env, mod->elseBody, m->local);
	if (tmod == NULL) {
		fprintf(env->env.out_stream,
				"Expected modifieable expression at %"PRId32":%"PRId32"\n",
				mod->elseBody->line, mod->elseBody->charPos);
		return;
	}
	tmod->typeSetter(tmod, builder, env, reg);
	tmod->free(tmod);
	proc->bind(proc, trueL);
}

YModifier* ymodifier(YCodeGen* builder, YoyoCEnvironment* env, YNode* node, bool local) {
	if (node->type == IdentifierReferenceN) {
		YIdentifierReferenceNode* ref = (YIdentifierReferenceNode*) node;
		YIdentifierModifier* mod = malloc(sizeof(YIdentifierModifier));
		mod->id = env->bytecode->getSymbolId(env->bytecode, ref->id);
		mod->mod.setter = Identifier_setter;
		mod->mod.remover = Identifier_remover;
		mod->mod.typeSetter = Identifier_setType;
		mod->mod.free = (void (*)(YModifier*)) free;
		mod->mod.local = local;
		return (YModifier*) mod;
	} else if (node->type == FieldReferenceN) {
		YFieldReferenceNode* ref = (YFieldReferenceNode*) node;
		YFieldModifier* mod = malloc(sizeof(YFieldModifier));
		mod->id = env->bytecode->getSymbolId(env->bytecode, ref->field);
		mod->object = ref->object;
		mod->mod.setter = Field_setter;
		mod->mod.remover = Field_remover;
		mod->mod.typeSetter = Field_setType;
		mod->mod.free = (void (*)(YModifier*)) free;
		mod->mod.local = local;
		return (YModifier*) mod;
	} else if (node->type == ArrayReferenceN) {
		YArrayReferenceNode* ref = (YArrayReferenceNode*) node;
		YArrayModifier* mod = malloc(sizeof(YArrayModifier));
		mod->array = ref->array;
		mod->index = ref->index;
		mod->mod.setter = Array_setter;
		mod->mod.remover = Array_remover;
		mod->mod.typeSetter = Array_setType;
		mod->mod.free = (void (*)(YModifier*)) free;
		mod->mod.local = local;
		return (YModifier*) mod;
	} else if (node->type == FilledArrayN) {
		YFilledArrayNode* farr = (YFilledArrayNode*) node;
		YListModifier* mod = malloc(sizeof(YListModifier));
		mod->list = farr->array;
		mod->length = farr->length;
		mod->mod.setter = List_setter;
		mod->mod.remover = List_remover;
		mod->mod.typeSetter = List_setType;
		mod->mod.free = (void (*)(YModifier*)) free;
		mod->mod.local = local;
		return (YModifier*) mod;
	} else if (node->type == ConditionN) {
		YConditionNode* cnd = (YConditionNode*) node;
		YConditionalModifier* mod = malloc(sizeof(YConditionalModifier));
		mod->cond = cnd->cond;
		mod->body = cnd->body;
		mod->elseBody = cnd->elseBody;
		mod->mod.setter = Conditional_setter;
		mod->mod.remover = Conditinal_remover;
		mod->mod.typeSetter = Conditional_setType;
		mod->mod.free = (void (*)(YModifier*)) free;
		mod->mod.local = local;
		return (YModifier*) mod;
	} else {
		CompilationError(builder->err_stream, L"Unmodifieable", node,
				builder->bc);
	}
	return NULL;
}

int32_t ytranslate(YCodeGen* builder, YoyoCEnvironment* env, YNode* node) {
	ProcedureBuilder* proc = builder->proc;
	int32_t output = -1;
	size_t startLen = proc->proc->code_length;
	switch (node->type) {
	case ConstantN: {
		int32_t reg = proc->nextRegister(proc);
		yconstant_t cnst = ((YConstantNode*) node)->id;
		int32_t cid = -1;
		if (cnst.type == Int64Constant) {
			int64_t value = cnst.value.i64;
			if (value>=INT32_MIN&&
				value<=INT32_MAX) {
				proc->append(proc, VM_LoadInteger, reg, (int32_t) value, -1);
				output = reg;
			}
			cid = builder->bc->addIntegerConstant(builder->bc, cnst.value.i64);
		}
		if (cnst.type == BoolConstant)
			cid = builder->bc->addBooleanConstant(builder->bc,
					cnst.value.boolean);
		if (cnst.type == Fp64Constant)
			cid = builder->bc->addFloatConstant(builder->bc, cnst.value.fp64);
		if (cnst.type == WcsConstant) {
			cid = builder->bc->addStringConstant(builder->bc, cnst.value.wcs);
		}
		if (output==-1) {
			proc->append(proc, VM_LoadConstant, reg, cid, -1);
			output = reg;
		}
	}
		break;
	case IdentifierReferenceN: {
		int32_t id = env->bytecode->getSymbolId(env->bytecode,
				((YIdentifierReferenceNode*) node)->id);
		int32_t reg = proc->nextRegister(proc);
		LocalVarEntry* e = Procedure_getLocalVar(proc, id);
		if (e!=NULL) {
			proc->append(proc, VM_Copy, reg, e->value_reg, -1);
		} else {
			proc->append(proc, VM_GetField, reg, 0, id);
		}
		output = reg;
	}
		break;
	case ObjectN: {
		YObjectNode* obj = (YObjectNode*) node;
		int32_t reg = proc->nextRegister(proc);
		int32_t scopeReg = proc->nextRegister(proc);
		if (obj->parent == NULL)
			proc->append(proc, VM_NewObject, reg, -1, -1);
		else {
			int32_t preg = ytranslate(builder, env, obj->parent);
			proc->append(proc, VM_NewObject, reg, preg, -1);
			proc->append(proc, VM_NewField, reg,
					builder->bc->getSymbolId(builder->bc, L"super"), preg);
			proc->unuse(proc, preg);
		}
		proc->append(proc, VM_NewField, reg,
				builder->bc->getSymbolId(builder->bc, L"outer"), 0);
		proc->append(proc, VM_NewField, reg,
				builder->bc->getSymbolId(builder->bc, L"this"), reg);
		proc->append(proc, VM_Push, 0, -1, -1);
		proc->append(proc, VM_NewComplexObject, scopeReg, reg, 1);
		proc->append(proc, VM_Swap, 0, scopeReg, -1);
		for (size_t i = 0; i < obj->methods_length; i++) {
			for (size_t j = 0; j < obj->methods[i].count; j++) {
				int32_t lreg = ytranslate(builder, env,
						(YNode*) obj->methods[i].lambda[j]);
				proc->append(proc, VM_Push, lreg, -1, -1);
				proc->unuse(proc, lreg);
			}
			int32_t oreg = proc->nextRegister(proc);
			proc->append(proc, VM_NewOverload, oreg, obj->methods[i].count, -1);
			proc->append(proc, VM_NewField, reg,
					builder->bc->getSymbolId(builder->bc, obj->methods[i].id),
					oreg);
			proc->unuse(proc, oreg);
		}
		for (size_t i = 0; i < obj->fields_length; i++) {
			ObjectNodeField* fld = &obj->fields[i];
			int32_t vreg = ytranslate(builder, env, fld->value);
			proc->append(proc, VM_NewField, reg,
					builder->bc->getSymbolId(builder->bc, fld->id), vreg);
			proc->unuse(proc, vreg);
			if (fld->type != NULL) {
				int32_t treg = ytranslate(builder, env, fld->type);
				proc->append(proc, VM_ChangeType, reg,
						builder->bc->getSymbolId(builder->bc, fld->id), treg);
				proc->unuse(proc, treg);
			}
		}
		proc->append(proc, VM_Swap, scopeReg, 0, -1);
		proc->unuse(proc, scopeReg);
		output = reg;
	}
		break;
	case ArrayReferenceN: {
		YArrayReferenceNode* arr = (YArrayReferenceNode*) node;
		int32_t areg = ytranslate(builder, env, arr->array);
		int32_t ireg = ytranslate(builder, env, arr->index);
		proc->append(proc, VM_ArrayGet, areg, areg, ireg);
		proc->unuse(proc, ireg);
		output = areg;
	}
		break;
	case SubsequenceN: {
		YSubsequenceReferenceNode* subseq = (YSubsequenceReferenceNode*) node;
		int fromReg = ytranslate(builder, env, subseq->from);
		int toReg = ytranslate(builder, env, subseq->to);
		int arrayReg = ytranslate(builder, env, subseq->array);
		proc->append(proc, VM_Subsequence, arrayReg, fromReg, toReg);
		proc->unuse(proc, fromReg);
		proc->unuse(proc, toReg);
		output = arrayReg;
	}
		break;
	case FieldReferenceN: {
		YFieldReferenceNode* fref = (YFieldReferenceNode*) node;
		int32_t obj = ytranslate(builder, env, fref->object);
		proc->append(proc, VM_GetField, obj, obj,
				env->bytecode->getSymbolId(env->bytecode, fref->field));
		output = obj;
	}
		break;
	case CallN: {
		YCallNode* call = (YCallNode*) node;
		for (size_t i = 0; i < call->argc; i++) {
			int32_t reg = ytranslate(builder, env, call->args[i]);
			proc->append(proc, VM_Push, reg, -1, -1);
			proc->unuse(proc, reg);
		}
		int32_t reg = proc->nextRegister(proc);
		proc->append(proc, VM_LoadInteger, reg, call->argc, -1);
		proc->append(proc, VM_Push, reg, -1, -1);
		proc->unuse(proc, reg);
		reg = ytranslate(builder, env, call->function);
		int32_t scopeReg = -1;
		if (call->scope != NULL)
			scopeReg = ytranslate(builder, env, call->scope);
		proc->append(proc, VM_Call, reg, reg, scopeReg);
		proc->unuse(proc, scopeReg);
		output = reg;
	}
		break;
	case LambdaN: {
		YLambdaNode* ln = (YLambdaNode*) node;

		ProcedureBuilder* lproc = builder->newProcedure(builder);
		int32_t pid = lproc->proc->id;
		int32_t outR = ytranslate(builder, env, ln->body);
		lproc->append(lproc, VM_Return, outR, -1, -1);
		lproc->unuse(lproc, outR);
		builder->endProcedure(builder);

		int32_t reg = proc->nextRegister(proc);
		if (ln->retType != NULL) {
			int atReg = ytranslate(builder, env, ln->retType);
			proc->append(proc, VM_Push, atReg, -1, -1);
			proc->unuse(proc, atReg);
		} else {
			proc->append(proc, VM_LoadConstant, reg,
					env->bytecode->getNullConstant(env->bytecode), -1);
			proc->append(proc, VM_Push, reg, -1, -1);
		}
		for (size_t i = 0; i < ln->argc; i++) {
			if (ln->argTypes[i] != NULL) {
				int atReg = ytranslate(builder, env, ln->argTypes[i]);
				proc->append(proc, VM_Push, atReg, -1, -1);
				proc->unuse(proc, atReg);
			} else {
				proc->append(proc, VM_LoadConstant, reg,
						env->bytecode->getNullConstant(env->bytecode), -1);
				proc->append(proc, VM_Push, reg, -1, -1);
			}
			proc->append(proc, VM_LoadInteger, reg,
					builder->bc->getSymbolId(builder->bc, ln->args[i]), -1);
			proc->append(proc, VM_Push, reg, -1, -1);
		}
		proc->append(proc, VM_LoadInteger, reg, ln->argc, -1);
		proc->append(proc, VM_Push, reg, -1, -1);
		proc->append(proc, VM_LoadConstant, reg,
				builder->bc->addBooleanConstant(builder->bc, ln->vararg), -1);
		proc->append(proc, VM_Push, reg, -1, -1);
		proc->append(proc, VM_LoadConstant, reg,
				builder->bc->addBooleanConstant(builder->bc, ln->method), -1);
		proc->append(proc, VM_Push, reg, -1, -1);
		int32_t scopeReg = proc->nextRegister(proc);
		int32_t staticReg = proc->nextRegister(proc);
		proc->append(proc, VM_NewObject, scopeReg, 0, -1);
		proc->append(proc, VM_LoadConstant, staticReg,
				builder->bc->getNullConstant(builder->bc), -1);
		proc->append(proc, VM_NewField, scopeReg,
				builder->bc->getSymbolId(builder->bc, L"static"), staticReg);
		proc->append(proc, VM_NewLambda, reg, pid, scopeReg);
		proc->unuse(proc, scopeReg);
		proc->unuse(proc, staticReg);

		output = reg;
	}
		break;
	case FilledArrayN: {
		YFilledArrayNode* farr = (YFilledArrayNode*) node;
		int32_t reg = proc->nextRegister(proc);
		int32_t index = proc->nextRegister(proc);
		proc->append(proc, VM_NewArray, reg, -1, -1);
		for (size_t i = 0; i < farr->length; i++) {
			int32_t r = ytranslate(builder, env, farr->array[i]);
			proc->append(proc, VM_LoadInteger, index, i, -1);
			proc->append(proc, VM_ArraySet, reg, index, r);
			proc->unuse(proc, r);
		}
		proc->unuse(proc, index);
		output = reg;
	}
		break;
	case GeneratedArrayN: {
		YGeneratedArrayNode* garr = (YGeneratedArrayNode*) node;
		int32_t arr = proc->nextRegister(proc);
		proc->append(proc, VM_NewArray, arr, -1, -1);
		int32_t oneReg = proc->nextRegister(proc);
		int32_t index = proc->nextRegister(proc);
		int32_t reg = proc->nextRegister(proc);
		int32_t count = ytranslate(builder, env, garr->count);
		int32_t val = ytranslate(builder, env, garr->element);
		proc->append(proc, VM_LoadInteger, oneReg, 1, -1);
		proc->append(proc, VM_LoadInteger, index, 0, -1);

		int32_t startL = proc->nextLabel(proc);
		int32_t endL = proc->nextLabel(proc);

		proc->bind(proc, startL);
		proc->append(proc, VM_Copy, reg, index, -1);
		proc->append(proc, VM_FastCompare, reg, count, COMPARE_EQUALS);
		proc->append(proc, VM_GotoIfTrue, endL, reg, -1);

		proc->append(proc, VM_ArraySet, arr, index, val);
		proc->append(proc, VM_Add, index, index, oneReg);

		proc->append(proc, VM_Goto, startL, -1, -1);
		proc->bind(proc, endL);

		proc->unuse(proc, val);
		proc->unuse(proc, index);
		proc->unuse(proc, reg);
		proc->unuse(proc, oneReg);
		proc->unuse(proc, count);
		output = arr;
	}
		break;
	case UnaryN: {
		YUnaryNode* un = (YUnaryNode*) node;
		int32_t reg = ytranslate(builder, env, un->argument);
		switch (un->operation) {
		case Negate:
			proc->append(proc, VM_Negate, reg, reg, -1);
			break;
		case Not:
		case LogicalNot:
			proc->append(proc, VM_Not, reg, reg, -1);
			break;
		case PreDecrement: {
			proc->append(proc, VM_Decrement, reg, reg, -1);
			YModifier* mod = ymodifier(builder, env, un->argument, false);
			if (mod == NULL) {
				fprintf(env->env.out_stream,
						"Expected modifieable expression at %"PRId32":%"PRId32"\n",
						un->argument->line, un->argument->charPos);
				break;
			}
			mod->setter(mod, builder, env, false, reg);
			mod->free(mod);
		}
			break;
		case PreIncrement: {
			proc->append(proc, VM_Increment, reg, reg, -1);
			YModifier* mod = ymodifier(builder, env, un->argument, false);
			if (mod == NULL) {
				fprintf(env->env.out_stream,
						"Expected modifieable expression at %"PRId32":%"PRId32"\n",
						un->argument->line, un->argument->charPos);
				break;
			}
			mod->setter(mod, builder, env, false, reg);
			mod->free(mod);
		}
			break;
		case PostDecrement: {
			int32_t res = proc->nextRegister(proc);
			proc->append(proc, VM_Decrement, res, reg, -1);

			YModifier* mod = ymodifier(builder, env, un->argument, false);
			if (mod == NULL) {
				fprintf(env->env.out_stream,
						"Expected modifieable expression at %"PRId32":%"PRId32"\n",
						un->argument->line, un->argument->charPos);
				break;
			}
			mod->setter(mod, builder, env, false, res);
			mod->free(mod);
			proc->unuse(proc, res);

		}
			break;
		case PostIncrement: {
			int32_t res = proc->nextRegister(proc);
			proc->append(proc, VM_Increment, res, reg, -1);

			YModifier* mod = ymodifier(builder, env, un->argument, false);
			if (mod == NULL) {
				fprintf(env->env.out_stream,
						"Expected modifieable expression at %"PRId32":%"PRId32"\n",
						un->argument->line, un->argument->charPos);
				break;
			}
			mod->setter(mod, builder, env, false, res);
			mod->free(mod);

			proc->unuse(proc, res);

		}
			break;
		default:
			break;
		}
		output = reg;
	}
		break;
	case BinaryN: {
		YBinaryNode* bin = (YBinaryNode*) node;
		if (bin->operation == LogicalAnd) {
			output = proc->nextRegister(proc);
			proc->append(proc, VM_LoadConstant, output,
					builder->bc->addBooleanConstant(builder->bc, false), -1);
			int32_t endL = proc->nextLabel(proc);
			int32_t cnd1 = ytranslate(builder, env, bin->left);
			proc->append(proc, VM_GotoIfFalse, endL, cnd1, -1);
			proc->unuse(proc, cnd1);
			cnd1 = ytranslate(builder, env, bin->right);
			proc->append(proc, VM_GotoIfFalse, endL, cnd1, -1);
			proc->unuse(proc, cnd1);
			proc->append(proc, VM_LoadConstant, output,
					builder->bc->addBooleanConstant(builder->bc, true), -1);
			proc->bind(proc, endL);
			break;
		}
		if (bin->operation == LogicalOr) {
			output = proc->nextRegister(proc);
			proc->append(proc, VM_LoadConstant, output,
					builder->bc->addBooleanConstant(builder->bc, true), -1);
			int32_t endL = proc->nextLabel(proc);
			int32_t cnd1 = ytranslate(builder, env, bin->left);
			proc->append(proc, VM_GotoIfTrue, endL, cnd1, -1);
			proc->unuse(proc, cnd1);
			cnd1 = ytranslate(builder, env, bin->right);
			proc->append(proc, VM_GotoIfTrue, endL, cnd1, -1);
			proc->unuse(proc, cnd1);
			proc->append(proc, VM_LoadConstant, output,
					builder->bc->addBooleanConstant(builder->bc, false), -1);
			proc->bind(proc, endL);
			break;
		}
		int32_t left = ytranslate(builder, env, bin->left);
		int32_t right = ytranslate(builder, env, bin->right);
		int32_t out = left;
		switch (bin->operation) {
		case Add:
			proc->append(proc, VM_Add, out, left, right);
			break;
		case Subtract:
			proc->append(proc, VM_Subtract, out, left, right);
			break;
		case Multiply:
			proc->append(proc, VM_Multiply, out, left, right);
			break;
		case Divide:
			proc->append(proc, VM_Divide, out, left, right);
			break;
		case Modulo:
			proc->append(proc, VM_Modulo, out, left, right);
			break;
		case Power:
			proc->append(proc, VM_Power, out, left, right);
			break;
		case ShiftLeft:
			proc->append(proc, VM_ShiftLeft, out, left, right);
			break;
		case ShiftRight:
			proc->append(proc, VM_ShiftRight, out, left, right);
			break;
		case And:
			proc->append(proc, VM_And, out, left, right);
			break;
		case Or:
			proc->append(proc, VM_Or, out, left, right);
			break;
		case Xor:
			proc->append(proc, VM_Xor, out, left, right);
			break;
		case BEquals:
			proc->append(proc, VM_FastCompare, left, right, COMPARE_EQUALS);
			break;
		case NotEquals:
			proc->append(proc, VM_FastCompare, left, right, COMPARE_NOT_EQUALS);
			break;
		case Lesser:
			proc->append(proc, VM_FastCompare, left, right, COMPARE_LESSER);
			break;
		case Greater:
			proc->append(proc, VM_FastCompare, left, right, COMPARE_GREATER);
			break;
		case LesserOrEquals:
			proc->append(proc, VM_FastCompare, left, right,
					COMPARE_LESSER_OR_EQUALS);
			break;
		case GreaterOrEquals:
			proc->append(proc, VM_FastCompare, left, right,
					COMPARE_GREATER_OR_EQUALS);
			break;
		default:
			break;
		}
		proc->unuse(proc, right);
		output = out;
	}
		break;
	case AssignmentN: {
		YAssignmentNode* asn = (YAssignmentNode*) node;
		size_t index = 0;
		int32_t* src = malloc(sizeof(int32_t) * asn->src_len);
		for (size_t i = 0; i < asn->src_len; i++)
			src[i] = ytranslate(builder, env, asn->src[i]);

		for (size_t i = 0; i < asn->dest_len; i++) {
			YModifier* mod = ymodifier(builder, env, asn->dest[i], asn->local);
			if (mod != NULL) {
				if (asn->type != NULL) {
					int32_t nullReg = proc->nextRegister(proc);
					proc->append(proc, VM_LoadConstant, nullReg,
							env->bytecode->getNullConstant(env->bytecode), -1);
					mod->setter(mod, builder, env, asn->newVar, nullReg);
					proc->unuse(proc, nullReg);
					int32_t typeReg = ytranslate(builder, env, asn->type);
					mod->typeSetter(mod, builder, env, typeReg);
					proc->unuse(proc, typeReg);
				}
				if (asn->operation == AAssign) {
					mod->setter(mod, builder, env, asn->newVar, src[index++]);
				} else if (asn->operation == AAddAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_Add, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				} else if (asn->operation == ASubAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_Subtract, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				} else if (asn->operation == AMulAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_Multiply, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				} else if (asn->operation == ADivAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_Divide, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				} else if (asn->operation == AModAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_Modulo, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				} else if (asn->operation == APowerAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_Power, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				} else if (asn->operation == AShiftLeftAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_ShiftLeft, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				} else if (asn->operation == AShiftRightAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_ShiftRight, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				} else if (asn->operation == AOrAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_Or, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				} else if (asn->operation == AAndAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_And, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				} else if (asn->operation == AXorAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_Xor, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				} else if (asn->operation == ALogicalAndAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_And, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				} else if (asn->operation == ALogicalOrAssign) {
					int32_t val = ytranslate(builder, env, asn->dest[i]);
					proc->append(proc, VM_Or, val, val, src[index++]);
					mod->setter(mod, builder, env, asn->newVar, val);
					proc->unuse(proc, val);
				}
				mod->free(mod);
			}
			index %= asn->src_len;
		}

		for (size_t i = 0; i < asn->src_len; i++)
			proc->unuse(proc, src[i]);
		free(src);
		if (asn->dest_len == 1) {
			output = ytranslate(builder, env, asn->dest[0]);
		} else {
			output = proc->nextRegister(proc);
			proc->append(proc, VM_NewArray, output, -1, -1);
			int32_t index = proc->nextRegister(proc);
			for (size_t i = 0; i < asn->dest_len; i++) {
				proc->append(proc, VM_LoadInteger, index, i, -1);
				int32_t val = ytranslate(builder, env, asn->dest[i]);
				proc->append(proc, VM_ArraySet, output, index, val);
				proc->unuse(proc, val);
			}
			proc->unuse(proc, index);
		}
	}
		break;
	case DeleteN: {
		YDeleteNode* del = (YDeleteNode*) node;
		for (size_t i = 0; i < del->length; i++) {
			YModifier* mod = ymodifier(builder, env, del->list[i], false);
			mod->remover(mod, builder, env);
			mod->free(mod);
		}
	}
		break;
	case ConditionN: {
		YConditionNode* cnd = (YConditionNode*) node;
		int32_t out = -1;
		if (cnd->elseBody != NULL) {
			int32_t trueL = proc->nextLabel(proc);
			int32_t falseL = proc->nextLabel(proc);
			int32_t cond = ytranslate(builder, env, cnd->cond);
			proc->append(proc, VM_GotoIfFalse, falseL, cond, -1);
			out = cond;

			int32_t tReg = ytranslate(builder, env, cnd->body);
			proc->append(proc, VM_Copy, out, tReg, -1);
			proc->unuse(proc, tReg);
			proc->append(proc, VM_Goto, trueL, -1, -1);
			proc->bind(proc, falseL);
			int32_t fReg = ytranslate(builder, env, cnd->elseBody);
			proc->append(proc, VM_Copy, out, fReg, -1);
			proc->unuse(proc, fReg);
			proc->bind(proc, trueL);
		} else {
			int32_t trueL = proc->nextLabel(proc);
			int32_t falseL = proc->nextLabel(proc);
			int32_t cond = ytranslate(builder, env, cnd->cond);
			proc->append(proc, VM_GotoIfFalse, falseL, cond, -1);
			out = cond;

			int32_t tReg = ytranslate(builder, env, cnd->body);
			proc->append(proc, VM_Copy, out, tReg, -1);
			proc->unuse(proc, tReg);
			proc->append(proc, VM_Goto, trueL, -1, -1);
			proc->bind(proc, falseL);
			proc->append(proc, VM_LoadConstant, out,
					env->bytecode->getNullConstant(env->bytecode), -1);
			proc->bind(proc, trueL);

		}
		output = out;

	}
		break;
	case WhileLoopN: {
		YWhileLoopNode* loop = (YWhileLoopNode*) node;
		int32_t startL = proc->nextLabel(proc);
		int32_t endL = proc->nextLabel(proc);
		proc->startLoop(proc, builder->bc->getSymbolId(builder->bc, loop->id),
				startL, endL);

		if (loop->evalOnStart) {
			proc->bind(proc, startL);
			int32_t cond = ytranslate(builder, env, loop->cond);
			proc->append(proc, VM_GotoIfFalse, endL, cond, -1);
			proc->unuse(proc, cond);
			proc->unuse(proc, ytranslate(builder, env, loop->body));
			proc->append(proc, VM_Goto, startL, -1, -1);
			proc->bind(proc, endL);
		} else {
			proc->bind(proc, startL);
			proc->unuse(proc, ytranslate(builder, env, loop->body));
			int32_t cond = ytranslate(builder, env, loop->cond);
			proc->append(proc, VM_GotoIfTrue, startL, cond, -1);
			proc->unuse(proc, cond);
			proc->bind(proc, endL);
		}
		proc->endLoop(proc);
	}
		break;
	case LoopN: {
		YLoopNode* loop = (YLoopNode*) node;
		int32_t startL = proc->nextLabel(proc);
		int32_t endL = proc->nextLabel(proc);
		proc->startLoop(proc, builder->bc->getSymbolId(builder->bc, loop->id),
				startL, endL);

		proc->bind(proc, startL);
		proc->unuse(proc, ytranslate(builder, env, loop->body));
		proc->append(proc, VM_Goto, startL, -1, -1);
		proc->bind(proc, endL);

		proc->endLoop(proc);
	}
		break;
	case ForLoopN: {
		YForLoopNode* loop = (YForLoopNode*) node;
		int32_t realStartL = proc->nextLabel(proc);
		int32_t startL = proc->nextLabel(proc);
		int32_t endL = proc->nextLabel(proc);
		proc->startLoop(proc, builder->bc->getSymbolId(builder->bc, loop->id),
				startL, endL);

		proc->unuse(proc, ytranslate(builder, env, loop->init));

		proc->bind(proc, realStartL);
		int32_t cond = ytranslate(builder, env, loop->cond);
		proc->append(proc, VM_GotoIfFalse, endL, cond, -1);
		proc->unuse(proc, cond);
		proc->unuse(proc, ytranslate(builder, env, loop->body));
		proc->bind(proc, startL);
		proc->unuse(proc, ytranslate(builder, env, loop->loop));
		proc->append(proc, VM_Goto, realStartL, -1, -1);
		proc->bind(proc, endL);

		proc->endLoop(proc);
	}
		break;
	case ForeachLoopN: { //TODO 
		YForeachLoopNode* loop = (YForeachLoopNode*) node;
		YModifier* mod = ymodifier(builder, env, loop->refnode, false);
		int32_t reg = ytranslate(builder, env, loop->col);
		int32_t startL = proc->nextLabel(proc);
		int32_t endL = proc->nextLabel(proc);
		proc->startLoop(proc, builder->bc->getSymbolId(builder->bc, loop->id),
				startL, endL);

		proc->append(proc, VM_Iterator, reg, reg, -1);
		proc->bind(proc, startL);
		int32_t val = proc->nextRegister(proc);
		proc->append(proc, VM_Iterate, val, reg, endL);
		mod->setter(mod, builder, env, true, val);
		proc->unuse(proc, val);
		proc->unuse(proc, ytranslate(builder, env, loop->body));

		proc->append(proc, VM_Goto, startL, -1, -1);
		proc->bind(proc, endL);
		proc->endLoop(proc);
		proc->append(proc, VM_Swap, 0, 0, -1);

		proc->unuse(proc, reg);
		mod->free(mod);
	}
		break;
	case ReturnN: {
		int32_t out = ytranslate(builder, env,
				((YValueReturnNode*) node)->value);
		proc->append(proc, VM_Return, out, -1, -1);
		proc->unuse(proc, out);
	}
		break;
	case ThrowN: {
		int32_t out = ytranslate(builder, env,
				((YValueReturnNode*) node)->value);
		proc->append(proc, VM_Throw, out, -1, -1);
		proc->unuse(proc, out);
	}
		break;
	case PassN:
		proc->append(proc, VM_Nop, -1, -1, -1);
		break;
	case BreakN: {
		int32_t id = builder->bc->getSymbolId(builder->bc,
				((YLoopControlNode*) node)->label);
		ProcdeureBuilderLoop* loop = proc->loop;
		if (id != -1)
			loop = proc->getLoop(proc, id);
		if (loop != NULL) {
			proc->append(proc, VM_Goto, loop->end, -1, -1);
		} else {
			CompilationError(builder->err_stream, L"Loop not found", node, bc);
		}
	}
		break;
	case ContinueN: {
		int32_t id = builder->bc->getSymbolId(builder->bc,
				((YLoopControlNode*) node)->label);
		ProcdeureBuilderLoop* loop = proc->loop;
		if (id != -1)
			loop = proc->getLoop(proc, id);
		if (loop != NULL) {
			proc->append(proc, VM_Goto, loop->start, -1, -1);
		} else {
			CompilationError(builder->err_stream, L"Loop not found", node, bc);
		}
	}
		break;
	case TryN: {
		YTryNode* tn = (YTryNode*) node;
		int32_t endL = proc->nextLabel(proc);
		int32_t catchL = -1;
		int32_t elseL = (tn->elseBody != NULL) ? proc->nextLabel(proc) : -1;
		int32_t finL = (tn->finBody != NULL) ? proc->nextLabel(proc) : -1;
		output = proc->nextRegister(proc);
		if (tn->catchBody != NULL) {
			catchL = proc->nextLabel(proc);
			proc->append(proc, VM_OpenCatch, catchL, -1, -1);
		}
		int32_t reg = ytranslate(builder, env, tn->tryBody);
		proc->append(proc, VM_Copy, output, reg, -1);
		proc->unuse(proc, reg);
		if (catchL != -1)
			proc->append(proc, VM_CloseCatch, -1, -1, -1);
		proc->append(proc, VM_Goto, elseL != -1 ? elseL : endL, -1, -1);
		if (catchL != -1) {
			int32_t regExc = proc->nextRegister(proc);
			int32_t tempReg = proc->nextRegister(proc);
			proc->bind(proc, catchL);
			proc->append(proc, VM_Catch, regExc, -1, -1);
			proc->append(proc, VM_CloseCatch, -1, -1, -1);
			proc->append(proc, VM_NewObject, tempReg, 0, -1);
			proc->append(proc, VM_Swap, tempReg, 0, -1);

			YModifier* mod = ymodifier(builder, env, tn->catchRef, false);
			mod->setter(mod, builder, env, true, regExc);
			mod->free(mod);
			proc->unuse(proc, regExc);

			reg = ytranslate(builder, env, tn->catchBody);
			proc->append(proc, VM_Copy, output, reg, -1);
			proc->unuse(proc, reg);

			proc->append(proc, VM_Swap, tempReg, 0, -1);
			proc->unuse(proc, tempReg);
			proc->append(proc, VM_Goto, finL != -1 ? finL : endL, -1, -1);
		}
		if (elseL != -1) {
			proc->bind(proc, elseL);
			reg = ytranslate(builder, env, tn->elseBody);
			proc->append(proc, VM_Copy, output, reg, -1);
			proc->unuse(proc, reg);
		}
		if (finL != -1) {
			proc->bind(proc, finL);
			reg = ytranslate(builder, env, tn->finBody);
			proc->append(proc, VM_Copy, output, reg, -1);
			proc->unuse(proc, reg);
		}
		proc->bind(proc, endL);
	}
		break;
	case SwitchN: {
		YSwitchNode* swn = (YSwitchNode*) node;
		int32_t endL = proc->nextLabel(proc);
		int32_t* labels = malloc(sizeof(int32_t) * swn->length);
		int32_t valueReg = ytranslate(builder, env, swn->value);
		for (size_t i = 0; i < swn->length; i++) {
			labels[i] = proc->nextLabel(proc);
			int32_t reg = ytranslate(builder, env, swn->cases[i].value);
			proc->append(proc, VM_FastCompare, reg, valueReg, COMPARE_EQUALS);
			proc->append(proc, VM_GotoIfTrue, labels[i], reg, -1);
			proc->unuse(proc, reg);
		}
		output = valueReg;
		if (swn->defCase != NULL) {
			int32_t resR = ytranslate(builder, env, swn->defCase);
			proc->append(proc, VM_Copy, output, resR, -1);
			proc->unuse(proc, resR);
		}
		proc->append(proc, VM_Goto, endL, -1, -1);
		for (size_t i = 0; i < swn->length; i++) {
			proc->bind(proc, labels[i]);
			int32_t resR = ytranslate(builder, env, swn->cases[i].stmt);
			proc->append(proc, VM_Copy, output, resR, -1);
			proc->unuse(proc, resR);
			proc->append(proc, VM_Goto, endL, -1, -1);
		}
		proc->bind(proc, endL);
		free(labels);
	}
		break;
	case OverloadN: {
		YOverloadNode* ovrld = (YOverloadNode*) node;
		for (size_t i = 0; i < ovrld->count; i++) {
			int32_t reg = ytranslate(builder, env, ovrld->lambdas[i]);
			proc->append(proc, VM_Push, reg, -1, -1);
			proc->unuse(proc, reg);
		}
		int32_t def = -1;
		if (ovrld->defLambda != NULL)
			def = ytranslate(builder, env, ovrld->defLambda);
		output = proc->nextRegister(proc);
		proc->append(proc, VM_NewOverload, output, ovrld->count, def);
		proc->unuse(proc, def);
	}
		break;
	case UsingN: {
		YUsingNode* usng = (YUsingNode*) node;
		for (size_t i = 0; i < usng->count; i++) {
			int32_t reg = ytranslate(builder, env, usng->scopes[i]);
			proc->append(proc, VM_Push, reg, -1, -1);
			proc->unuse(proc, reg);
		}
		int32_t scope = proc->nextRegister(proc);
		proc->append(proc, VM_NewComplexObject, scope, 0, usng->count);
		proc->append(proc, VM_Swap, scope, 0, -1);
		output = ytranslate(builder, env, usng->body);
		proc->append(proc, VM_Swap, scope, 0, -1);
		proc->unuse(proc, scope);
	}
		break;
	case WithN: {
		YWithNode* wth = (YWithNode*) node;
		int32_t scope = ytranslate(builder, env, wth->scope);
		proc->append(proc, VM_Push, 0, -1, -1);
		proc->append(proc, VM_NewComplexObject, scope, scope, 1);
		proc->append(proc, VM_Swap, scope, 0, -1);
		output = ytranslate(builder, env, wth->body);
		proc->append(proc, VM_Swap, scope, 0, -1);
		proc->unuse(proc, scope);
	}
		break;
	case InterfaceN: {
		YInterfaceNode* in = (YInterfaceNode*) node;
		for (size_t i = 0; i < in->attr_count; i++) {
			int reg = ytranslate(builder, env, in->types[i]);
			proc->append(proc, VM_Push, reg, -1, -1);
			proc->append(proc, VM_LoadInteger, reg,
					builder->bc->getSymbolId(builder->bc, in->ids[i]), -1);
			proc->append(proc, VM_Push, reg, -1, -1);
			proc->unuse(proc, reg);
		}
		for (size_t i = 0; i < in->parent_count; i++) {
			int reg = ytranslate(builder, env, in->parents[i]);
			proc->append(proc, VM_Push, reg, -1, -1);
			proc->unuse(proc, reg);
		}
		output = proc->nextRegister(proc);
		proc->append(proc, VM_NewInterface, output, in->parent_count,
				in->attr_count);
	}
		break;
	case BlockN: {
		YBlockNode* block = (YBlockNode*) node;
		int32_t out = -1;
		int32_t oldScope = proc->nextRegister(proc);
		proc->append(proc, VM_Copy, oldScope, 0, -1);
		proc->append(proc, VM_NewObject, 0, 0, -1);
		for (size_t i = 0; i < block->funcs_count; i++) {
			for (size_t j = 0; j < block->funcs[i].count; j++) {
				int32_t reg = ytranslate(builder, env,
						(YNode*) block->funcs[i].lambda[j]);
				proc->append(proc, VM_Push, reg, -1, -1);
				proc->unuse(proc, reg);
			}
			int32_t reg = proc->nextRegister(proc);
			proc->append(proc, VM_NewOverload, reg, block->funcs[i].count, -1);
			proc->append(proc, VM_SetField, 0,
					builder->bc->getSymbolId(builder->bc, block->funcs[i].id),
					reg);
			proc->unuse(proc, reg);
		}
		for (size_t i = 0; i < block->length; i++) {
			proc->unuse(proc, out);
			out = ytranslate(builder, env, block->block[i]);
		}
		proc->append(proc, VM_Copy, 0, oldScope, -1);
		proc->unuse(proc, oldScope);
		output = out;
	}
		break;
	case RootN: {
		YBlockNode* root = (YBlockNode*) node;
		for (size_t i = 0; i < root->funcs_count; i++) {
			for (size_t j = 0; j < root->funcs[i].count; j++) {
				int32_t reg = ytranslate(builder, env,
						(YNode*) root->funcs[i].lambda[j]);
				proc->append(proc, VM_Push, reg, -1, -1);
				proc->unuse(proc, reg);
			}
			int32_t reg = proc->nextRegister(proc);
			proc->append(proc, VM_NewOverload, reg, root->funcs[i].count, -1);
			proc->append(proc, VM_SetField, 0,
					builder->bc->getSymbolId(builder->bc, root->funcs[i].id),
					reg);
			proc->unuse(proc, reg);
		}
		uint32_t last = -1;
		for (size_t i = 0; i < root->length; i++) {
			proc->unuse(proc, last);
			last = ytranslate(builder, env, root->block[i]);
		}
		proc->append(proc, VM_Return, last, -1, -1);
		proc->unuse(proc, last);
	}
		break;
	default:
		break;
	}
	if (node->line != -1 && node->charPos != -1)
		proc->proc->addCodeTableEntry(proc->proc, node->line, node->charPos,
				startLen, proc->proc->code_length,
				env->bytecode->getSymbolId(env->bytecode, node->fileName));
	return output;
}

int32_t ycompile(YoyoCEnvironment* env, YNode* root, FILE* err_stream) {
	if (root == NULL)
		return -1;
	YCodeGen builder;
	builder.bc = env->bytecode;
    builder.jit = env->jit;
	builder.proc = NULL;
	builder.newProcedure = YCodeGen_newProcedure;
	builder.endProcedure = YCodeGen_endProcedure;
	builder.err_stream = err_stream;
	builder.preprocess = env->preprocess_bytecode;

	ProcedureBuilder* proc = builder.newProcedure(&builder);
	int32_t pid = proc->proc->id;

	builder.proc->unuse(builder.proc, ytranslate(&builder, env, root));

	while (builder.proc != NULL)
		builder.endProcedure(&builder);

	root->free(root);
	fflush(err_stream);
	return pid;
}

