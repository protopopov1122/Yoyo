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

#include "types/types.h"

wchar_t* Object_toString(YValue* vobj, YThread* th) {
	YObject* obj = (YObject*) vobj;
	char* fmt = "Object@%p";
	size_t sz = snprintf(NULL, 0, fmt, obj);
	char* cstr = malloc(sizeof(char) * sz);
	sprintf(cstr, fmt, obj);
	wchar_t* wstr = malloc(sizeof(wchar_t) * strlen(cstr));
	mbstowcs(wstr, cstr, sizeof(wchar_t) * strlen(cstr));
	free(cstr);
	return wstr;
}
uint64_t Object_hashCode(YValue* vobj, YThread* th) {
	YObject* obj = (YObject*) vobj;
	int32_t mid = getSymbolId(&th->runtime->symbols,
	HASHCODE);
	if (obj->contains(obj, mid, th)) {
		YValue* val = obj->get(obj, mid, th);
		if (val->type->type == LambdaT) {
			YLambda* lmbd = (YLambda*) val;
			YValue* out = invokeLambda(lmbd, NULL, NULL, 0, th);
			if (out->type->type == IntegerT)
				return (uint64_t) ((YInteger*) out)->value;
		}
	}
	return Common_hashCode(vobj, th);
}
YValue* Object_readIndex(YValue* o, YValue* index, YThread* th) {
	YObject* obj = (YObject*) o;
	if (obj->contains(obj, getSymbolId(&th->runtime->symbols,
	READ_INDEX), th)) {
		YValue* val = obj->get(obj, getSymbolId(&th->runtime->symbols,
		READ_INDEX), th);
		if (val->type->type == LambdaT) {
			YLambda* lambda = (YLambda*) val;
			return invokeLambda(lambda, NULL, &index, 1, th);
		}
	}
	return getNull(th);
}
YValue* Object_writeIndex(YValue* o, YValue* index, YValue* value, YThread* th) {
	YObject* obj = (YObject*) o;
	if (obj->contains(obj, getSymbolId(&th->runtime->symbols,
	WRITE_INDEX), th)) {
		YValue* val = obj->get(obj, getSymbolId(&th->runtime->symbols,
		WRITE_INDEX), th);
		if (val->type->type == LambdaT) {
			YLambda* lambda = (YLambda*) val;
			YValue* args[] = { index, value };
			return invokeLambda(lambda, NULL, args, 2, th);
		}
	}
	return getNull(th);
}
YoyoIterator* Object_iterator(YValue* v, YThread* th) {
	YObject* obj = (YObject*) v;
	if (obj->iterator)
		return (YoyoIterator*) obj;
	YoyoIterator* iter = NULL;
	int32_t id = getSymbolId(&th->runtime->symbols, L"iter");
	if (obj->contains(obj, id, th)) {
		YValue* val = obj->get(obj, id, th);
		if (val->type->type == LambdaT) {
			YLambda* exec = (YLambda*) val;
			YValue* out = invokeLambda(exec, NULL, NULL, 0, th);
			if (out->type->type == ObjectT && ((YObject*) out)->iterator)
				iter = (YoyoIterator*) out;
			else if (out->type->type == ObjectT)
				iter = newYoyoIterator((YObject*) out, th);
		}
	} else
		iter = newYoyoIterator(obj, th);
	return iter;
}

YValue* Object_readProperty(int32_t id, YValue* v, YThread* th) {
	YObject* obj = (YObject*) v;
	if (!obj->contains(obj, id, th)) {
		YValue* val = Common_readProperty(id, v, th);
		if (val != NULL)
			return val;
	}
	return obj->get(obj, id, th);
}

void Object_type_init(YRuntime* runtime) {
	runtime->ObjectType.type = ObjectT;
	runtime->ObjectType.TypeConstant = newAtomicType(ObjectT,
			runtime->CoreThread);
	runtime->ObjectType.oper.add_operation = concat_operation;
	runtime->ObjectType.oper.subtract_operation = undefined_binary_operation;
	runtime->ObjectType.oper.multiply_operation = undefined_binary_operation;
	runtime->ObjectType.oper.divide_operation = undefined_binary_operation;
	runtime->ObjectType.oper.modulo_operation = undefined_binary_operation;
	runtime->ObjectType.oper.power_operation = undefined_binary_operation;
	runtime->ObjectType.oper.and_operation = undefined_binary_operation;
	runtime->ObjectType.oper.or_operation = undefined_binary_operation;
	runtime->ObjectType.oper.xor_operation = undefined_binary_operation;
	runtime->ObjectType.oper.shr_operation = undefined_binary_operation;
	runtime->ObjectType.oper.shl_operation = undefined_binary_operation;
	runtime->ObjectType.oper.compare = compare_operation;
	runtime->ObjectType.oper.negate_operation = undefined_unary_operation;
	runtime->ObjectType.oper.not_operation = undefined_unary_operation;
	runtime->ObjectType.oper.toString = Object_toString;
	runtime->ObjectType.oper.readProperty = Object_readProperty;
	runtime->ObjectType.oper.hashCode = Object_hashCode;
	runtime->ObjectType.oper.readIndex = Object_readIndex;
	runtime->ObjectType.oper.writeIndex = Object_writeIndex;
	runtime->ObjectType.oper.subseq = NULL;
	runtime->ObjectType.oper.iterator = Object_iterator;
}
