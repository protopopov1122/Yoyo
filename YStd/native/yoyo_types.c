#include "yoyo-runtime.h"

YOYO_FUNCTION(YSTD_TYPES_SIGNATURE) {
	YoyoType* yret = th->runtime->NullType.TypeConstant;
	YoyoType** yargs = NULL;
	int32_t yargc = 0;
	bool yvararg = false;

	if (argc > 0) {
		if (args[0]->type->type == DeclarationT)
			yret = (YoyoType*) args[0];
		else
			yret = args[0]->type->TypeConstant;
		yargc = argc - 1;
		yargs = malloc(sizeof(YoyoType*) * yargc);
		for (size_t i = 1; i < argc; i++) {
			if (args[i]->type->type == DeclarationT)
				yargs[i - 1] = (YoyoType*) args[i];
			else
				yargs[i - 1] = args[i]->type->TypeConstant;
		}
	}

	YValue* out = (YValue*) newLambdaSignature(false, yargc, yvararg, yargs,
			yret, th);
	free(yargs);
	return out;
}
YOYO_FUNCTION(YSTD_TYPES_TYPES) {
	YObject* obj = th->runtime->newObject(NULL, th);
#define NEW_TYPE(obj, type, tname, th) obj->put(obj, getSymbolId\
                                                        (&th->runtime->symbols, tname),\
                                                 (YValue*) th->runtime->type.TypeConstant, true, th);

	obj->put(obj, getSymbolId(&th->runtime->symbols, L"any"),
			(YValue*) newAtomicType(AnyT, th), true, th);
	NEW_TYPE(obj, DeclarationType, L"declaration", th);
	NEW_TYPE(obj, ArrayType, L"array", th);
	NEW_TYPE(obj, BooleanType, L"boolean", th);
	NEW_TYPE(obj, FloatType, L"float", th);
	NEW_TYPE(obj, IntType, L"int", th);
	NEW_TYPE(obj, LambdaType, L"lambda", th);
	NEW_TYPE(obj, NullType, L"null", th);
	NEW_TYPE(obj, ObjectType, L"struct", th);
	NEW_TYPE(obj, StringType, L"string", th);
#undef NEW_TYPE
	return (YValue*) obj;
}
YOYO_FUNCTION(YSTD_TYPES_TREE_OBJECT) {
	YObject* super = args[0]->type->type==ObjectT ? (YObject*) args[0] : NULL;
	return (YValue*) newTreeObject(super, th);
}

YOYO_FUNCTION(YSTD_TYPES_HASH_OBJECT) {
	YObject* super = args[0]->type->type==ObjectT ? (YObject*) args[0] : NULL;
	return (YValue*) newHashObject(super, th);
}
