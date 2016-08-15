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
		CAST_OBJECT(obj, args[0]);
		return (YValue*) newArrayObject(obj, th);
	} else
		return getNull(th);
}

YOYO_FUNCTION(YSTD_UTIL_ARRAY_TUPLE) {
	if (TYPE(args[0], &th->runtime->ArrayType)) {
		CAST_ARRAY(array, args[0]);
		return (YValue*) newTuple(array, th);
	} else
		return getNull(th);
}

YOYO_FUNCTION(YSTD_UTIL_OBJECT_READONLY) {
	if (TYPE(args[0], &th->runtime->ObjectType)) {
		CAST_OBJECT(obj, args[0]);
		return (YValue*) newReadonlyObject(obj, th);
	} else
		return getNull(th);
}
