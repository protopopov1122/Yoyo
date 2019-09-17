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

#include "types.h"

uint64_t Object_hashCode(YValue* vobj, YThread* th) {
	YObject* obj = (YObject*) vobj;
	int32_t mid = getSymbolId(&th->runtime->symbols,
	HASHCODE);
	if (obj->contains(obj, mid, th)) {
		YValue* val = obj->get(obj, mid, th);
		if (val->type == &th->runtime->LambdaType) {
			YLambda* lmbd = (YLambda*) val;
			YValue* out = invokeLambda(lmbd, obj, NULL, 0, th);
			if (out->type == &th->runtime->IntType)
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
		if (val->type == &th->runtime->LambdaType) {
			YLambda* lambda = (YLambda*) val;
			return invokeLambda(lambda, obj, &index, 1, th);
		}
	} else {
		wchar_t* wcs = toString(index, th);
		YValue* val = OBJECT_GET(obj, wcs, th);
		free(wcs);
		return val;
	}
	return getNull(th);
}
YValue* Object_writeIndex(YValue* o, YValue* index, YValue* value, YThread* th) {
	YObject* obj = (YObject*) o;
	if (obj->contains(obj, getSymbolId(&th->runtime->symbols,
	WRITE_INDEX), th)) {
		YValue* val = obj->get(obj, getSymbolId(&th->runtime->symbols,
		WRITE_INDEX), th);
		if (val->type == &th->runtime->LambdaType) {
			YLambda* lambda = (YLambda*) val;
			YValue* args[] = { index, value };
			return invokeLambda(lambda, obj, args, 2, th);
		}
	} else {
		wchar_t* wcs = toString(index, th);
		OBJECT_PUT(obj, wcs, value, th);
		free(wcs);
		return getNull(th);
	}
	return getNull(th);
}
YValue* Object_removeIndex(YValue* o, YValue* index, YThread* th) {
	YObject* obj = (YObject*) o;
	if (obj->contains(obj, getSymbolId(&th->runtime->symbols,
	REMOVE_INDEX), th)) {
		YValue* val = obj->get(obj, getSymbolId(&th->runtime->symbols,
		REMOVE_INDEX), th);
		if (val->type == &th->runtime->LambdaType) {
			YLambda* lambda = (YLambda*) val;
			return invokeLambda(lambda, obj, &index, 1, th);
		}
	} else {
		wchar_t* wcs = toString(index, th);
		OBJECT_REMOVE(obj, wcs, th);
		free(wcs);
		return getNull(th);
	}	
	return getNull(th);
}
YoyoIterator* Object_iterator(YValue* v, YThread* th) {
	YObject* obj = (YObject*) v;
	YoyoIterator* iter = NULL;
	int32_t id = getSymbolId(&th->runtime->symbols, L"iter");
	if (obj->contains(obj, id, th)) {
		YValue* val = obj->get(obj, id, th);
		if (val->type == &th->runtime->LambdaType) {
			YLambda* exec = (YLambda*) val;
			YValue* out = invokeLambda(exec, obj, NULL, 0, th);
			if (out->type == &th->runtime->ObjectType)
				iter = newYoyoIterator((YObject*) out, th);
			else if (out->type == IteratorType)
				iter = (YoyoIterator*) out;
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
	YThread* th = yoyo_thread(runtime);
	runtime->ObjectType.wstring = L"object";
	runtime->ObjectType.TypeConstant = newAtomicType(&th->runtime->ObjectType,
			th);
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
	runtime->ObjectType.oper.toString = Common_toString;
	runtime->ObjectType.oper.readProperty = Object_readProperty;
	runtime->ObjectType.oper.hashCode = Object_hashCode;
	runtime->ObjectType.oper.readIndex = Object_readIndex;
	runtime->ObjectType.oper.writeIndex = Object_writeIndex;
	runtime->ObjectType.oper.removeIndex = Object_removeIndex;
	runtime->ObjectType.oper.subseq = NULL;
	runtime->ObjectType.oper.iterator = Object_iterator;
}
