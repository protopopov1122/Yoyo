/*
 * Copyright (C) 2016  Jevgenijs Protopopovs <protopopov1122@yandex.ru>
 */
/*This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 3 as published by
 the Free Software Foundation.


 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include "jit.h"
#include "bytecode.h"
#include "analyze.h"

/* File contains methods to build bytecode and get access to it.
 * Work with bytecode internal structures like
 * constant pool, symbol pool, procedure list.
 * Also work with procedure internals like
 * code, register information, label lists and
 * tables to identify bytecode and source code.
 * See bytecode description. */

void freeBytecode(ILBytecode* bc) {
	for (size_t i = 0; i < bc->constants.size; i++)
		free(bc->constants.pool[i]);
	free(bc->constants.pool);
	for (size_t i = 0; i < bc->procedure_count; i++) {
		ILProcedure* proc = bc->procedures[i];
		if (proc == NULL)
			continue;
		procedure_stats_free(proc->stats);
		free(proc->code);
		free(proc->labels.table);
		free(proc->codeTable.table);
		free(proc);
	}
	free(bc->procedures);
	DESTROY_MUTEX(&bc->access_mutex);
	free(bc);
}

void Procedure_appendCode(ILProcedure* proc, uint8_t* nc, size_t len) {
	size_t newLen = proc->code_length + len;
	proc->code = realloc(proc->code, sizeof(uint8_t) * newLen);
	for (size_t i = 0; i < len; i++)
		proc->code[i + proc->code_length] = nc[i];
	proc->code_length = newLen;
}
void Procedure_addLabel(ILProcedure* proc, int32_t key, uint32_t value) {
	proc->labels.table = realloc(proc->labels.table,
			sizeof(LabelEntry) * (proc->labels.length + 1));
	proc->labels.table[proc->labels.length].id = key;
	proc->labels.table[proc->labels.length].value = value;
	proc->labels.length++;
}
LabelEntry* Procedure_getLabel(ILProcedure* proc, int32_t id) {
	for (size_t i = 0; i < proc->labels.length; i++)
		if (proc->labels.table[i].id == id)
			return &proc->labels.table[i];
	return NULL;
}
void Procedure_addCodeTableEntry(ILProcedure* proc, uint32_t line,
		uint32_t charPos, uint32_t offset, uint32_t end, int32_t file) {
	proc->codeTable.length++;
	proc->codeTable.table = realloc(proc->codeTable.table,
			sizeof(CodeTableEntry) * proc->codeTable.length);
	size_t i = proc->codeTable.length - 1;
	proc->codeTable.table[i].line = line;
	proc->codeTable.table[i].charPos = charPos;
	proc->codeTable.table[i].offset = offset;
	proc->codeTable.table[i].end = end;
	proc->codeTable.table[i].file = file;
}
CodeTableEntry* Procedure_getCodeTableEntry(ILProcedure* proc, uint32_t pc) {
	CodeTableEntry* entry = NULL;
	for (size_t i = 0; i < proc->codeTable.length; i++) {
		CodeTableEntry* ent = &proc->codeTable.table[i];
		if (ent->offset <= pc && pc < ent->end) {
			if (entry == NULL)
				entry = ent;
			if (entry != NULL && ent->offset > entry->offset)
				entry = ent;
		}
	}
	return entry;
}

void Procedure_free(struct ILProcedure* proc, struct ILBytecode* bc) {
	MUTEX_LOCK(&bc->access_mutex);
	bc->procedures[proc->id] = NULL;
	free(proc->code);
	free(proc->labels.table);
	free(proc->codeTable.table);
	if (proc->compiled!=NULL)
		proc->compiled->free(proc->compiled);
	free(proc);
	MUTEX_UNLOCK(&bc->access_mutex);
}

void Procedure_mark(YoyoObject* ptr) {
	ptr->marked = true;
	ILProcedure* proc = (ILProcedure*) ptr;
	MARK(proc->bytecode);
}

void ILProcedure_free(YoyoObject* ptr) {
	ILProcedure* proc = (ILProcedure*) ptr;
	proc->free(proc, proc->bytecode);	
}

ILProcedure* Bytecode_newProcedure(ILBytecode* bc) {
	ILProcedure* proc = malloc(sizeof(ILProcedure));
	MUTEX_LOCK(&bc->access_mutex);

	proc->id = -1;

	for (size_t i = 0; i < bc->procedure_count; i++)
		if (bc->procedures[i] == NULL)
			proc->id = i;

	if (proc->id == -1) {
		proc->id = (int32_t) bc->procedure_count;
		bc->procedure_count++;
		bc->procedures = realloc(bc->procedures,
				sizeof(ILProcedure*) * bc->procedure_count);
	}

	bc->procedures[proc->id] = proc;

	proc->regc = 0;
	proc->code_length = 0;
	proc->code = NULL;

	proc->labels.table = NULL;
	proc->labels.length = 0;
	proc->codeTable.table = NULL;
	proc->codeTable.length = 0;

	proc->appendCode = Procedure_appendCode;
	proc->addLabel = Procedure_addLabel;
	proc->getLabel = Procedure_getLabel;
	proc->addCodeTableEntry = Procedure_addCodeTableEntry;
	proc->getCodeTableEntry = Procedure_getCodeTableEntry;
	proc->free = Procedure_free;

	proc->bytecode = bc;

	proc->compiled = NULL;
	proc->stats = NULL;

	initYoyoObject((YoyoObject*) proc, Procedure_mark, ILProcedure_free);

	MUTEX_UNLOCK(&bc->access_mutex);
	return proc;
}

int32_t Bytecode_getSymbolId(ILBytecode* bc, wchar_t* wstr) {
	return getSymbolId(bc->symbols, wstr);
}

wchar_t* Bytecode_getSymbolById(ILBytecode* bc, int32_t key) {
	return getSymbolById(bc->symbols, key);
}

int32_t Bytecode_addIntegerConstant(ILBytecode* bc, int64_t cnst) {
	for (size_t i = 0; i < bc->constants.size; i++)
		if (bc->constants.pool[i]->type == IntegerC
				&& bc->constants.pool[i]->value.i64 == cnst)
			return bc->constants.pool[i]->id;
	bc->constants.pool = realloc(bc->constants.pool,
			sizeof(Constant*) * (bc->constants.size + 1));
	Constant* out = malloc(sizeof(Constant));
	out->id = bc->constants.size;
	out->type = IntegerC;
	out->value.i64 = cnst;
	bc->constants.pool[out->id] = (Constant*) out;
	return bc->constants.size++;
}

int32_t Bytecode_addFloatConstant(ILBytecode* bc, double cnst) {
	for (size_t i = 0; i < bc->constants.size; i++)
		if (bc->constants.pool[i]->type == FloatC
				&& bc->constants.pool[i]->value.fp64 == cnst)
			return bc->constants.pool[i]->id;
	bc->constants.pool = realloc(bc->constants.pool,
			sizeof(Constant*) * (bc->constants.size + 1));
	Constant* out = malloc(sizeof(Constant));
	out->id = bc->constants.size;
	out->type = FloatC;
	out->value.fp64 = cnst;
	bc->constants.pool[out->id] = (Constant*) out;
	return bc->constants.size++;
}

int32_t Bytecode_addBooleanConstant(ILBytecode* bc, bool cnst) {
	for (size_t i = 0; i < bc->constants.size; i++)
		if (bc->constants.pool[i]->type == BooleanC
				&& bc->constants.pool[i]->value.boolean == cnst)
			return bc->constants.pool[i]->id;
	bc->constants.pool = realloc(bc->constants.pool,
			sizeof(Constant*) * (bc->constants.size + 1));
	Constant* out = malloc(sizeof(Constant));
	out->id = bc->constants.size;
	out->type = BooleanC;
	out->value.boolean = cnst;
	bc->constants.pool[out->id] = (Constant*) out;
	return bc->constants.size++;
}

int32_t Bytecode_addStringConstant(ILBytecode* bc, wchar_t* wstr1) {
	wchar_t* wstr = malloc(sizeof(wchar_t) * (wcslen(wstr1) + 1));
	wcscpy(wstr, wstr1);
	wstr[wcslen(wstr1)] = L'\0';
	int32_t cnst = bc->getSymbolId(bc, wstr);
	free(wstr);
	for (size_t i = 0; i < bc->constants.size; i++)
		if (bc->constants.pool[i]->type == StringC
				&& bc->constants.pool[i]->value.string_id == cnst)
			return bc->constants.pool[i]->id;
	bc->constants.pool = realloc(bc->constants.pool,
			sizeof(Constant*) * (bc->constants.size + 1));
	Constant* out = malloc(sizeof(Constant));
	out->id = bc->constants.size;
	out->type = StringC;
	out->value.string_id = cnst;
	bc->constants.pool[out->id] = (Constant*) out;
	return bc->constants.size++;
}

int32_t Bytecode_getNullConstant(ILBytecode* bc) {
	for (size_t i = 0; i < bc->constants.size; i++)
		if (bc->constants.pool[i]->type == NullC)
			return bc->constants.pool[i]->id;
	bc->constants.pool = realloc(bc->constants.pool,
			sizeof(Constant*) * (bc->constants.size + 1));
	Constant* out = malloc(sizeof(Constant));
	out->id = bc->constants.size;
	out->type = NullC;
	bc->constants.pool[out->id] = out;
	return bc->constants.size++;
}

Constant* Bytecode_getConstant(ILBytecode* bc, int32_t id) {
	for (size_t i = 0; i < bc->constants.size; i++)
		if (bc->constants.pool[i]->id == id)
			return bc->constants.pool[i];
	return NULL;
}

void Bytecode_mark(YoyoObject* ptr) {
	ptr->marked = true;
	ILBytecode* bc = (ILBytecode*) ptr;
	for (size_t i = 0; i < bc->procedure_count; i++)
		MARK(bc->procedures[i]);
}

ILBytecode* newBytecode(SymbolMap* sym) {
	ILBytecode* bc = malloc(sizeof(ILBytecode));

	bc->procedure_count = 0;
	bc->procedures = NULL;

	bc->constants.pool = NULL;
	bc->constants.size = 0;

	bc->symbols = sym;

	bc->newProcedure = Bytecode_newProcedure;
	bc->getSymbolId = Bytecode_getSymbolId;
	bc->getSymbolById = Bytecode_getSymbolById;

	bc->addIntegerConstant = Bytecode_addIntegerConstant;
	bc->addFloatConstant = Bytecode_addFloatConstant;
	bc->addBooleanConstant = Bytecode_addBooleanConstant;
	bc->addStringConstant = Bytecode_addStringConstant;
	bc->getNullConstant = Bytecode_getNullConstant;
	bc->getConstant = Bytecode_getConstant;

	initYoyoObject((YoyoObject*) bc, Bytecode_mark, (void (*)(YoyoObject*)) freeBytecode);

	NEW_MUTEX(&bc->access_mutex);
	return bc;
}
