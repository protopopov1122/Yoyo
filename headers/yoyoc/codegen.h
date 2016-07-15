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

#ifndef YOYOC_CODEGEN_H
#define YOYOC_CODEGEN_H

#include "yparse.h"

typedef struct YCodeGen YCodeGen;

typedef struct YModifier {
	void (*setter)(struct YModifier*, struct YCodeGen*, YoyoCEnvironment*, bool,
			int32_t);
	void (*remover)(struct YModifier*, struct YCodeGen*, YoyoCEnvironment*);
	void (*typeSetter)(struct YModifier*, struct YCodeGen*, YoyoCEnvironment*,
			int32_t);
	void (*free)(struct YModifier*);
} YModifier;

typedef struct ProcedureBuilderLoop {
	int32_t id;

	int32_t start;
	int32_t end;

	struct ProcedureBuilderLoop* prev;
} ProcdeureBuilderLoop;

typedef struct ProcedureBuilder {
	ILProcedure* proc;

	int32_t (*nextLabel)(struct ProcedureBuilder*);
	int32_t (*nextRegister)(struct ProcedureBuilder*);
	void (*unuse)(struct ProcedureBuilder*, int32_t);
	void (*append)(struct ProcedureBuilder*, uint8_t, int32_t, int32_t,
			int32_t);
	void (*bind)(struct ProcedureBuilder*, int32_t);
	ProcdeureBuilderLoop* (*startLoop)(struct ProcedureBuilder*, int32_t, int32_t, int32_t);
	void (*endLoop)(struct ProcedureBuilder*);
	ProcdeureBuilderLoop* (*getLoop)(struct ProcedureBuilder*, int32_t);

	int32_t nLabel;
	uint8_t* regs;
	size_t regs_length;
	size_t regc;
	ProcdeureBuilderLoop* loop;

	struct ProcedureBuilder* prev;
} ProcedureBuilder;

typedef struct YCodeGen {
	ILBytecode* bc;

	ProcedureBuilder* (*newProcedure)(struct YCodeGen*);
	void (*endProcedure)(struct YCodeGen*);

	ProcedureBuilder* proc;

	FILE* err_stream;
} YCodeGen;

#define CompilationError(file, msg, node, bc) fprintf(file, "%ls at %ls(%" PRId32 ":%" PRId32 ")\n", msg,\
	bc->getSymbolById(bc, node->fname), node->line, node->charPos);

int32_t ycompile(YoyoCEnvironment*, YNode*, FILE*);
YModifier* ymodifier(YCodeGen*, YoyoCEnvironment*, YNode*);

#endif
