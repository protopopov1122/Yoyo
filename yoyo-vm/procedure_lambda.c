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

#include "yoyo-runtime.h"
#include "interpreter.h"

/*File contains implementation of lambda
 * interface. Procedure lambda contains
 * id of Yoyo bytecode procedure, scope
 * and arguments ids. When lambda is called
 * C procedure create new scope based
 * on lambda scope, add all arguments
 * with appropriate argument id in it
 * and call Yoyo procedure with this scope
 * (using invoke in 'interpreter.c')*/

typedef struct ProcedureLambda {
	YLambda lambda;

	int32_t procid;
	int32_t* argids;
	YObject* scope;
	ILBytecode* bytecode;
} ProcedureLambda;

void ProcedureLambda_mark(YoyoObject* ptr) {
	ptr->marked = true;
	ProcedureLambda* lmbd = (ProcedureLambda*) ptr;
	MARK(lmbd->scope);
	MARK(lmbd->lambda.sig);
	MARK(lmbd->bytecode);
}

void ProcedureLambda_free(YoyoObject* ptr) {
	ProcedureLambda* lmbd = (ProcedureLambda*) ptr;
	free(lmbd->argids);
	free(lmbd);
}

YValue* ProcedureLambda_exec(YLambda* l, YObject* scp, YValue** args,
		size_t argc, YThread* th) {
	ProcedureLambda* lam = (ProcedureLambda*) l;
	if (argc != l->sig->argc)
		return getNull(th);
	YObject* scope = th->runtime->newObject(lam->scope, th);
	if (scp != NULL) {
		OBJECT_NEW(scope, L"self", scp, th);
	}
	for (size_t i = 0; i < argc; i++) {
		scope->put(scope, lam->argids[i], args[i], true, th);
		scope->setType(scope, lam->argids[i], l->sig->args[i], th);
	}
	return invoke(lam->procid, lam->bytecode, scope, l->sig->ret, th);
}

YoyoType* ProcedureLambda_signature(YLambda* l, YThread* th) {
	return (YoyoType*) l->sig;
}

YLambda* newProcedureLambda(int32_t procid, ILBytecode* bc, YObject* scope,
		int32_t* argids, YoyoLambdaSignature* sig, YThread* th) {
	ProcedureLambda* lmbd = calloc(1, sizeof(ProcedureLambda));

	initYoyoObject(&lmbd->lambda.parent.o, ProcedureLambda_mark,
			ProcedureLambda_free);
	th->runtime->gc->registrate(th->runtime->gc, &lmbd->lambda.parent.o);

	lmbd->lambda.parent.type = &th->runtime->LambdaType;
	lmbd->lambda.sig = sig;
	lmbd->lambda.signature = ProcedureLambda_signature;

	lmbd->bytecode = bc;
	lmbd->procid = procid;
	lmbd->scope = scope;
	lmbd->argids = malloc(sizeof(int32_t) * sig->argc);
	for (size_t i = 0; i < sig->argc; i++)
		lmbd->argids[i] = argids[i];

	lmbd->lambda.execute = ProcedureLambda_exec;


	return (YLambda*) lmbd;
}
