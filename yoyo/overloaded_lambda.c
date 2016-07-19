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

#include "../headers/yoyo/value.h"
#include "yoyo/interpreter.h"

typedef struct OverloadedLambda {
	YLambda parent;

	YLambda** lambdas;
	size_t count;
	YLambda* defLambda;
} OverloadedLambda;

void OverloadedLambda_free(YoyoObject* ptr) {
	OverloadedLambda* lmbd = (OverloadedLambda*) ptr;
	free(lmbd->lambdas);
	free(lmbd);
}

void OverloadedLambda_mark(YoyoObject* ptr) {
	ptr->marked = true;
	OverloadedLambda* lmbd = (OverloadedLambda*) ptr;
	for (size_t i = 0; i < lmbd->count; i++)
		if (lmbd->lambdas[i] != NULL)
			MARK(lmbd->lambdas[i]);
	if (lmbd->defLambda != NULL) {
		MARK(lmbd->defLambda);
	}
	MARK(lmbd->parent.sig);
}

YValue* OverloadedLambda_exec(YLambda* l, YValue** args, size_t argc,
		YThread* th) {
	OverloadedLambda* lmbd = (OverloadedLambda*) l;
	for (size_t i = 0; i < lmbd->count; i++) {
		if (lmbd->lambdas[i] != NULL && lmbd->lambdas[i]->sig->vararg
				&& lmbd->lambdas[i]->sig->argc - 1 <= argc) {

			l = lmbd->lambdas[i];
			return invokeLambda(l, args, argc, th);
		}
		if (lmbd->lambdas[i] != NULL && lmbd->lambdas[i]->sig->argc == argc)
			return invokeLambda(lmbd->lambdas[i], args, argc, th);

	}
	if (lmbd->defLambda == NULL) {
		throwException(L"LambdaArgumentMismatch", NULL, 0, th);
		return getNull(th);
	} else {
		YArray* arr = newArray(th);
		for (size_t i = argc - 1; i < argc; i--)
			arr->add(arr, args[i], th);
		return invokeLambda(lmbd->defLambda, (YValue**) &arr, 1, th);
	}
}

YoyoType* OverloadedLambda_signature(YLambda* l, YThread* th) {
	OverloadedLambda* ovl = (OverloadedLambda*) l;
	size_t len = ovl->count + (ovl->defLambda != NULL ? 1 : 0);
	YoyoType** types = malloc(sizeof(YoyoType*) * len);
	for (size_t i = 0; i < ovl->count; i++)
		types[i] = ovl->lambdas[i]->signature(ovl->lambdas[i], th);
	if (ovl->defLambda != NULL)
		types[ovl->count] = ovl->defLambda->signature(ovl->defLambda, th);
	YoyoType* out = newTypeMix(types, len, th);
	free(types);
	return out;
}

YLambda* newOverloadedLambda(YLambda** l, size_t c, YLambda* def, YThread* th) {
	OverloadedLambda* lmbd = malloc(sizeof(OverloadedLambda));
	initYoyoObject((YoyoObject*) lmbd, OverloadedLambda_mark,
			OverloadedLambda_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) lmbd);
	lmbd->count = c;
	lmbd->lambdas = malloc(sizeof(YLambda*) * c);
	for (size_t i = 0; i < c; i++)
		lmbd->lambdas[i] = l[i];
	lmbd->parent.sig = newLambdaSignature(-1, false, NULL, NULL, th);
	lmbd->parent.signature = OverloadedLambda_signature;
	lmbd->parent.execute = OverloadedLambda_exec;
	lmbd->parent.parent.type = &th->runtime->LambdaType;
	lmbd->defLambda = def;
	return (YLambda*) lmbd;
}
