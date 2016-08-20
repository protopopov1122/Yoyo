#include "yoyo-runtime.h"

YOYO_FUNCTION(YSTD_TYPES_SIGNATURE) {
	YoyoType* yret = th->runtime->NullType.TypeConstant;
	YoyoType** yargs = NULL;
	int32_t yargc = 0;
	bool yvararg = false;

	if (argc > 0) {
		if (args[0]->type == &th->runtime->DeclarationType)
			yret = (YoyoType*) args[0];
		else
			yret = args[0]->type->TypeConstant;
		yargc = argc - 1;
		yargs = malloc(sizeof(YoyoType*) * yargc);
		for (size_t i = 1; i < argc; i++) {
			if (args[i]->type == &th->runtime->DeclarationType)
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
			(YValue*) newAtomicType(&th->runtime->NullType, th), true, th);
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
	YObject* super = args[0]->type == &th->runtime->ObjectType ? (YObject*) args[0] : NULL;
	return (YValue*) newTreeObject(super, th);
}

YOYO_FUNCTION(YSTD_TYPES_HASH_OBJECT) {
	YObject* super = args[0]->type == &th->runtime->ObjectType ? (YObject*) args[0] : NULL;
	return (YValue*) newHashObject(super, th);
}

YOYO_FUNCTION(YSTD_TYPES_INTEGER_PARSE_INT) {
	wchar_t* wcs = toString(args[0], th);
	int64_t i = 0;
	if (parse_int(wcs, &i)) {
		free(wcs);
		return newInteger(i, th);	
	}
	throwException(L"NotInteger", &wcs, 1, th);
	free(wcs);
	return getNull(th);
}

YOYO_FUNCTION(YSTD_TYPES_INTEGER_MIN) {
	return newInteger(INT64_MIN, th);
}

YOYO_FUNCTION(YSTD_TYPES_INTEGER_MAX) {
	return newInteger(INT64_MAX, th);
}

YOYO_FUNCTION(YSTD_TYPES_FLOAT_PARSE) {
	wchar_t* wcs = toString(args[0], th);
	char *oldLocale = setlocale(LC_NUMERIC, NULL);
	setlocale(LC_NUMERIC, "C");
	double fp64 = wcstod(wcs, NULL);
	setlocale(LC_NUMERIC, oldLocale);
	return newFloat(fp64, th);
}

YOYO_FUNCTION(YSTD_TYPES_FLOAT_INF) {
	return newFloat(INFINITY, th);
}

YOYO_FUNCTION(YSTD_TYPES_FLOAT_NAN) {
	return newFloat(NAN, th);
}

YOYO_FUNCTION(YSTD_TYPES_STRING_FROM_BYTES) {
	if (args[0] == getNull(th))
		return getNull(th);
	YArray* arr = (YArray*) args[0];
	wchar_t* wcs = calloc(1, sizeof(wchar_t) * (arr->size(arr, th) + 1));
	for (size_t i=0;i<arr->size(arr, th);i++) {
		wcs[i] = (wchar_t) ((YInteger*) arr->get(arr, i, th))->value;
	}
	YValue* ystr = newString(wcs, th);
	free(wcs);
	return ystr;	
}

YOYO_FUNCTION(YSTD_TYPES_STRING_FROM_MULTIBYTES) {
	if (args[0] == getNull(th))
		return getNull(th);
	YArray* arr = (YArray*) args[0];
	char* mbs = calloc(1, sizeof(char) * (arr->size(arr, th) + 1));
	for (size_t i=0;i<arr->size(arr, th);i++) {
		mbs[i] = (char) ((YInteger*) arr->get(arr, i, th))->value;
	}
	wchar_t* wcs = calloc(1, sizeof(wchar_t) * (strlen(mbs) + 1));
	mbstowcs(wcs, mbs, sizeof(wchar_t) * strlen(mbs));
	YValue* ystr = newString(wcs, th);
	free(wcs);
	free(mbs);
	return ystr;	
}
