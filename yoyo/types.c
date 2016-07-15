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

#include "yoyo.h"

typedef struct AtomicType {
	struct YoyoType type;

	ValueType atomic;
} AtomicType;

bool AtomicType_verify(YoyoType* t, YValue* value, YThread* th) {
	AtomicType* at = (AtomicType*) t;
	return at->atomic == value->type->type || at->atomic == AnyT
			|| value->type->type == AnyT;
}
bool AtomicType_compatible(YoyoType* t1, YoyoType* t2, YThread* th) {
	return ((AtomicType*) t1)->atomic == AnyT
			|| (t2->type == AtomicDT
					&& ((AtomicType*) t2)->atomic == ((AtomicType*) t1)->atomic);
}

void AtomicType_mark(HeapObject* ptr) {
	ptr->marked = true;
}
void AtomicType_free(HeapObject* ptr) {
	free(ptr);
}

YoyoType* newAtomicType(ValueType a, YThread* th) {
	AtomicType* type = calloc(1, sizeof(AtomicType));
	initHeapObject((HeapObject*) type, AtomicType_mark, AtomicType_free);
	th->runtime->gc->registrate(th->runtime->gc, (HeapObject*) type);
	type->type.parent.type = &th->runtime->DeclarationType;

	type->atomic = a;
	type->type.verify = AtomicType_verify;
	type->type.compatible = AtomicType_compatible;
	type->type.type = AtomicDT;
	switch (a) {
	case IntegerT:
		type->type.string = L"int";
		break;

	case FloatT:
		type->type.string = L"float";
		break;

	case BooleanT:
		type->type.string = L"boolean";
		break;

	case StringT:
		type->type.string = L"string";
		break;

	case LambdaT:
		type->type.string = L"lambda";
		break;

	case ArrayT:
		type->type.string = L"array";
		break;

	case ObjectT:
		type->type.string = L"object";
		break;

	case AnyT:
		type->type.string = L"any";
		break;

	case DeclarationT:
		type->type.string = L"declaration";
		break;
	}

	return (YoyoType*) type;
}

YoyoType* Interface_get(YoyoInterface* yi, int32_t id) {
	for (size_t i = 0; i < yi->attr_count; i++)
		if (yi->attrs[i].id == id)
			return yi->attrs[i].type;
	for (size_t i = 0; i < yi->parent_count; i++) {
		YoyoType* tp = Interface_get(yi->parents[i], id);
		if (tp != NULL)
			return tp;
	}
	return NULL;
}

bool Interface_verify(YoyoType* t, YValue* v, YThread* th) {
	YoyoInterface* i = (YoyoInterface*) t;
	if (v->type->type == AnyT)
		return true;
	for (size_t j = 0; j < i->parent_count; j++)
		if (!i->parents[j]->type.verify((YoyoType*) i->parents[j], v, th))
			return false;
	if (i->attr_count == 0)
		return true;
	if (v->type->type != ObjectT)
		return false;
	YObject* obj = (YObject*) v;
	for (size_t j = 0; j < i->attr_count; j++) {
		YoyoAttribute* attr = &i->attrs[j];
		if (!obj->contains(obj, attr->id, th))
			return false;
		YValue* val = obj->get(obj, attr->id, th);
		if (!attr->type->verify(attr->type, val, th))
			return false;
	}
	return true;
}
bool Interface_compatible(YoyoType* t1, YoyoType* t2, YThread* th) {
	YoyoInterface* i1 = (YoyoInterface*) t1;
	if (t2->type != InterfaceDT)
		return false;
	YoyoInterface* i2 = (YoyoInterface*) t2;
	for (size_t i = 0; i < i1->parent_count; i++)
		if (!i1->parents[i]->type.compatible((YoyoType*) i1->parents[i], t2,
				th))
			return false;
	for (size_t i = 0; i < i1->attr_count; i++) {
		YoyoAttribute* attr1 = &i1->attrs[i];
		YoyoType* type2 = Interface_get(i2, attr1->id);
		if (type2 == NULL || !attr1->type->compatible(attr1->type, type2, th))
			return false;
	}
	return true;
}

void InterfaceType_mark(HeapObject* ptr) {
	ptr->marked = true;
	YoyoInterface* i = (YoyoInterface*) ptr;
	for (size_t j = 0; j < i->parent_count; j++)
		MARK(i->parents[j]);
	for (size_t j = 0; j < i->attr_count; j++)
		MARK(i->attrs[j].type);
}
void InterfaceType_free(HeapObject* t) {
	YoyoInterface* i = (YoyoInterface*) t;
	free(i->parents);
	free(i->attrs);
	free(i->type.string);
	free(i);
}

YoyoType* newInterface(YoyoInterface** parents, size_t pcount,
		YoyoAttribute* attrs, size_t count, YThread* th) {
	YoyoInterface* i = calloc(1, sizeof(YoyoInterface));
	initHeapObject((HeapObject*) i, InterfaceType_mark, InterfaceType_free);
	th->runtime->gc->registrate(th->runtime->gc, (HeapObject*) i);
	i->type.parent.type = &th->runtime->DeclarationType;

	size_t realPCount = 0;
	for (size_t i = 0; i < pcount; i++)
		if (parents[i] != NULL)
			realPCount++;
	i->parents = malloc(sizeof(YoyoType*) * realPCount);
	i->parent_count = realPCount;
	size_t index = 0;
	for (size_t j = 0; j < pcount; j++)
		if (parents[j] != NULL)
			i->parents[index++] = parents[j];

	i->attrs = malloc(sizeof(YoyoAttribute) * count);
	i->attr_count = count;
	memcpy(i->attrs, attrs, sizeof(YoyoAttribute) * count);

	StringBuilder* sb = newStringBuilder(L"");
	if (pcount > 0) {
		for (size_t i = 0; i < pcount; i++) {
			sb->append(sb, parents[i]->type.string);
			if (i + 1 < pcount)
				sb->append(sb, L" & ");
		}
	}
	if (pcount > 0 && count > 0)
		sb->append(sb, L" & ");
	if (count > 0) {
		sb->append(sb, L"(");
		for (size_t i = 0; i < count; i++) {
			sb->append(sb, getSymbolById(&th->runtime->symbols, attrs[i].id));
			sb->append(sb, L": ");
			sb->append(sb, attrs[i].type->string);
			if (i + 1 < count)
				sb->append(sb, L"; ");
		}
		sb->append(sb, L")");
	}
	i->type.string = sb->string;
	free(sb);

	i->type.verify = Interface_verify;
	i->type.compatible = Interface_compatible;
	i->type.type = InterfaceDT;
	return (YoyoType*) i;
}

typedef struct YoyoTypeMix {
	YoyoType type;

	YoyoType** types;
	size_t length;
} YoyoTypeMix;

void TypeMix_mark(HeapObject* ptr) {
	ptr->marked = true;
	YoyoTypeMix* mix = (YoyoTypeMix*) ptr;
	for (size_t i = 0; i < mix->length; i++)
		MARK(mix->types[i]);
}
void TypeMix_free(HeapObject* ptr) {
	YoyoTypeMix* mix = (YoyoTypeMix*) ptr;
	free(mix->type.string);
	free(mix->types);
	free(mix);
}
bool TypeMix_verify(YoyoType* t, YValue* val, YThread* th) {
	if (val->type->type == AnyT)
		return true;
	YoyoTypeMix* mix = (YoyoTypeMix*) t;
	for (size_t i = 0; i < mix->length; i++)
		if (mix->types[i]->verify(mix->types[i], val, th))
			return true;
	return false;
}
bool TypeMix_compatible(YoyoType* t1, YoyoType* t2, YThread* th) {
	YoyoTypeMix* mix1 = (YoyoTypeMix*) t1;
	if (t2->type == TypeMixDT) {
		YoyoTypeMix* mix2 = (YoyoTypeMix*) t2;
		for (size_t i = 0; i < mix2->length; i++) {
			if (!t1->compatible(t1, mix2->types[i], th))
				return false;
		}
		return true;
	}
	for (size_t i = 0; i < mix1->length; i++)
		if (mix1->types[i]->compatible(mix1->types[i], t2, th))
			return true;
	return false;
}

YoyoType* newTypeMix(YoyoType** types, size_t len, YThread* th) {
	YoyoTypeMix* mix = malloc(sizeof(YoyoTypeMix));
	initHeapObject((HeapObject*) mix, TypeMix_mark, TypeMix_free);
	th->runtime->gc->registrate(th->runtime->gc, (HeapObject*) mix);
	mix->type.parent.type = &th->runtime->DeclarationType;

	StringBuilder* sb = newStringBuilder(L"");
	for (size_t i = 0; i < len; i++) {
		sb->append(sb, types[i]->string);
		if (i + 1 < len)
			sb->append(sb, L" | ");
	}
	mix->type.string = sb->string;
	free(sb);
	mix->type.verify = TypeMix_verify;
	mix->type.compatible = TypeMix_compatible;
	mix->type.type = TypeMixDT;

	mix->length = len;
	mix->types = malloc(sizeof(YoyoType*) * len);
	memcpy(mix->types, types, sizeof(YoyoType*) * len);

	return (YoyoType*) mix;
}

typedef struct ArrayType {
	YoyoType type;

	YoyoType** types;
	size_t count;
} ArrayType;

void ArrayType_mark(HeapObject* ptr) {
	ptr->marked = true;
	ArrayType* arr = (ArrayType*) ptr;
	for (size_t i = 0; i < arr->count; i++)
		MARK(arr->types[i]);
}
void ArrayType_free(HeapObject* ptr) {
	ArrayType* arr = (ArrayType*) ptr;
	free(arr->types);
	free(arr->type.string);
	free(arr);
}
bool ArrayType_verify(YoyoType* t, YValue* val, YThread* th) {
	ArrayType* arrt = (ArrayType*) t;
	if (val->type->type == AnyT)
		return true;
	if (val->type->type != ArrayT)
		return false;
	YArray* arr = (YArray*) val;
	size_t offset = 0;
	for (size_t i = 0; i < arr->size(arr, th); i++) {
		YoyoType* type = arrt->types[offset++];
		offset = offset % arrt->count;
		if (!type->verify(type, arr->get(arr, i, th), th))
			return false;
	}
	return true;
}
bool ArrayType_compatible(YoyoType* t1, YoyoType* t2, YThread* th) {
	ArrayType* arr1 = (ArrayType*) t1;
	if (t2->type != ArrayDT)
		return false;
	ArrayType* arr2 = (ArrayType*) t2;
	if (arr1->count == 0 || arr2->count == 0)
		return arr1->count == 0 && arr2->count == 0;
	size_t index = 0;
	for (size_t i = 0; i < arr1->count; i++) {
		if (!arr1->types[i]->compatible(arr1->types[i], arr2->types[index++],
				th))
			return false;
		index = index % arr2->count;
	}
	return true;
}

YoyoType* newArrayType(YoyoType** types, size_t count, YThread* th) {
	ArrayType* arr = malloc(sizeof(ArrayType));
	initHeapObject((HeapObject*) arr, ArrayType_mark, ArrayType_free);
	th->runtime->gc->registrate(th->runtime->gc, (HeapObject*) arr);
	arr->type.parent.type = &th->runtime->DeclarationType;

	StringBuilder* sb = newStringBuilder(L"[");
	for (size_t i = 0; i < count; i++) {
		sb->append(sb, types[i]->string);
		if (i + 1 < count)
			sb->append(sb, L", ");
		else
			sb->append(sb, L"]");
	}
	arr->type.string = sb->string;
	free(sb);
	arr->type.verify = ArrayType_verify;
	arr->type.compatible = ArrayType_compatible;
	arr->type.type = ArrayDT;

	arr->types = malloc(sizeof(YoyoType*) * count);
	memcpy(arr->types, types, sizeof(YoyoType*) * count);
	arr->count = count;

	return (YoyoType*) arr;
}

void LambdaSignature_mark(HeapObject* ptr) {
	ptr->marked = true;
	YoyoLambdaSignature* sig = (YoyoLambdaSignature*) ptr;
	MARK(sig->ret);
	for (int32_t i = 0; i < sig->argc; i++)
		MARK(sig->args[i]);
}
void LambdaSignature_free(HeapObject* ptr) {
	YoyoLambdaSignature* sig = (YoyoLambdaSignature*) ptr;
	free(sig->args);
	free(sig->type.string);
	free(sig);
}

bool LambdaSignature_verify(YoyoType* t, YValue* v, YThread* th) {
	if (v->type->type != LambdaT)
		return false;
	YoyoType* type = ((YLambda*) v)->signature((YLambda*) v, th);
	return type->compatible(t, type, th);
}
bool LambdaSignature_compatible(YoyoType* t1, YoyoType* t2, YThread* th) {
	YoyoLambdaSignature* sig1 = (YoyoLambdaSignature*) t1;
	if (t2->type != LambdaSignatureDT)
		return false;
	YoyoLambdaSignature* sig2 = (YoyoLambdaSignature*) t2;
	if (sig2->argc == -1)
		return true;
	if (sig1->argc != sig2->argc)
		return false;
	for (size_t i = 0; i < sig1->argc; i++)
		if (sig1->args[i] != NULL && sig2->args[i] != NULL
				&& !sig1->args[i]->compatible(sig1->args[i], sig2->args[i], th))
			return false;
	if (sig1->ret != NULL && sig2->ret != NULL
			&& !sig1->ret->compatible(sig1->ret, sig2->ret, th))
		return false;

	return true;
}

YoyoLambdaSignature* newLambdaSignature(int32_t argc, bool vararg,
		YoyoType** args, YoyoType* ret, YThread* th) {
	YoyoLambdaSignature* sig = calloc(1, sizeof(YoyoLambdaSignature));
	initHeapObject((HeapObject*) sig, LambdaSignature_mark,
			LambdaSignature_free);
	th->runtime->gc->registrate(th->runtime->gc, (HeapObject*) sig);
	sig->type.parent.type = &th->runtime->DeclarationType;

	sig->type.verify = LambdaSignature_verify;
	sig->type.compatible = LambdaSignature_compatible;
	sig->type.type = LambdaSignatureDT;

	sig->argc = argc;
	sig->ret = ret;
	sig->vararg = vararg;
	sig->args = calloc(1, sizeof(YoyoType*) * argc);
	if (args != NULL)
		memcpy(sig->args, args, sizeof(YoyoType*) * argc);

	StringBuilder* sb = newStringBuilder(L"(");

	if (sig->argc > 0)
		for (size_t i = 0; i < sig->argc; i++) {
			if (i + 1 >= sig->argc && sig->vararg)
				sb->append(sb, L"?");
			sb->append(sb,
					sig->args[i] != NULL ?
							sig->args[i]->string :
							th->runtime->NullType.TypeConstant->string);
			if (i + 1 < argc)
				sb->append(sb, L", ");
		}
	sb->append(sb, L")->");
	sb->append(sb,
			ret != NULL ?
					sig->ret->string :
					th->runtime->NullType.TypeConstant->string);
	sig->type.string = sb->string;
	free(sb);

	return sig;
}

typedef struct NotNullType {
	YoyoType type;

	YoyoType* ytype;
} NotNullType;
void NotNull_mark(HeapObject* ptr) {
	ptr->marked = true;
	NotNullType* nnt = (NotNullType*) ptr;
	MARK(nnt->ytype);
}
void NotNull_free(HeapObject* ptr) {
	free(ptr);
}
bool NotNull_verify(YoyoType* t, YValue* val, YThread* th) {
	if (val->type->type == AnyT)
		return false;
	NotNullType* nnt = (NotNullType*) t;
	return nnt->ytype->verify(nnt->ytype, val, th);
}
bool NotNull_compatible(YoyoType* t, YoyoType* t2, YThread* th) {
	YoyoType* t1 = ((NotNullType*) t)->ytype;
	if (t2->type == TypeMixDT) {
		YoyoTypeMix* mix = (YoyoTypeMix*) t2;
		for (size_t i = 0; i < mix->length; i++)
			if (t->compatible(t, mix->types[i], th))
				return true;
		return false;
	} else if (t2->type == NotNullDT) {
		NotNullType* nnt = (NotNullType*) t2;
		return t1->compatible(t1, nnt->ytype, th);
	}
	return t1->compatible(t1, t2, th);
}

YoyoType* newNotNullType(YoyoType* type, YThread* th) {
	NotNullType* nnt = malloc(sizeof(NotNullType));
	initHeapObject((HeapObject*) nnt, NotNull_mark, NotNull_free);
	th->runtime->gc->registrate(th->runtime->gc, (HeapObject*) nnt);
	nnt->type.parent.type = &th->runtime->DeclarationType;

	nnt->ytype = type;
	StringBuilder* sb = newStringBuilder(L"not-null:");
	sb->append(sb, type->string);
	nnt->type.string = sb->string;
	free(sb);
	nnt->type.type = NotNullDT;
	nnt->type.verify = NotNull_verify;
	nnt->type.compatible = NotNull_compatible;

	return (YoyoType*) nnt;
}
