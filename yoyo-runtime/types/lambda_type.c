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

#include "types.h"

wchar_t* Lambda_toString(YValue* obj, YThread* th) {
	YLambda* l = (YLambda*) obj;
	wchar_t* src = l->signature(l, th)->string;
	wchar_t* out = calloc(1, sizeof(wchar_t) * (wcslen(src) + 1));
	wcscpy(out, src);
	return out;
}

bool Lambda_equals(YValue* v1, YValue* v2, YThread* th) {
	return newBoolean(v1 == v2, th);
}

YOYO_FUNCTION(Lambda_signature) {
	YLambda* lmbd = (YLambda*) ((NativeLambda*) lambda)->object;
	return (YValue*) lmbd->signature(lmbd, th);
}

YOYO_FUNCTION(Lambda_call) {
	YLambda* lmbd = (YLambda*) ((NativeLambda*) lambda)->object;
	return invokeLambda(lmbd, NULL, args, argc, th);
}

YOYO_FUNCTION(Lambda_callArray) {
	YLambda* lmbd = (YLambda*) ((NativeLambda*) lambda)->object;
	if (args[0]->type != &th->runtime->ArrayType)
		return getNull(th);
	YArray* array = (YArray*) args[0];
	YValue** arr_args = malloc(sizeof(YValue*) * array->size(array, th));
	for (size_t i = 0; i < array->size(array, th); i++)
		arr_args[i] = array->get(array, i, th);
	YValue* ret = invokeLambda(lmbd, NULL, arr_args, array->size(array, th),
			th);
	free(arr_args);
	return ret;
}

typedef struct CurriedLambda {
	YoyoObject o;
	YLambda* lambda;
	size_t argc;
	YValue** args;
} CurriedLambda;

void CurriedLambda_mark(YoyoObject* ptr) {
	ptr->marked = true;
	CurriedLambda* lambda = (CurriedLambda*) ptr;
	MARK(lambda);
	for (size_t i=0;i<lambda->argc;i++)
		MARK(lambda->args[i]);
}

void CurriedLambda_free(YoyoObject* ptr) {
	CurriedLambda* lambda = (CurriedLambda*) ptr;
	free(lambda->args);
	free(lambda);
}

YOYO_FUNCTION(CurriedLambda_exec) {
	CurriedLambda* cl = (CurriedLambda*) ((NativeLambda*) lambda)->object;
	size_t cargc = argc + cl->argc;
	if (cargc == cl->lambda->sig->argc ||
		(cl->lambda->sig->method && cargc - 1 == cl->lambda->sig->argc)) {
		YValue** cargs = calloc(cargc, sizeof(YValue*));
		memcpy(cargs, cl->args, sizeof(YValue*) * cl->argc);
		memcpy(&cargs[cl->argc], args, sizeof(YValue*) * argc);
		YValue* res = cl->lambda->execute(cl->lambda, NULL, cargs, cargc, th);
		free(cargs);
		return res;
	}
	CurriedLambda* ncl = malloc(sizeof(CurriedLambda));
	ncl->lambda = cl->lambda;
	ncl->argc = cl->argc + argc;
	ncl->args = calloc(ncl->argc, sizeof(YValue*));
	memcpy(ncl->args, cl->args, sizeof(YValue*) * cl->argc);
	memcpy(&ncl->args[cl->argc], args, sizeof(YValue*) * argc);
	initYoyoObject((YoyoObject*) ncl, CurriedLambda_mark, CurriedLambda_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) ncl);
	return (YValue*) newNativeLambda(-1, CurriedLambda_exec, (YoyoObject*) ncl, th);
}

YOYO_FUNCTION(Lambda_curry) {
	YLambda* lmbd = (YLambda*) ((NativeLambda*) lambda)->object;
	if (lmbd->sig->vararg) {
		throwException(L"CurryingVarargLambda", NULL, 0, th);
		return getNull(th);
	}
	if (lmbd->sig->argc < 2)
		return (YValue*) lmbd;
	CurriedLambda* cl = malloc(sizeof(CurriedLambda));
	cl->lambda = lmbd;
	cl->argc = argc;
	cl->args = calloc(argc, sizeof(YValue*));
	memcpy(cl->args, args, sizeof(YValue*) * argc);
	initYoyoObject((YoyoObject*) cl, CurriedLambda_mark, CurriedLambda_free);
	th->runtime->gc->registrate(th->runtime->gc, (YoyoObject*) cl);
	YLambda* out = newNativeLambda(-1, CurriedLambda_exec, (YoyoObject*) cl, th);
	return (YValue*) out;
}

YValue* Lambda_readProperty(int32_t key, YValue* v, YThread* th) {
	NEW_METHOD(L"signature", Lambda_signature, 0, v);
	NEW_METHOD(L"call", Lambda_call, -1, v);
	NEW_METHOD(L"callArray", Lambda_callArray, 1, v);
	NEW_METHOD(L"curry", Lambda_curry, -1, v);
	return Common_readProperty(key, v, th);
}

void Lambda_type_init(YRuntime* runtime) {
	YThread* th = yoyo_thread(runtime);
	runtime->LambdaType.wstring = L"lambda";
	runtime->LambdaType.TypeConstant = newAtomicType(&th->runtime->LambdaType,
			th);
	runtime->LambdaType.oper.add_operation = concat_operation;
	runtime->LambdaType.oper.subtract_operation = undefined_binary_operation;
	runtime->LambdaType.oper.multiply_operation = undefined_binary_operation;
	runtime->LambdaType.oper.divide_operation = undefined_binary_operation;
	runtime->LambdaType.oper.modulo_operation = undefined_binary_operation;
	runtime->LambdaType.oper.power_operation = undefined_binary_operation;
	runtime->LambdaType.oper.and_operation = undefined_binary_operation;
	runtime->LambdaType.oper.or_operation = undefined_binary_operation;
	runtime->LambdaType.oper.xor_operation = undefined_binary_operation;
	runtime->LambdaType.oper.shr_operation = undefined_binary_operation;
	runtime->LambdaType.oper.shl_operation = undefined_binary_operation;
	runtime->LambdaType.oper.compare = compare_operation;
	runtime->LambdaType.oper.negate_operation = undefined_unary_operation;
	runtime->LambdaType.oper.not_operation = undefined_unary_operation;
	runtime->LambdaType.oper.toString = Lambda_toString;
	runtime->LambdaType.oper.readProperty = Lambda_readProperty;
	runtime->LambdaType.oper.hashCode = Common_hashCode;
	runtime->LambdaType.oper.readIndex = NULL;
	runtime->LambdaType.oper.writeIndex = NULL;
	runtime->LambdaType.oper.removeIndex = NULL;
	runtime->LambdaType.oper.subseq = NULL;
	runtime->LambdaType.oper.iterator = NULL;
}
