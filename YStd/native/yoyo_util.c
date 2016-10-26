#include "yoyo-runtime.h"

YOYO_FUNCTION(YSTD_UTIL_RAND) {
	return newInteger(rand(), th);
}

YOYO_FUNCTION(YSTD_UTIL_RAND_SEED) {
	srand(getInteger(args[0], th));
	return getNull(th);
}

YOYO_FUNCTION(YSTD_UTIL_ARRAY_WRAP_OBJECT) {
	if (TYPE(args[0], &th->runtime->ObjectType)) {
		YObject* obj = CAST_OBJECT(args[0]);
		return (YValue*) newArrayObject(obj, th);
	} else
		return getNull(th);
}

YOYO_FUNCTION(YSTD_UTIL_ARRAY_TUPLE) {
	if (TYPE(args[0], &th->runtime->ArrayType)) {
		YArray* array = CAST_ARRAY(args[0]);
		return (YValue*) newTuple(array, th);
	} else
		return getNull(th);
}

YOYO_FUNCTION(YSTD_UTIL_OBJECT_READONLY) {
	if (TYPE(args[0], &th->runtime->ObjectType)) {
		YObject* obj = CAST_OBJECT(args[0]);
		return (YValue*) newReadonlyObject(obj, th);
	} else
		return getNull(th);
}

YOYO_FUNCTION(YSTD_UTIL_OBJECT_COMPLEX) {
	YObject* base = NULL;
	YObject** mixins = NULL;
	size_t mixinc = 0;
	if (argc > 0 && args[0]->type == &th->runtime->ObjectType) {
		base = (YObject*) args[0];
	} else {
		throwException(L"Expected base object", NULL, 0, th);
		return getNull(th);
	}
	for (size_t i = 1; i < argc; i++) {
		if (args[i]->type != &th->runtime->ObjectType) {
			free(mixins);
			throwException(L"Expected object", NULL, 0, th);
			return getNull(th);
		}
		mixins = realloc(mixins, sizeof(YObject*) * (++mixinc));
		mixins[mixinc - 1] = (YObject*) args[i];
	}
	YObject* cobj = newComplexObject(base, mixins, mixinc, th);
	free(mixins);
	return (YValue*) cobj;
}
